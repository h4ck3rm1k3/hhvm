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

#include <folly/Range.h>

#include "mcrouter/lib/mc/msg.h"
#include "mcrouter/lib/mc/umbrella.h"
#include "mcrouter/lib/McOperation.h"

namespace folly {
class IOBuf;
}

namespace facebook { namespace memcache {

class McReply;
class McRequest;

/**
 * Parse an on-the-wire Umbrella request.
 *
 * @param source           Unchained IOBuf; [body, body + nbody) must point
 *                         inside it.
 * @param header, nheader  [header, header + nheader) must point to a valid
 *                         Umbrella header.
 * @param body, nbody      [body, body + nbody) must point to a valid
 *                         Umbrella body stored in the `source` IOBuf
 * @paramOut opOut         Parsed operation.
 * @paramOut reqidOut      Parsed request ID
 * @return                 Parsed request.
 * @throws                 std::runtime_error on any parse error.
 */
McRequest umbrellaParseRequest(const folly::IOBuf& source,
                               const uint8_t* header, size_t nheader,
                               const uint8_t* body, size_t nbody,
                               mc_op_t& opOut, uint64_t& reqidOut);

class UmbrellaSerializedMessage {
 public:
  UmbrellaSerializedMessage();
  void clear();
  bool prepare(const McReply& reply, mc_op_t op, uint64_t reqid,
               struct iovec*& iovOut, size_t& niovOut);

  template<int Op>
  bool prepare(const McRequest& request, McOperation<Op>, uint64_t reqid,
               struct iovec*& iovOut, size_t& niovOut);

 private:
  static constexpr size_t kMaxIovs = 16;
  struct iovec iovs_[kMaxIovs];

  entry_list_msg_t msg_;
  static constexpr size_t kInlineEntries = 16;
  size_t nEntries_{0};
  um_elist_entry_t entries_[kInlineEntries];

  static constexpr size_t kInlineStrings = 16;
  size_t nStrings_{0};
  folly::StringPiece strings_[kInlineStrings];

  size_t offset_{0};

  bool error_{false};

  void appendInt(entry_type_t type, int32_t tag, uint64_t val);
  void appendDouble(double val);
  void appendString(int32_t tag, const uint8_t* data, size_t len,
                    entry_type_t type = BSTRING);

  /**
   * Put message header and all added entries/strings into iovecs.
   *
   * @return  number of iovecs that contain a complete message.
   */
  size_t finalizeMessage();

  UmbrellaSerializedMessage(const UmbrellaSerializedMessage&) = delete;
  UmbrellaSerializedMessage& operator=(
    const UmbrellaSerializedMessage&) = delete;
  UmbrellaSerializedMessage(UmbrellaSerializedMessage&&) noexcept = delete;
  UmbrellaSerializedMessage& operator=(UmbrellaSerializedMessage&&) = delete;
};

}} // facebook::memcache

#include "UmbrellaProtocol-inl.h"
