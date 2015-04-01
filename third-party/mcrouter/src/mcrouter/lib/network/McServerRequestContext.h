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

#include <utility>

#include "mcrouter/lib/McReply.h"
#include "mcrouter/lib/McRequest.h"

namespace facebook { namespace memcache {

template <class OnRequest>
class McServerOnRequestWrapper;
class McServerSession;
class MultiOpParent;

/**
 * API for users of McServer to send back a reply for a request.
 *
 * Each onRequest callback is provided a context object,
 * which must eventually be surrendered back via a reply() call.
 */
class McServerRequestContext {
 public:
  /**
   * Notify the server that the request-reply exchange is complete.
   *
   * @param ctx  The original context.
   * @param msg  Reply to hand off.
   */
  static void reply(McServerRequestContext&& ctx, McReply&& reply);

  ~McServerRequestContext();

  McServerRequestContext(McServerRequestContext&& other) noexcept;
  McServerRequestContext& operator=(McServerRequestContext&& other);

  /**
   * Get the associated McServerSession
   */
  McServerSession& session();

 private:
  McServerSession* session_;

  /* Pack these together, operation + flags takes one word */
  mc_op_t operation_;
  bool noReply_;
  bool replied_{false};

  uint64_t reqid_;
  struct AsciiState {
    std::shared_ptr<MultiOpParent> parent_;
    folly::Optional<folly::IOBuf> key_;
  };
  std::unique_ptr<AsciiState> asciiState_;

  bool noReply(const McReply& reply) const;

  folly::Optional<folly::IOBuf>& asciiKey() {
    if (!asciiState_) {
      asciiState_ = folly::make_unique<AsciiState>();
    }
    return asciiState_->key_;
  }
  bool hasParent() const {
    return asciiState_ && asciiState_->parent_;
  }
  MultiOpParent& parent() const {
    assert(hasParent());
    return *asciiState_->parent_;
  }

  McServerRequestContext(const McServerRequestContext&) = delete;
  const McServerRequestContext& operator=(const McServerRequestContext&)
    = delete;

  /* Only McServerSession can create these */
  friend class McServerSession;
  friend class MultiOpParent;
  friend class WriteBuffer;
  McServerRequestContext(McServerSession& s, mc_op_t op, uint64_t r,
                         bool nr = false,
                         std::shared_ptr<MultiOpParent> parent = nullptr);
};

/**
 * OnRequest callback interface. This is an implementation detail.
 */
class McServerOnRequest {
public:
  virtual ~McServerOnRequest() {}

private:
  McServerOnRequest() {}
  virtual void requestReady(McServerRequestContext&& ctx,
                            McRequest&& req,
                            mc_op_t operation) = 0;

  friend class McServerSession;
  template <class OnRequest>
  friend class McServerOnRequestWrapper;
};

/**
 * Helper class to wrap user-defined callbacks in a correct virtual interface.
 * This is needed since we're mixing templates and virtual functions.
 */
template <class OnRequest>
class McServerOnRequestWrapper : public McServerOnRequest {
 public:
  template <typename... Args>
  explicit McServerOnRequestWrapper(Args&&... args)
      : onRequest_(std::forward<Args>(args)...) {
  }

 private:
  OnRequest onRequest_;

  void requestReady(McServerRequestContext&& ctx, McRequest&& req,
                    mc_op_t operation);
  friend class McServerSession;
};

}}  // facebook::memcache

#include "McServerRequestContext-inl.h"
