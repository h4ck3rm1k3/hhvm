/*
 *  Copyright (c) 2015, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
namespace facebook { namespace memcache {

template<int Op>
McSerializedRequest::McSerializedRequest(const McRequest& req,
                                         McOperation<Op>, size_t reqId,
                                         mc_protocol_t protocol)
  : protocol_(protocol) {

  switch (protocol_) {
    case mc_ascii_protocol:
      new (&asciiRequest_) AsciiSerializedRequest();
      if (req.key().length() > MC_KEY_MAX_LEN_ASCII) {
        result_ = Result::BAD_KEY;
        return;
      }
      serializeMcMsgAscii(req.dependentMsg((mc_op_t)Op));
      break;
    case mc_umbrella_protocol:
      new (&umbrellaMessage_) UmbrellaSerializedMessage();
      if (!umbrellaMessage_.prepare(req, McOperation<Op>(), reqId, iovsBegin_,
                                    iovsCount_)) {
        result_ = Result::ERROR;
      }
      break;
    case mc_unknown_protocol:
    case mc_binary_protocol:
    case mc_nprotocols:
      checkLogic(false, "Used unsupported protocol! Value: {}", (int)protocol_);
      result_ = Result::ERROR;
      iovsCount_ = 0;
  }

}

}} // facebook::memcache
