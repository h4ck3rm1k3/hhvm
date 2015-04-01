/*
 *  Copyright (c) 2015, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "McSerializedRequest.h"

#include "mcrouter/lib/McMsgRef.h"
#include "mcrouter/lib/McRequest.h"

namespace facebook { namespace memcache {

McSerializedRequest::~McSerializedRequest() {
  switch (protocol_) {
    case mc_ascii_protocol:
      asciiRequest_.~AsciiSerializedRequest();
      break;
    case mc_umbrella_protocol:
      umbrellaMessage_.~UmbrellaSerializedMessage();
      break;
    case mc_unknown_protocol:
    case mc_binary_protocol:
    case mc_nprotocols:
      break;
  }
}

McSerializedRequest::Result McSerializedRequest::serializationResult() const {
  return result_;
}

void McSerializedRequest::serializeMcMsgAscii(const McMsgRef& req) {
  size_t hdrLength = mc_ascii_req_max_hdr_length(req.get());

  asciiRequest_.asciiBuffer = std::unique_ptr<char[]>(new char[hdrLength]);

  int r = mc_serialize_req_ascii(req.get(), asciiRequest_.asciiBuffer.get(),
                                 hdrLength, asciiRequest_.iovs, kMaxIovs);

  if (r <= 0) {
    result_ = Result::ERROR;
  } else {
    iovsBegin_ = asciiRequest_.iovs;
    iovsCount_ = r;
  }
}

}} // facebook::memcache
