/*
 *  Copyright (c) 2015, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "squangle/mysql_client/AsyncConnectionPool.h"
#include "squangle/mysql_client/AsyncMysqlClient.h"
#include "squangle/mysql_client/FutureAdapter.h"

#include <memory>

#include "folly/Memory.h"

#include <vector>

#include <mysql.h>
#include <mysqld_error.h>
#include <chrono>

namespace facebook {
namespace common {
namespace mysql_client {

constexpr std::chrono::milliseconds PoolOptions::kCleanUpTimeout;
constexpr Duration PoolOptions::kDefaultMaxIdleTime;
constexpr Duration PoolOptions::kDefaultMaxAge;

MysqlPooledHolder::MysqlPooledHolder(
    std::unique_ptr<MysqlConnectionHolder> holder_base,
    std::weak_ptr<AsyncConnectionPool> weak_pool)
    : MysqlConnectionHolder(std::move(holder_base)),
      good_for_(Duration::zero()),
      weak_pool_(weak_pool) {
  auto lock_pool = weak_pool.lock();
  if (lock_pool) {
    lock_pool->stats()->incrCreatedPoolConnections();
    lock_pool->addOpenConnection(getKey());
  }
}

MysqlPooledHolder::~MysqlPooledHolder() { removeFromPool(); }

void MysqlPooledHolder::setOwnerPool(std::weak_ptr<AsyncConnectionPool> pool) {
  // In case this connection belonged to a pool before
  removeFromPool();
  weak_pool_ = pool;
  auto lock_pool = weak_pool_.lock();
  // Extra care here, checking if we changing it to nullptr
  if (lock_pool) {
    lock_pool->stats()->incrCreatedPoolConnections();
    lock_pool->addOpenConnection(getKey());
  }
}

void MysqlPooledHolder::removeFromPool() {
  auto lock_pool = weak_pool_.lock();
  if (lock_pool) {
    lock_pool->stats()->incrDestroyedPoolConnections();
    lock_pool->removeOpenConnection(getKey());
  }
}

std::shared_ptr<AsyncConnectionPool> AsyncConnectionPool::makePool(
    AsyncMysqlClient* mysql_client, const PoolOptions& pool_options) {

  auto pool = std::make_shared<AsyncConnectionPool>(mysql_client, pool_options);
  // No need for explicit conversion, but this is converting to weak pointer
  pool->setSelfWeakPointer(pool);

  return pool;
}

AsyncConnectionPool::AsyncConnectionPool(AsyncMysqlClient* mysql_client,
                                         const PoolOptions& pool_options)
    : conn_storage_(mysql_client->threadId(),
                    pool_options.getPoolLimit() * 2,
                    pool_options.getIdleTimeout()),
      cleanup_timer_(mysql_client->getEventBase(), &conn_storage_),
      mysql_client_(mysql_client),
      conn_per_key_limit_(pool_options.getPerKeyLimit()),
      pool_conn_limit_(pool_options.getPoolLimit()),
      connection_age_timeout_(pool_options.getAgeTimeout()),
      expiration_policy_(pool_options.getExpPolicy()),
      finished_shutdown_(false) {
  if (!mysql_client_->runInThread([this]() {
        cleanup_timer_.scheduleTimeout(PoolOptions::kCleanUpTimeout);
      })) {
    LOG(DFATAL) << "Unable to schedule timeout due Thrift event issue";
  }
}

AsyncConnectionPool::~AsyncConnectionPool() {
  VLOG(2) << "Connection pool dying";
  if (!finished_shutdown_.load(std::memory_order_acquire)) {
    shutdown();
  }

  VLOG(2) << "Connection pool shutdown completed";
}

void AsyncConnectionPool::shutdown() {
  VLOG(2) << "Shutting down";
  std::unique_lock<std::mutex> lock(shutdown_mutex_);
  // Will block adding anything to the pool
  shutting_down_ = true;

  // cancelTimeout can only be ran in the tevent thread
  if (std::this_thread::get_id() == mysql_client_->threadId()) {
    cleanup_timer_.cancelTimeout();
    conn_storage_.clearAll();
    finished_shutdown_.store(true, std::memory_order_relaxed);
    LOG(INFO) << "Shutting down in tevent thread";
  } else {
    mysql_client_->runInThread([this]() {
      cleanup_timer_.cancelTimeout();
      conn_storage_.clearAll();
      // Reacquire lock
      std::unique_lock<std::mutex> lock(shutdown_mutex_);
      finished_shutdown_.store(true, std::memory_order_relaxed);
      this->shutdown_condvar_.notify_one();
    });
    shutdown_condvar_.wait(lock, [this] {
      return finished_shutdown_.load(std::memory_order_acquire);
    });
  }
}

folly::Future<ConnectResult> AsyncConnectionPool::connectFuture(
    const string& host,
    int port,
    const string& database_name,
    const string& user,
    const string& password,
    const ConnectionOptions& conn_opts) {
  return connectFuture(
      host, port, database_name, user, password, "", conn_opts);
}

folly::Future<ConnectResult> AsyncConnectionPool::connectFuture(
    const string& host,
    int port,
    const string& database_name,
    const string& user,
    const string& password,
    const string& special_tag,
    const ConnectionOptions& conn_opts) {
  return toFuture(
      beginConnection(host, port, database_name, user, password, special_tag)
          ->setConnectionOptions(conn_opts));
}

std::unique_ptr<Connection> AsyncConnectionPool::connect(
    const string& host,
    int port,
    const string& database_name,
    const string& user,
    const string& password,
    const ConnectionOptions& conn_opts) {
  auto op = beginConnection(host, port, database_name, user, password);
  op->setConnectionOptions(conn_opts);
  // This will throw (intended behaviour) in case the operation didn't succeed
  return std::move(blockingConnectHelper(op));
}

std::shared_ptr<ConnectOperation> AsyncConnectionPool::beginConnection(
    const string& host,
    int port,
    const string& database_name,
    const string& user,
    const string& password,
    const string& special_tag) {
  std::shared_ptr<ConnectPoolOperation> ret;
  {
    std::unique_lock<std::mutex> lock(shutdown_mutex_);
    // Assigning here to read from pool safely
    ret = std::make_shared<ConnectPoolOperation>(
        self_pointer_,
        mysql_client_,
        ConnectionKey(host, port, database_name, user, password, special_tag));
    if (shutting_down_) {
      LOG(ERROR)
          << "Attempt to start pool operation while pool is shutting down";
      ret->cancel();
    }
  }

  mysql_client_->addOperation(ret);
  return ret;
}

void AsyncConnectionPool::recycleMysqlConnection(
    std::unique_ptr<MysqlConnectionHolder> mysql_conn) {
  // this method can run by any thread where the Connection is dying
  {
    std::unique_lock<std::mutex> lock(shutdown_mutex_);
    if (shutting_down_) {
      return;
    }
  }
  VLOG(2) << "Trying to recycle connection";

  if (!mysql_conn->isReusable()) {
    return;
  }

  // Check server_status for in_transaction bit
  if (mysql_conn->inTransaction()) {
    // To avoid complication, we are just going to close the connection
    LOG_EVERY_N(ERROR, 1000) << "Closing connection during a transaction "
                                "without explicitly committing";
    return;
  }

  auto pool = self_pointer_;
  auto pmysql_conn = mysql_conn.release();
  bool scheduled = mysql_client_->runInThread([pool, pmysql_conn]() {
    std::unique_ptr<MysqlPooledHolder> mysql_conn(
        static_cast<MysqlPooledHolder*>(pmysql_conn));
    auto shared_pool = pool.lock();
    if (!shared_pool) {
      return;
    }

    // in mysql 5.7 we can use mysql_reset_connection
    // We don't have a nonblocking version for reset connection, so we
    // are going to delete the old one and the open connection being
    // removed procedure is going to check if it needs to open new one
    shared_pool->addConnection(std::move(mysql_conn), false);
  });

  if (!scheduled) {
    delete pmysql_conn;
  }
}

void AsyncConnectionPool::registerForConnection(
    ConnectPoolOperation* raw_pool_op) {
  // Runs only in main thread by run() in the ConnectPoolOperation
  CHECK_EQ(std::this_thread::get_id(), mysql_client_->threadId());
  {
    std::unique_lock<std::mutex> lock(shutdown_mutex_);
    if (shutting_down_) {
      VLOG(4) << "Pool is shutting down, operation being canceled";
      raw_pool_op->cancel();
      return;
    }
  }
  stats()->incrConnectionsRequested();
  auto conn_key = raw_pool_op->getKey();
  std::unique_ptr<MysqlPooledHolder> mysql_conn =
      conn_storage_.popConnection(conn_key);

  if (mysql_conn == nullptr) {
    stats()->incrPoolMisses();
    // TODO: Check if we are jammed and fail fast

    // The client holds shared pointers for all active operations
    // this method is called by the `run()` in the operation, so it
    // should always exist in the client
    auto pool_op = std::dynamic_pointer_cast<ConnectPoolOperation>(
      raw_pool_op->getSharedPointer());
    // Sanity check
    DCHECK(pool_op != nullptr);
    conn_storage_.queueOperation(pool_op);

    tryRequestNewConnection(conn_key, raw_pool_op->getAttemptTimeout());
  } else {
    // Cache hit
    stats()->incrPoolHits();

    mysql_conn->setReusable(true);
    raw_pool_op->connectionCallback(std::move(mysql_conn));
  }
}

bool AsyncConnectionPool::canCreateMoreConnections(
    const ConnectionKey* conn_key) {
  CHECK_EQ(std::this_thread::get_id(), mysql_client_->threadId());
  std::unique_lock<std::mutex> l(counter_mutex_);
  auto open_conns = open_connections_[*conn_key];
  auto pending_conns = pending_connections_[*conn_key];

  auto enqueued_pool_ops = conn_storage_.numQueuedOperations(conn_key);

  auto client_total_conns = mysql_client_->numStartedAndOpenConnections();
  auto client_conn_limit = mysql_client_->getPoolsConnectionLimit();

  // We have the number of connections we are opening and the number of already
  // open, we shouldn't try to create over this sum
  int num_pool_allocated = num_open_connections_ + num_pending_connections_;
  int num_per_key_allocated = open_conns + pending_conns;

  // First we check global limit, then limits of the pool. If we can create more
  // connections, we check if we need comparing the amount of already being
  // opened connections for that key with the number of enqueued operations (the
  // operation that is requesting a new connection should be enqueued at this
  // point.
  if (client_total_conns < client_conn_limit &&
      num_pool_allocated < pool_conn_limit_ &&
      num_per_key_allocated < conn_per_key_limit_ &&
      pending_conns < enqueued_pool_ops) {
    return true;
  }
  return false;
}

std::pair<uint64_t, uint64_t> AsyncConnectionPool::getConnKeyStatus(
    const ConnectionKey* conn_key) {
  std::unique_lock<std::mutex> l(counter_mutex_);
  auto open_conns = open_connections_[*conn_key];
  auto pending_conns = pending_connections_[*conn_key];
  return std::make_pair(open_conns, pending_conns);
}

void AsyncConnectionPool::addOpenConnection(const ConnectionKey* conn_key) {
  std::unique_lock<std::mutex> l(counter_mutex_);
  ++open_connections_[*conn_key];
  ++num_open_connections_;
}

void AsyncConnectionPool::removeOpenConnection(const ConnectionKey* conn_key) {
  std::unique_lock<std::mutex> l(counter_mutex_);

  auto iter = open_connections_.find(*conn_key);
  DCHECK(iter != open_connections_.end());
  if (--iter->second == 0) {
    open_connections_.erase(iter);
  }

  --num_open_connections_;
  connectionSpotFreed(conn_key);
}

void AsyncConnectionPool::addOpeningConn(const ConnectionKey* conn_key) {
  std::unique_lock<std::mutex> l(counter_mutex_);
  ++pending_connections_[*conn_key];
  ++num_pending_connections_;
}

void AsyncConnectionPool::removeOpeningConn(const ConnectionKey* conn_key) {
  std::unique_lock<std::mutex> l(counter_mutex_);
  --pending_connections_[*conn_key];
  --num_pending_connections_;
}

void AsyncConnectionPool::connectionSpotFreed(const ConnectionKey* conn_key) {
  // Now we check if we should create more connections in case there are queued
  // operations in need
  auto weak_pool = self_pointer_;
  auto key = *conn_key;
  mysql_client_->runInThread([weak_pool, key]() {
    auto pool = weak_pool.lock();
    if (pool) {
      pool->tryRequestNewConnection(&key);
    }
  });
}

void AsyncConnectionPool::tryRequestNewConnection(const ConnectionKey* conn_key,
                                                  Duration timeout) {
  // Only called internally, this doesn't need to check if it's shutting
  // down
  CHECK_EQ(std::this_thread::get_id(), mysql_client_->threadId());
  {
    std::unique_lock<std::mutex> lock(shutdown_mutex_);
    if (shutting_down_) {
      return;
    }
  }

  // Checking if limits allow creating more connections
  if (canCreateMoreConnections(conn_key)) {
    VLOG(11) << "Requesting new Connection";
    // get a shared pointer for operation

    auto connOp = mysql_client_->beginConnection(*conn_key);
    connOp->setTimeout(timeout);
    auto pool_ptr = self_pointer_;
    // The attribute part we can do later
    connOp->setCallback([pool_ptr](ConnectOperation& connOp) {
      auto locked_pool = pool_ptr.lock();
      if (!locked_pool) {
        return;
      }
      if (!connOp.ok()) {
        VLOG(2) << "Failed to create new connection";
        locked_pool->removeOpeningConn(connOp.getKey());
        locked_pool->failedToConnect(connOp);
        return;
      }
      auto conn = connOp.releaseConnection();
      auto mysql_conn = conn->stealMysqlConnectionHolder();
      // Now we got a connection from the client, it will become a pooled
      // connection
      auto pooled_conn = folly::make_unique<MysqlPooledHolder>(
          std::move(mysql_conn), pool_ptr);
      locked_pool->removeOpeningConn(pooled_conn->getKey());
      locked_pool->addConnection(std::move(pooled_conn), true);
    });

    try {
      connOp->run();
      addOpeningConn(conn_key);
    }
    catch (OperationStateException& e) {
      LOG(ERROR) << "Client is drain or dying, cannot ask for more connections";
    }
  }
}

void AsyncConnectionPool::failedToConnect(ConnectOperation& conn_op) {
  // Propagating ConnectOperation failure to queued operations in case
  // This will help us fail fast incorrect passwords or users.
  if (conn_op.result() == OperationResult::Failed) {
    conn_storage_.failOperations(conn_op.getKey(),
                                 conn_op.result(),
                                 conn_op.mysql_errno(),
                                 conn_op.mysql_error());
  }
  connectionSpotFreed(conn_op.getKey());
}

// Shall be called anytime a fresh connection is ready or a recycled
void AsyncConnectionPool::addConnection(
    std::unique_ptr<MysqlPooledHolder> mysql_conn, bool brand_new) {
  // Only called internally, this doesn't need to check if it's shutting
  // down
  CHECK_EQ(std::this_thread::get_id(), mysql_client_->threadId());
  if (brand_new) {
    if (expiration_policy_ == ExpirationPolicy::Age) {
      // TODO add noise to expiration age
      mysql_conn->setLifeDuration(connection_age_timeout_);
    }
  }

  VLOG(11) << "New connection ready to be used";
  auto pool_op = conn_storage_.popOperation(mysql_conn->getKey());
  if (pool_op == nullptr) {
    VLOG(11) << "No operations waiting for Connection, enqueueing it";
    conn_storage_.queueConnection(std::move(mysql_conn));
  } else {
    mysql_conn->setReusable(true);
    pool_op->connectionCallback(std::move(mysql_conn));
  }
}

AsyncConnectionPool::CleanUpTimer::CleanUpTimer(ata::TEventBase* base,
                                                ConnStorage* pool)
    : ata::TAsyncTimeout(base), pool_(pool) {}

void AsyncConnectionPool::CleanUpTimer::timeoutExpired() noexcept {
  pool_->cleanupConnections();
  pool_->cleanupOperations();
  scheduleTimeout(PoolOptions::kCleanUpTimeout);
}

std::shared_ptr<ConnectPoolOperation>
AsyncConnectionPool::ConnStorage::popOperation(const ConnectionKey* conn_key) {
  CHECK_EQ(std::this_thread::get_id(), allowed_thread_id_);

  PoolOpList& list = waitList_[*conn_key];
  while (!list.empty()) {
    std::weak_ptr<ConnectPoolOperation> weak_op = list.front();
    list.pop_front();
    auto ret = weak_op.lock();
    if (ret && !ret->done()) {
      VLOG(11) << "Operation found for " << conn_key->getDisplayString();
      return ret;
    }
  }

  VLOG(11) << "No operations in waitList " << conn_key->getDisplayString();
  return nullptr;
}

void AsyncConnectionPool::ConnStorage::queueOperation(
    std::shared_ptr<ConnectPoolOperation>& pool_op) {
  CHECK_EQ(std::this_thread::get_id(), allowed_thread_id_);

  PoolOpList& list = waitList_[*pool_op->getKey()];
  std::weak_ptr<ConnectPoolOperation> weak_op = pool_op;
  list.push_back(std::move(weak_op));
  VLOG(11) << "Enqueued pool operation in "
           << pool_op->getKey()->getDisplayString();
}

void AsyncConnectionPool::ConnStorage::failOperations(
    const ConnectionKey* conn_key,
    OperationResult op_result,
    int mysql_errno,
    const string& mysql_error) {
  CHECK_EQ(std::this_thread::get_id(), allowed_thread_id_);

  PoolOpList& list = waitList_[*conn_key];
  while (!list.empty()) {
    std::weak_ptr<ConnectPoolOperation> weak_op = list.front();
    list.pop_front();
    auto lock_op = weak_op.lock();
    if (lock_op && !lock_op->done()) {
      lock_op->failureCallback(op_result, mysql_errno, mysql_error);
    }
  }
}

std::unique_ptr<MysqlPooledHolder>
AsyncConnectionPool::ConnStorage::popConnection(const ConnectionKey* conn_key) {
  CHECK_EQ(std::this_thread::get_id(), allowed_thread_id_);

  auto iter = stock_.find(*conn_key);
  if (iter == stock_.end() || iter->second.empty()) {
    VLOG(11) << "No connections in queue for " << conn_key->getDisplayString();
    return nullptr;
  } else {
    VLOG(11) << "Connection found for " << conn_key->getDisplayString();
    std::unique_ptr<MysqlPooledHolder> ret;
    ret = std::move(iter->second.front());
    iter->second.pop_front();
    return ret;
  }
}

void AsyncConnectionPool::ConnStorage::queueConnection(
    std::unique_ptr<MysqlPooledHolder> newConn) {
  CHECK_EQ(std::this_thread::get_id(), allowed_thread_id_);

  // If it doesn't have space, remove the oldest and add this
  MysqlConnectionList& list = stock_[*newConn->getKey()];
  Timepoint now = std::chrono::high_resolution_clock::now();
  idle_expiration_time_[newConn.get()] = now + max_idle_time_;

  list.push_back(std::move(newConn));
  if (list.size() > conn_limit_) {
    list.pop_front();
  }
}

void AsyncConnectionPool::ConnStorage::cleanupConnections() {
  CHECK_EQ(std::this_thread::get_id(), allowed_thread_id_);

  Timepoint now = std::chrono::high_resolution_clock::now();
  for (auto connListIt = stock_.begin(); connListIt != stock_.end();) {
    auto& connList = connListIt->second;
    for (MysqlConnectionList::iterator it = connList.begin();
         it != connList.end();) {
      bool shouldDelete = false;

      shouldDelete =
          ((*it)->getLifeDuration() != Duration::zero() &&
           ((*it)->getCreationTime() + (*it)->getLifeDuration() < now)) ||
          idle_expiration_time_[(*it).get()] < now;
      // TODO maybe check if by any chance the connection was killed
      if (shouldDelete) {
        idle_expiration_time_.erase((*it).get());
        it = connList.erase(it);
      } else {
        ++it;
      }
    }
    if (connList.empty()) {
      connListIt = stock_.erase(connListIt);
    } else {
      ++connListIt;
    }
  }
}

void AsyncConnectionPool::ConnStorage::cleanupOperations() {
  CHECK_EQ(std::this_thread::get_id(), allowed_thread_id_);

  for (auto poolOpListIt = waitList_.begin();
       poolOpListIt != waitList_.end();) {
    auto& poolOpList = poolOpListIt->second;
    for (PoolOpList::iterator it = poolOpList.begin();
         it != poolOpList.end();) {
      // check if weak pointer expired
      auto op = (*it).lock();
      if (!op || op->done()) {
        it = poolOpList.erase(it);
        VLOG(11) << "Operation being erased during clean up";
      } else {
        ++it;
      }
    }
    if (poolOpList.empty()) {
      poolOpListIt = waitList_.erase(poolOpListIt);
    } else {
      ++poolOpListIt;
    }
  }
}

void AsyncConnectionPool::ConnStorage::clearAll() {
  CHECK_EQ(std::this_thread::get_id(), allowed_thread_id_);

  // Clearing all operations in the queue
  for (auto& poolOpListIt : waitList_) {
    auto& poolOpList = poolOpListIt.second;
    for (PoolOpList::iterator it = poolOpList.begin(); it != poolOpList.end();
         ++it) {
      // check if weak pointer expired
      auto locked_op = (*it).lock();
      if (locked_op) {
        locked_op->cancel();
        VLOG(2) << "Cancelling operation in the pool during clean up";
      }
    }
  }
  waitList_.clear();
  // For the connections we don't need to close one by one, we can just
  // clear the list and leave the destructor to handle it.
  stock_.clear();
}

void ConnectPoolOperation::attemptFailed(OperationResult result) {
  ++attempts_made_;
  if (shouldCompleteOperation(result)) {
    completeOperation(result);
    return;
  }

  conn()->socketHandler()->unregisterHandler();
  conn()->socketHandler()->cancelTimeout();

  auto now = std::chrono::high_resolution_clock::now();
  // Adjust timeout
  auto timeout_attempt_based =
      attempt_timeout_ +
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
  timeout_ = min(timeout_attempt_based, total_timeout_);

  specializedRun();
}

ConnectPoolOperation* ConnectPoolOperation::specializedRun() {
  if (!async_client()->runInThread([this]() {
        // There is a race condition that allows a cancelled operation
        // getting here, but checking inside the main thread again is fine.

    // Initialize all we need from our tevent handler
    if (attempts_made_ == 0) {
      conn()->associateWithClientThread();
    }
    conn()->socketHandler()->setOperation(this);

    // Set timeout for waiting for connection
    auto end = timeout_ + start_time_;
        auto now = std::chrono::high_resolution_clock::now();
        if (now >= end) {
          timeoutTriggered();
          return;
        }

        conn()->socketHandler()->scheduleTimeout(
            std::chrono::duration_cast<std::chrono::milliseconds>(end - now)
                .count());

        auto shared_pool = pool_.lock();
        // Remove before to not count against itself
        removeClientReference();
        if (shared_pool) {
          shared_pool->registerForConnection(this);
        } else {
          VLOG(2) << "Pool is gone, operation must cancel";
          this->cancel();
        }
      })) {
    completeOperationInner(OperationResult::Failed);
  }
  return this;
}

void ConnectPoolOperation::specializedTimeoutTriggered() {
  auto locked_pool = pool_.lock();
  if (locked_pool) {
    // Check if the timeout happened because of the host is being slow or the
    // pool is lacking resources
    auto key_status = locked_pool->getConnKeyStatus(getKey());
    auto num_open = key_status.first;
    auto num_opening = key_status.second;

    // As a way to be realistic regarding the reason a connection was not
    // obtained, we start from the principle that this is pool's fault.
    // We can only blame the host (by forwarding 2013) if we have no
    // open connections and none trying to be open.
    // The second rule is applied where the resource restriction is so small
    // that the pool can't even try to open a connection.
    if (!(num_open == 0 && (num_opening > 0 ||
                            locked_pool->canCreateMoreConnections(getKey())))) {
      auto delta = std::chrono::high_resolution_clock::now() - start_time_;
      int64_t delta_micros =
          std::chrono::duration_cast<std::chrono::microseconds>(delta).count();
      auto msg = folly::stringPrintf(
          "connection to %s:%d timed out in pool(open %lu, opening %lu) (took "
          "%.2fms)",
          host().c_str(),
          port(),
          num_open,
          num_opening,
          delta_micros / 1000.0);
      setAsyncClientError(ER_OUT_OF_RESOURCES, msg);
      attemptFailed(OperationResult::TimedOut);
      return;
    }
  }

  ConnectOperation::specializedTimeoutTriggered();
}

void ConnectPoolOperation::connectionCallback(
    std::unique_ptr<MysqlPooledHolder> mysql_conn) {
  CHECK_EQ(std::this_thread::get_id(), async_client()->threadId());
  if (!mysql_conn) {
    LOG(DFATAL) << "Unexpected error";
    completeOperation(OperationResult::Failed);
    return;
  }

  conn()->socketHandler()->changeHandlerFD(
      mysql_get_file_descriptor(mysql_conn->mysql()));

  conn()->setMysqlConnectionHolder(std::move(mysql_conn));
  auto pool = pool_;
  conn()->setConnectionDyingCallback([pool](
      std::unique_ptr<MysqlConnectionHolder> mysql_conn) {
    auto shared_pool = pool.lock();
    if (shared_pool) {
      shared_pool->recycleMysqlConnection(std::move(mysql_conn));
    }
  });
  if (conn()->mysql()) {
    attemptSucceeded(OperationResult::Succeeded);
  } else {
    VLOG(2) << "Error: Failed to acquire connection";
    attemptFailed(OperationResult::Failed);
  }
}

void ConnectPoolOperation::failureCallback(OperationResult failure,
                                           int mysql_errno,
                                           const string& mysql_error) {
  mysql_errno_ = mysql_errno;
  mysql_error_ = mysql_error;
  attemptFailed(failure);
}

void ConnectPoolOperation::socketActionable() {
  CHECK_EQ(std::this_thread::get_id(), async_client()->threadId());

  LOG(DFATAL) << "Should not be called";
}
}
}
} // namespace facebook::common::mysql_client
