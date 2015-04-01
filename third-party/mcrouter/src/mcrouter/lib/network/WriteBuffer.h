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

#include <folly/Optional.h>
#include <folly/ThreadLocal.h>

#include "mcrouter/lib/mc/ascii_response.h"
#include "mcrouter/lib/mc/protocol.h"
#include "mcrouter/lib/mc/umbrella.h"
#include "mcrouter/lib/McReply.h"
#include "mcrouter/lib/network/UniqueIntrusiveList.h"
#include "mcrouter/lib/network/McServerRequestContext.h"
#include "mcrouter/lib/network/UmbrellaProtocol.h"

namespace facebook { namespace memcache {

class McServerSession;

class AsciiSerializedReply {
 public:
  AsciiSerializedReply();
  ~AsciiSerializedReply();
  void clear();
  bool prepare(const McReply& reply,
               mc_op_t operation,
               const folly::Optional<folly::IOBuf>& key,
               struct iovec*& iovOut, size_t& niovOut);

 private:
  static const size_t kMaxIovs = 16;
  struct iovec iovs_[kMaxIovs];
  mc_ascii_response_buf_t asciiResponse_;

  AsciiSerializedReply(const AsciiSerializedReply&) = delete;
  AsciiSerializedReply& operator=(const AsciiSerializedReply&) = delete;
  AsciiSerializedReply(AsciiSerializedReply&&) noexcept = delete;
  AsciiSerializedReply& operator=(AsciiSerializedReply&&) = delete;
};

class WriteBuffer {
 private:
  UniqueIntrusiveListHook hook_;

 public:
  using Queue = UniqueIntrusiveList<WriteBuffer,
                                    &WriteBuffer::hook_>;

  explicit WriteBuffer(mc_protocol_t protocol);
  ~WriteBuffer();

  /**
   * Allows using this buffer again without doing a complete
   * re-initialization
   */
  void clear();

  /**
   * If successful, iovOut/niovOut will on return point to an array of iovs
   * contained within this struct which will contain a serialized
   * representation of the given reply.
   *
   * @return true On success
   */
  bool prepare(McServerRequestContext&& ctx, McReply&& reply,
               struct iovec*& iovOut, size_t& niovOut);

 private:
  const mc_protocol_t protocol_;

  /* Write buffers */
  union {
    AsciiSerializedReply asciiReply_;
    UmbrellaSerializedMessage umbrellaReply_;
  };

  folly::Optional<McServerRequestContext> ctx_;
  folly::Optional<McReply> reply_;

  WriteBuffer(const WriteBuffer&) = delete;
  WriteBuffer& operator=(const WriteBuffer&) = delete;
  WriteBuffer(WriteBuffer&&) noexcept = delete;
  WriteBuffer& operator=(WriteBuffer&&) = delete;
};

class WriteBufferQueue {
 public:
  explicit WriteBufferQueue(mc_protocol_t protocol)
      : protocol_(protocol) {
    if (protocol_ != mc_ascii_protocol &&
        protocol_ != mc_umbrella_protocol) {
      throw std::runtime_error("Invalid protocol");
    }
  }

  WriteBuffer& push() {
    auto& freeQ = freeQueue();
    if (freeQ.empty()) {
      return queue_.pushBack(folly::make_unique<WriteBuffer>(protocol_));
    } else {
      return queue_.pushBack(freeQ.popFront());
    }
  }

  void pop() {
    auto& freeQ = freeQueue();
    if (freeQ.size() < kMaxFreeQueueSz) {
      auto& wb = freeQ.pushBack(queue_.popFront());
      wb.clear();
    } else {
      queue_.popFront();
    }
  }

  bool empty() {
    return queue_.empty();
  }

 private:
  mc_protocol_t protocol_;
  WriteBuffer::Queue& freeQueue() {
    static folly::ThreadLocal<WriteBuffer::Queue> freeQ[mc_nprotocols];
    assert((size_t)protocol_ < mc_nprotocols);
    return *freeQ[(size_t)protocol_];
  }
  WriteBuffer::Queue queue_;
  constexpr static size_t kMaxFreeQueueSz = 50;

  WriteBufferQueue(const WriteBufferQueue&) = delete;
  WriteBufferQueue& operator=(const WriteBufferQueue&) = delete;
  WriteBufferQueue(WriteBufferQueue&&) noexcept = delete;
  WriteBufferQueue& operator=(WriteBufferQueue&&) = delete;
};

}}  // facebook::memcache
