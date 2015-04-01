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

namespace proxygen {

class TTLBAStats {
 public:
  virtual ~TTLBAStats() noexcept {}

  virtual void recordTTLBAExceedLimit() noexcept = 0;
  virtual void recordTTLBAIOBSplitByEom() noexcept = 0;
  virtual void recordTTLBANotFound() noexcept = 0;
  virtual void recordTTLBAReceived() noexcept = 0;
  virtual void recordTTLBATimeout() noexcept = 0;
  virtual void recordTTLBAEomPassed() noexcept = 0;
  virtual void recordTTLBATracked() noexcept = 0;
};

} // namespace proxygen
