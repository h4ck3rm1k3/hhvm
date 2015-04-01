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

#include <memory>
#include <string>
#include <vector>

namespace facebook { namespace memcache {

class McrouterOptions;

namespace mcrouter {

class McrouterRouteHandleIf;
class ProxyClientCommon;
class ProxyRoute;
class ServiceInfo;

class ProxyConfigIf {
 public:
  virtual std::string getConfigMd5Digest() const = 0;

  virtual ProxyRoute& proxyRoute() const = 0;

  virtual std::shared_ptr<ServiceInfo> serviceInfo() const = 0;

  virtual const std::vector<std::shared_ptr<const ProxyClientCommon>>&
  getClients() const = 0;

  virtual std::shared_ptr<McrouterRouteHandleIf>
  getRouteHandleForAsyncLog(const std::string& asynclogName) const = 0;

  virtual ~ProxyConfigIf() {}
};

}}}  // facebook::memcache::mcrouter
