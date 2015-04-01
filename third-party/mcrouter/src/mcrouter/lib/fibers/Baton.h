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

#include <atomic>

#include <folly/detail/Futex.h>

#include "mcrouter/lib/fibers/TimeoutController.h"

namespace facebook { namespace memcache {

class Fiber;

/**
 * @class Baton
 *
 * Primitive which allows to put current Fiber to sleep and wake it from another
 * Fiber/thread.
 */
class Baton {
 public:
  Baton();

  ~Baton() {}

  /**
   * Puts active fiber to sleep. Returns when post is called.
   */
  void wait();

  /**
   * Puts active fiber to sleep. Returns when post is called.
   *
   * @param mainContextFunc this function is immediately executed on the main
   *        context.
   */
  template <typename F>
  void wait(F&& mainContextFunc);

  /**
   * This is here only not break tao/locks. Please don't use it, because it is
   * inefficient when used on Fibers.
   */
  template<typename C, typename D = typename C::duration>
  bool timed_wait(const std::chrono::time_point<C,D>& timeout);

  /**
   * Puts active fiber to sleep. Returns when post is called.
   *
   * @param timeout Baton will be automatically awaken if timeout is hit
   *
   * @return true if was posted, false if timeout expired
   */
  bool timed_wait(TimeoutController::Duration timeout);

  /**
   * Puts active fiber to sleep. Returns when post is called.
   *
   * @param timeout Baton will be automatically awaken if timeout is hit
   * @param mainContextFunc this function is immediately executed on the main
   *        context.
   *
   * @return true if was posted, false if timeout expired
   */
  template <typename F>
  bool timed_wait(TimeoutController::Duration timeout, F&& mainContextFunc);

  /**
   * Checks if the baton has been posted without blocking.
   * @return    true iff the baton has been posted.
   */
  bool try_wait();

  /**
   * Wakes up Fiber which was waiting on this Baton (or if no Fiber is waiting,
   * next wait() call will return immediately).
   */
  void post();

 private:
  enum {
    /**
     * Must be positive.  If multiple threads are actively using a
     * higher-level data structure that uses batons internally, it is
     * likely that the post() and wait() calls happen almost at the same
     * time.  In this state, we lose big 50% of the time if the wait goes
     * to sleep immediately.  On circa-2013 devbox hardware it costs about
     * 7 usec to FUTEX_WAIT and then be awoken (half the t/iter as the
     * posix_sem_pingpong test in BatonTests).  We can improve our chances
     * of early post by spinning for a bit, although we have to balance
     * this against the loss if we end up sleeping any way.  Spins on this
     * hw take about 7 nanos (all but 0.5 nanos is the pause instruction).
     * We give ourself 300 spins, which is about 2 usec of waiting.  As a
     * partial consolation, since we are using the pause instruction we
     * are giving a speed boost to the colocated hyperthread.
     */
    PreBlockAttempts = 300,
  };

  explicit Baton(intptr_t state) : waitingFiber_(state) {};

  void postHelper(intptr_t new_value);
  void postThread();
  void waitThread();
  /**
   * Spin for "some time" (see discussion on PreBlockAttempts) waiting
   * for a post.
   * @return true if we received a post the spin wait, false otherwise. If the
   *         function returns true then Baton state is guaranteed to be POSTED
   */
  bool spinWaitForEarlyPost();

  bool timedWaitThread(TimeoutController::Duration timeout);

  static constexpr intptr_t NO_WAITER = 0;
  static constexpr intptr_t POSTED = -1;
  static constexpr intptr_t TIMEOUT = -2;
  static constexpr intptr_t THREAD_WAITING = -3;

  union {
    std::atomic<intptr_t> waitingFiber_;
    struct {
      folly::detail::Futex<> futex;
      int32_t _unused_packing;
    } futex_;
  };
};

}}

#include "mcrouter/lib/fibers/Baton-inl.h"
