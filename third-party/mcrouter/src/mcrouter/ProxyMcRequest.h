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

#include "mcrouter/config.h"
#include "mcrouter/lib/McRequestWithContext.h"
#include "mcrouter/lib/Operation.h"

namespace facebook { namespace memcache { namespace mcrouter {

class ProxyMcReply;
class ProxyRequestContext;

enum class RequestClass {
  NORMAL,
  FAILOVER,
  SHADOW,
};

class ProxyMcRequest : public McRequestWithContext<ProxyRequestContext> {
 public:
  template<typename... Args>
  explicit ProxyMcRequest(Args&&... args)
    : McRequestWithContext<ProxyRequestContext>(
    std::forward<Args>(args)...) {}
  /* implicit */ ProxyMcRequest(
  McRequestWithContext<ProxyRequestContext> req)
    : McRequestWithContext<ProxyRequestContext>(std::move(req)) {}

  ProxyMcRequest clone() const;
  void setRequestClass(RequestClass type) {
    reqClass_ = type;
  }
  RequestClass getRequestClass() const {
    return reqClass_;
  }
  folly::StringPiece getRequestClassString() const;

 private:
  RequestClass reqClass_{RequestClass::NORMAL};
};

} // mcrouter

template <typename Operation>
struct ReplyType<Operation, mcrouter::ProxyMcRequest> {
  typedef mcrouter::ProxyMcReply type;
};

}}  // facebook::memcache
