/*
 *  Copyright (c) 2015, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <chrono>
#include <string>

#include <folly/io/IOBufQueue.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/DelayedDestruction.h>

#include "mcrouter/lib/fbi/cpp/ObjectPool.h"
#include "mcrouter/lib/fibers/Baton.h"
#include "mcrouter/lib/network/ConnectionOptions.h"
#include "mcrouter/lib/network/McParser.h"
#include "mcrouter/lib/network/McSerializedRequest.h"
#include "mcrouter/lib/network/UniqueIntrusiveList.h"

namespace facebook { namespace memcache {

template <int op>
class McOperation;
class McReply;
class McRequest;

namespace detail {
class OnEventBaseDestructionCallback;
}

/**
 * A base class for network communication with memcache protocol.
 *
 * This is an impl class, user should use AsyncMcClient.
 */
class AsyncMcClientImpl :
      public folly::DelayedDestruction,
      private folly::AsyncSocket::ConnectCallback,
      private folly::AsyncTransportWrapper::ReadCallback,
      private folly::AsyncTransportWrapper::WriteCallback,
      private McParser::ClientParseCallback {

 public:

  static std::shared_ptr<AsyncMcClientImpl> create(folly::EventBase& eventBase,
                                                   ConnectionOptions options);

  AsyncMcClientImpl(const AsyncMcClientImpl&) = delete;
  AsyncMcClientImpl& operator=(const AsyncMcClientImpl&) = delete;

  // Fail all requests and close connection.
  void closeNow();

  void setStatusCallbacks(
    std::function<void()> onUp,
    std::function<void(const folly::AsyncSocketException&)> onDown);

  template <int Op>
  McReply sendSync(const McRequest& request, McOperation<Op>);

  template <int Op>
  void send(const McRequest& request, McOperation<Op>,
            std::function<void(McReply&&)> callback);

  void setThrottle(size_t maxInflight, size_t maxPending);

  size_t getPendingRequestCount() const;
  size_t getInflightRequestCount() const;
  std::pair<uint64_t, uint64_t> getBatchingStat() const;

 private:
  class TimeoutCallback;

  enum class ConnectionState {
    UP, // Connection is open and we can write into it.
    DOWN, // Connection is not open (or close), we need to reconnect.
    CONNECTING, // Currently connecting.
    ERROR // Currently processing error.
  };

  struct ConnectionStatusCallbacks {
    std::function<void()> onUp;
    std::function<void(const folly::AsyncSocketException&)>
      onDown;
  };

  // Class for storing internal data for each request.
  class ReqInfo {
   public:
    McSerializedRequest reqContext;

    uint64_t id;
    mc_op_t op;
    std::chrono::steady_clock::time_point sentAt;

    struct SyncContext {
      Baton baton;
      McReply reply;
    };

    struct AsyncContext {
      std::function<void(McReply&&)> replyCallback;
      explicit AsyncContext(std::function<void(McReply&&)> replyCallback_)
        : replyCallback(replyCallback_) {
      }
    };

    union {
      SyncContext syncContext;
      AsyncContext asyncContext;
    };

    std::function<void(const McReply&)> traceCallback;

    template <int Op>
    ReqInfo(const McRequest& request,
            uint64_t reqid,
            McOperation<Op>,
            mc_protocol_t protocol,
            std::shared_ptr<AsyncMcClientImpl> client)
      : reqContext(request, McOperation<Op>(), reqid, protocol),
        id(reqid),
        op((mc_op_t)Op),
        syncContext(),
        client_(client),
        isSync_(true) {
    }

    template <int Op>
    ReqInfo(const McRequest& request,
            uint64_t reqid,
            McOperation<Op>,
            mc_protocol_t protocol,
            std::function<void(McReply&&)> callback,
            std::shared_ptr<AsyncMcClientImpl> client)
      : reqContext(request, McOperation<Op>(), reqid, protocol),
        id(reqid),
        op((mc_op_t)Op),
        asyncContext(std::move(callback)),
        client_(client),
        isSync_(false) {
    }

    ~ReqInfo() {
      if (isSync()) {
        syncContext.~SyncContext();
      } else {
        asyncContext.~AsyncContext();
      }
    }

    ReqInfo(const ReqInfo&) = delete;
    ReqInfo& operator=(const ReqInfo& other) = delete;

    bool isSync() const {
      return isSync_;
    }

   private:
    static ObjectPool<ReqInfo>& getPool() {
      static thread_local ObjectPool<ReqInfo> pool(1024);
      return pool;
    }

    class Deleter {
     public:
      void operator()(ReqInfo* ptr) const {
        // Sync requests are allocated on stack, thus we can't destroy them.
        if (!ptr->isSync()) {
          getPool().free(ptr);
        }
      }
    };

   public:
    using UniquePtr = std::unique_ptr<ReqInfo, Deleter>;

    template <typename... Args>
    static UniquePtr getFromPool(Args&&... args) {
      auto ptr =
        UniquePtr(getPool().alloc(std::forward<Args>(args)...), Deleter());
      assert(!ptr->isSync_);
      return ptr;
    }

    UniquePtr createDummyPtr() {
      assert(isSync_);
      return UniquePtr(this, Deleter());
    }

   private:
    std::shared_ptr<AsyncMcClientImpl> client_;
    UniqueIntrusiveListHook hook_;
    bool isSync_{false};

   public:
    using RequestQueue = UniqueIntrusiveList<ReqInfo, &ReqInfo::hook_, Deleter>;
  };

  // We need to be able to get shared_ptr to ourself and shared_from_this()
  // doesn't work correctly with TDelayedDestruction.
  std::weak_ptr<AsyncMcClientImpl> selfPtr_;

  // Queue of requests, that are queued to be sent.
  ReqInfo::RequestQueue sendQueue_;
  // Queue of requests, that are currently being written to the socket.
  ReqInfo::RequestQueue writeQueue_;
  // Queue of requests, that are already sent and are waiting for replies.
  ReqInfo::RequestQueue pendingReplyQueue_;

  // Stats.
  std::pair<uint64_t, uint16_t> batchStatPrevious{0, 0};
  std::pair<uint64_t, uint16_t> batchStatCurrent{0, 0};

  // Id to request map. Used only in case of out-of-order protocol for fast
  // request lookup.
  std::unordered_map<uint64_t, ReqInfo*> idMap_;

  folly::EventBase& eventBase_;
  std::unique_ptr<McParser> parser_;

  // Socket related variables.
  ConnectionState connectionState_{ConnectionState::DOWN};
  folly::IOBufQueue buffer_;
  ConnectionOptions connectionOptions_;
  folly::AsyncTransportWrapper::UniquePtr socket_;
  ConnectionStatusCallbacks statusCallbacks_;

  bool outOfOrder_{false};

  // Id for the next message that will be used by the next sendMsg() call.
  uint64_t nextMsgId_{1};

  // Id of the next message pending for reply (request is already sent).
  // Only for in order protocol.
  uint64_t nextInflightMsgId_{1};

  // Throttle options (disabled by default).
  size_t maxPending_{0};
  size_t maxInflight_{0};

  // Timeout for sending requests.
  bool timeoutScheduled_{false};
  std::unique_ptr<TimeoutCallback> timeoutCallback_;

  // Writer loop related variables.
  class WriterLoop;
  bool writeScheduled_{false};
  std::unique_ptr<WriterLoop> writer_;

  bool isAborting_{false};
  std::unique_ptr<detail::OnEventBaseDestructionCallback>
    eventBaseDestructionCallback_;

  AsyncMcClientImpl(folly::EventBase& eventBase, ConnectionOptions options);

  ~AsyncMcClientImpl();

  // Common part for send/sendSync.
  void sendCommon(ReqInfo::UniquePtr req);

  // Write some requests from sendQueue_ to the socket, until max inflight limit
  // is reached or queue is empty.
  void pushMessages();
  // Callback for request timeout event.
  void timeoutExpired();
  // Schedule timeout event for the next request in the queue.
  void scheduleNextTimeout();
  // Schedule next writer loop if it's not scheduled.
  void scheduleNextWriterLoop();
  void cancelWriterCallback();

  // call reply callback for the request and remove it from requests map
  void reply(ReqInfo::UniquePtr req, McReply mcReply);

  // reply request with the reply received from network
  void replyReceived(uint64_t id, McReply mcReply);

  void attemptConnection();

  // TAsyncSocket::ConnectCallback overrides
  void connectSuccess() noexcept override;
  void connectErr(const folly::AsyncSocketException& ex) noexcept override;

  // We've have encountered some error or we're shutting down the client.
  // It goes to DOWN state.
  void processShutdown();

  // AsyncTransportWrapper::ReadCallback overrides
  void getReadBuffer(void** bufReturn, size_t* lenReturn) override;
  void readDataAvailable(size_t len) noexcept override;
  void readEOF() noexcept override;
  void readErr(const folly::AsyncSocketException& ex) noexcept override;

  // AsyncTransportWrapper::WriteCallback overrides
  void writeSuccess() noexcept override;
  void writeErr(size_t bytesWritten,
                const folly::AsyncSocketException& ex) noexcept override;

  // McParser::ClientParseCallback overrides
  void replyReady(McReply mcReply, mc_op_t operation, uint64_t reqid) override;
  void parseError(McReply errorReply) override;

  void sendFakeReply(ReqInfo& request);

  static void incMsgId(size_t& msgId);
};

}} // facebook::memcache

#include "AsyncMcClientImpl-inl.h"
