/*
 *  Copyright (c) 2015, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/Memory.h>

#include "mcrouter/lib/network/AsyncMcClientImpl.h"

namespace facebook { namespace memcache {

inline AsyncMcClient::AsyncMcClient(folly::EventBase& eventBase,
                                    ConnectionOptions options) :
    base_(AsyncMcClientImpl::create(eventBase, std::move(options))) {
}

inline void AsyncMcClient::closeNow() {
  base_->closeNow();
}

inline void AsyncMcClient::setStatusCallbacks(
    std::function<void()> onUp,
    std::function<void(const folly::AsyncSocketException&)> onDown) {
  base_->setStatusCallbacks(std::move(onUp), std::move(onDown));
}

template <typename Operation>
McReply AsyncMcClient::sendSync(const McRequest& request, Operation) {
  return base_->sendSync(request, Operation());
}

template <typename Operation>
void AsyncMcClient::send(const McRequest& request, Operation,
                         std::function<void(McReply&&)> callback) {
  base_->send(request, Operation(), std::move(callback));
}

inline void AsyncMcClient::setThrottle(size_t maxInflight, size_t maxPending) {
  base_->setThrottle(maxInflight, maxPending);
}

inline size_t AsyncMcClient::getPendingRequestCount() const {
  return base_->getPendingRequestCount();
}

inline size_t AsyncMcClient::getInflightRequestCount() const {
  return base_->getInflightRequestCount();
}

inline std::pair<uint64_t, uint64_t> AsyncMcClient::getBatchingStat() const {
  return base_->getBatchingStat();
}

}} // facebook::memcache
