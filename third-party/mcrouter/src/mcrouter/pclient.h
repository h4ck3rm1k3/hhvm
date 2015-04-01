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
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "mcrouter/TkoTracker.h"

namespace facebook { namespace memcache { namespace mcrouter {

class ProxyClientOwner;
class ProxyDestination;
class TkoCounters;

/**
 * ProxyDestinations from multiple proxy threads can share this storage.
 * If pdstn->shared is not null, it points to valid storage area
 * that's shared for all the clients with the same destination_key
 */
struct ProxyClientShared {
  /// "host:port" uniquely identifying this shared object
  std::string key;
  TkoTracker tko;

  ProxyClientShared(const std::string& key_,
                    const size_t tkoThreshold,
                    const size_t maxSoftTkos,
                    TkoCounters& globalTkos,
                    ProxyClientOwner& owner);

  /**
   * Should be called only under ProxyClientOwner::mx lock, e.g. from
   * foreach_shared_synchronized
   */
  const std::unordered_set<ProxyDestination*>& getDestinations() const {
    return pdstns_;
  }

  void removeDestination(ProxyDestination* pdstn);

  ~ProxyClientShared();

 private:
  ProxyClientOwner& owner_;
  /// ProxyDestinations that reference this shared object
  std::unordered_set<ProxyDestination*> pdstns_;

  friend class ProxyClientOwner;
};

/**
 * Manages the lifetime of proxy clients and their shared areas.
 */
struct ProxyClientOwner {
  /**
   * Creates/updates ProxyClientShared with the given pdstn
   * and also updates pdstn->shared pointer.
   */
  void updateProxyClientShared(ProxyDestination& pdstn,
                               const size_t tkoThreshold,
                               const size_t maxSoftTkos,
                               TkoCounters& globalTkos);
  /**
   * Calls func(key, ProxyClientShared*) for each live proxy client
   * shared object.  The whole map will be locked for the duration of the call.
   */
  template<typename F>
  void foreach_shared_synchronized(const F& func);

  std::weak_ptr<ProxyClientShared> getSharedByKey(const std::string& key);

 private:
  std::mutex mx;
  std::unordered_map<std::string, std::weak_ptr<ProxyClientShared>>
    pclient_shared;

  friend class ProxyClientShared;
};

}}}

#include "pclient-inl.h"
