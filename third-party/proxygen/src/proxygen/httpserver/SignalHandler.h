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

#include <thrift/lib/cpp/async/TAsyncSignalHandler.h>
#include <vector>

namespace proxygen {

class HTTPServer;

/**
 * Installs signal handler which will stop HTTPServer when the user presses
 * Ctrl-C. To be used if HTTPServer is the main process.
 *
 * Note: Should only be created from the thread invoking `HTTPServer::start()`.
 */
class SignalHandler: private apache::thrift::async::TAsyncSignalHandler {
 public:
  explicit SignalHandler(HTTPServer* server);

  void install(const std::vector<int>& signals);
 private:
  // TAsyncSignalHandler
  void signalReceived(int signum) noexcept override;

  HTTPServer* const server_{nullptr};
};

}
