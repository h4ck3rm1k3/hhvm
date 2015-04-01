/*
 *  Copyright (c) 2015, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <atomic>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <folly/Benchmark.h>
#include <folly/Memory.h>

#include "mcrouter/lib/fibers/AddTasks.h"
#include "mcrouter/lib/fibers/EventBaseLoopController.h"
#include "mcrouter/lib/fibers/FiberManager.h"
#include "mcrouter/lib/fibers/GenericBaton.h"
#include "mcrouter/lib/fibers/SimpleLoopController.h"
#include "mcrouter/lib/fibers/WhenN.h"

using namespace facebook::memcache;
using namespace facebook::memcache::mcrouter;

using folly::Try;

TEST(FiberManager, batonTimedWaitTimeout) {
  bool taskAdded = false;
  size_t iterations = 0;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
  if (!taskAdded) {
      manager.addTask(
        [&]() {
          Baton baton;

          auto res = baton.timed_wait(std::chrono::milliseconds(230));

          EXPECT_FALSE(res);
          EXPECT_EQ(5, iterations);

          loopController.stop();
        }
      );
      manager.addTask(
        [&]() {
          Baton baton;

          auto res = baton.timed_wait(std::chrono::milliseconds(130));

          EXPECT_FALSE(res);
          EXPECT_EQ(3, iterations);

          loopController.stop();
        }
      );
      taskAdded = true;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      iterations ++;
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, batonTimedWaitPost) {
  bool taskAdded = false;
  size_t iterations = 0;
  Baton* baton_ptr;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          Baton baton;
          baton_ptr = &baton;

          auto res = baton.timed_wait(std::chrono::milliseconds(130));

          EXPECT_TRUE(res);
          EXPECT_EQ(2, iterations);

          loopController.stop();
        }
      );
      taskAdded = true;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      iterations ++;
      if (iterations == 2) {
        baton_ptr->post();
      }
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, batonTimedWaitTimeoutEvb) {
  size_t tasksComplete = 0;

  folly::EventBase evb;

  FiberManager manager(folly::make_unique<EventBaseLoopController>());
  dynamic_cast<EventBaseLoopController&>(
    manager.loopController()).attachEventBase(evb);

  auto task = [&](size_t timeout_ms) {
    Baton baton;

    auto start = EventBaseLoopController::Clock::now();
    auto res = baton.timed_wait(std::chrono::milliseconds(timeout_ms));
    auto finish = EventBaseLoopController::Clock::now();

    EXPECT_FALSE(res);

    auto duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);

    EXPECT_GT(duration_ms.count(), timeout_ms - 50);
    EXPECT_LT(duration_ms.count(), timeout_ms + 50);

    if (++tasksComplete == 2) {
      evb.terminateLoopSoon();
    }
  };

  evb.runInEventBaseThread([&]() {
    manager.addTask(
      [&]() {
        task(500);
      }
    );
    manager.addTask(
      [&]() {
        task(250);
      }
    );
  });

  evb.loopForever();

  EXPECT_EQ(2, tasksComplete);
}

TEST(FiberManager, batonTimedWaitPostEvb) {
  size_t tasksComplete = 0;

  folly::EventBase evb;

  FiberManager manager(folly::make_unique<EventBaseLoopController>());
  dynamic_cast<EventBaseLoopController&>(
    manager.loopController()).attachEventBase(evb);

  evb.runInEventBaseThread([&]() {
      manager.addTask([&]() {
          Baton baton;

          evb.runAfterDelay([&]() {
              baton.post();
            },
            100);

          auto start = EventBaseLoopController::Clock::now();
          auto res = baton.timed_wait(std::chrono::milliseconds(130));
          auto finish = EventBaseLoopController::Clock::now();

          EXPECT_TRUE(res);

          auto duration_ms = std::chrono::duration_cast<
            std::chrono::milliseconds>(finish - start);

          EXPECT_TRUE(duration_ms.count() > 95 &&
                      duration_ms.count() < 110);

          if (++tasksComplete == 1) {
            evb.terminateLoopSoon();
          }
        });
    });

  evb.loopForever();

  EXPECT_EQ(1, tasksComplete);
}

TEST(FiberManager, batonTryWait) {

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  // Check if try_wait and post work as expected
  Baton b;

  manager.addTask([&](){
    while (!b.try_wait()) {
    }
  });
  auto thr = std::thread([&](){
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    b.post();
  });

  manager.loopUntilNoReady();
  thr.join();

  Baton c;

  // Check try_wait without post
  manager.addTask([&](){
    int cnt = 100;
    while (cnt && !c.try_wait()) {
      cnt--;
    }
    EXPECT_TRUE(!c.try_wait()); // must still hold
    EXPECT_EQ(cnt, 0);
  });

  manager.loopUntilNoReady();
}

TEST(FiberManager, genericBatonFiberWait) {
  FiberManager manager(folly::make_unique<SimpleLoopController>());

  GenericBaton b;
  bool fiberRunning = false;

  manager.addTask([&](){
    EXPECT_EQ(manager.hasActiveFiber(), true);
    fiberRunning = true;
    b.wait();
    fiberRunning = false;
  });

  EXPECT_FALSE(fiberRunning);
  manager.loopUntilNoReady();
  EXPECT_TRUE(fiberRunning); // ensure fiber still active

  auto thr = std::thread([&](){
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    b.post();
  });

  while (fiberRunning) {
    manager.loopUntilNoReady();
  }

  thr.join();
}

TEST(FiberManager, genericBatonThreadWait) {
  FiberManager manager(folly::make_unique<SimpleLoopController>());
  GenericBaton b;
  std::atomic<bool> threadWaiting(false);

  auto thr = std::thread([&](){
    threadWaiting = true;
    b.wait();
    threadWaiting = false;
  });

  while (!threadWaiting) {}
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  manager.addTask([&](){
    EXPECT_EQ(manager.hasActiveFiber(), true);
    EXPECT_TRUE(threadWaiting);
    b.post();
    while(threadWaiting) {}
  });

  manager.loopUntilNoReady();
  thr.join();
}

TEST(FiberManager, addTasksNoncopyable) {
  std::vector<FiberPromise<int>> pendingFibers;
  bool taskAdded = false;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          std::vector<std::function<std::unique_ptr<int>()>> funcs;
          for (size_t i = 0; i < 3; ++i) {
            funcs.push_back(
              [i, &pendingFibers]() {
                fiber::await([&pendingFibers](FiberPromise<int> promise) {
                    pendingFibers.push_back(std::move(promise));
                  });
                return folly::make_unique<int>(i*2 + 1);
              }
            );
          }

          auto iter = fiber::addTasks(funcs.begin(), funcs.end());

          size_t n = 0;
          while (iter.hasNext()) {
            auto result = iter.awaitNext();
            EXPECT_EQ(2 * iter.getTaskID() + 1, *result);
            EXPECT_GE(2 - n, pendingFibers.size());
            ++n;
          }
          EXPECT_EQ(3, n);
        }
      );
      taskAdded = true;
    } else if (pendingFibers.size()) {
      pendingFibers.back().setValue(0);
      pendingFibers.pop_back();
    } else {
      loopController.stop();
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, addTasksThrow) {
  std::vector<FiberPromise<int>> pendingFibers;
  bool taskAdded = false;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          std::vector<std::function<int()>> funcs;
          for (size_t i = 0; i < 3; ++i) {
            funcs.push_back(
              [i, &pendingFibers]() {
                fiber::await([&pendingFibers](FiberPromise<int> promise) {
                    pendingFibers.push_back(std::move(promise));
                  });
                if (i % 2 == 0) {
                  throw std::runtime_error("Runtime");
                }
                return i*2 + 1;
              }
            );
          }

          auto iter = fiber::addTasks(funcs.begin(), funcs.end());

          size_t n = 0;
          while (iter.hasNext()) {
            try {
              int result = iter.awaitNext();
              EXPECT_EQ(1, iter.getTaskID() % 2);
              EXPECT_EQ(2 * iter.getTaskID() + 1, result);
            } catch (...) {
              EXPECT_EQ(0, iter.getTaskID() % 2);
            }
            EXPECT_GE(2 - n, pendingFibers.size());
            ++n;
          }
          EXPECT_EQ(3, n);
        }
      );
      taskAdded = true;
    } else if (pendingFibers.size()) {
      pendingFibers.back().setValue(0);
      pendingFibers.pop_back();
    } else {
      loopController.stop();
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, addTasksVoid) {
  std::vector<FiberPromise<int>> pendingFibers;
  bool taskAdded = false;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          std::vector<std::function<void()>> funcs;
          for (size_t i = 0; i < 3; ++i) {
            funcs.push_back(
              [i, &pendingFibers]() {
                fiber::await([&pendingFibers](FiberPromise<int> promise) {
                    pendingFibers.push_back(std::move(promise));
                  });
              }
            );
          }

          auto iter = fiber::addTasks(funcs.begin(), funcs.end());

          size_t n = 0;
          while (iter.hasNext()) {
            iter.awaitNext();
            EXPECT_GE(2 - n, pendingFibers.size());
            ++n;
          }
          EXPECT_EQ(3, n);
        }
      );
      taskAdded = true;
    } else if (pendingFibers.size()) {
      pendingFibers.back().setValue(0);
      pendingFibers.pop_back();
    } else {
      loopController.stop();
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, addTasksVoidThrow) {
  std::vector<FiberPromise<int>> pendingFibers;
  bool taskAdded = false;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          std::vector<std::function<void()>> funcs;
          for (size_t i = 0; i < 3; ++i) {
            funcs.push_back(
              [i, &pendingFibers]() {
                fiber::await([&pendingFibers](FiberPromise<int> promise) {
                    pendingFibers.push_back(std::move(promise));
                  });
                if (i % 2 == 0) {
                  throw std::runtime_error("");
                }
              }
            );
          }

          auto iter = fiber::addTasks(funcs.begin(), funcs.end());

          size_t n = 0;
          while (iter.hasNext()) {
            try {
              iter.awaitNext();
              EXPECT_EQ(1, iter.getTaskID() % 2);
            } catch (...) {
              EXPECT_EQ(0, iter.getTaskID() % 2);
            }
            EXPECT_GE(2 - n, pendingFibers.size());
            ++n;
          }
          EXPECT_EQ(3, n);
        }
      );
      taskAdded = true;
    } else if (pendingFibers.size()) {
      pendingFibers.back().setValue(0);
      pendingFibers.pop_back();
    } else {
      loopController.stop();
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, reserve) {
  std::vector<FiberPromise<int>> pendingFibers;
  bool taskAdded = false;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          std::vector<std::function<void()>> funcs;
          for (size_t i = 0; i < 3; ++i) {
            funcs.push_back(
              [&pendingFibers]() {
                fiber::await([&pendingFibers](FiberPromise<int> promise) {
                    pendingFibers.push_back(std::move(promise));
                  });
              }
            );
          }

          auto iter = fiber::addTasks(funcs.begin(), funcs.end());

          iter.reserve(2);
          EXPECT_TRUE(iter.hasCompleted());
          EXPECT_TRUE(iter.hasPending());
          EXPECT_TRUE(iter.hasNext());

          iter.awaitNext();
          EXPECT_TRUE(iter.hasCompleted());
          EXPECT_TRUE(iter.hasPending());
          EXPECT_TRUE(iter.hasNext());

          iter.awaitNext();
          EXPECT_FALSE(iter.hasCompleted());
          EXPECT_TRUE(iter.hasPending());
          EXPECT_TRUE(iter.hasNext());

          iter.awaitNext();
          EXPECT_FALSE(iter.hasCompleted());
          EXPECT_FALSE(iter.hasPending());
          EXPECT_FALSE(iter.hasNext());
        }
      );
      taskAdded = true;
    } else if (pendingFibers.size()) {
      pendingFibers.back().setValue(0);
      pendingFibers.pop_back();
    } else {
      loopController.stop();
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, forEach) {
  std::vector<FiberPromise<int>> pendingFibers;
  bool taskAdded = false;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          std::vector<std::function<int()>> funcs;
          for (size_t i = 0; i < 3; ++i) {
            funcs.push_back(
              [i, &pendingFibers]() {
                fiber::await([&pendingFibers](FiberPromise<int> promise) {
                    pendingFibers.push_back(std::move(promise));
                  });
                return i * 2 + 1;
              }
            );
          }

          std::vector<std::pair<size_t, int>> results;
          fiber::forEach(funcs.begin(), funcs.end(),
            [&results](size_t id, int result) {
              results.push_back(std::make_pair(id, result));
            });
          EXPECT_EQ(3, results.size());
          EXPECT_TRUE(pendingFibers.empty());
          for (size_t i = 0; i < 3; ++i) {
            EXPECT_EQ(results[i].first * 2 + 1, results[i].second);
          }
        }
      );
      taskAdded = true;
    } else if (pendingFibers.size()) {
      pendingFibers.back().setValue(0);
      pendingFibers.pop_back();
    } else {
      loopController.stop();
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, whenN) {
  std::vector<FiberPromise<int>> pendingFibers;
  bool taskAdded = false;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          std::vector<std::function<int()>> funcs;
          for (size_t i = 0; i < 3; ++i) {
            funcs.push_back(
              [i, &pendingFibers]() {
                fiber::await([&pendingFibers](FiberPromise<int> promise) {
                    pendingFibers.push_back(std::move(promise));
                  });
                return i*2 + 1;
              }
            );
          }

          auto results = fiber::whenN(funcs.begin(), funcs.end(), 2);
          EXPECT_EQ(2, results.size());
          EXPECT_EQ(1, pendingFibers.size());
          for (size_t i = 0; i < 2; ++i) {
            EXPECT_EQ(results[i].first*2 + 1, results[i].second);
          }
        }
      );
      taskAdded = true;
    } else if (pendingFibers.size()) {
      pendingFibers.back().setValue(0);
      pendingFibers.pop_back();
    } else {
      loopController.stop();
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, whenNThrow) {
  std::vector<FiberPromise<int>> pendingFibers;
  bool taskAdded = false;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          std::vector<std::function<int()>> funcs;
          for (size_t i = 0; i < 3; ++i) {
            funcs.push_back(
              [i, &pendingFibers]() {
                fiber::await([&pendingFibers](FiberPromise<int> promise) {
                    pendingFibers.push_back(std::move(promise));
                  });
                throw std::runtime_error("Runtime");
                return i*2+1;
              }
            );
          }

          try {
            fiber::whenN(funcs.begin(), funcs.end(), 2);
          } catch (...) {
            EXPECT_EQ(1, pendingFibers.size());
          }
        }
      );
      taskAdded = true;
    } else if (pendingFibers.size()) {
      pendingFibers.back().setValue(0);
      pendingFibers.pop_back();
    } else {
      loopController.stop();
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, whenNVoid) {
  std::vector<FiberPromise<int>> pendingFibers;
  bool taskAdded = false;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          std::vector<std::function<void()>> funcs;
          for (size_t i = 0; i < 3; ++i) {
            funcs.push_back(
              [i, &pendingFibers]() {
                fiber::await([&pendingFibers](FiberPromise<int> promise) {
                    pendingFibers.push_back(std::move(promise));
                  });
              }
            );
          }

          auto results = fiber::whenN(funcs.begin(), funcs.end(), 2);
          EXPECT_EQ(2, results.size());
          EXPECT_EQ(1, pendingFibers.size());
        }
      );
      taskAdded = true;
    } else if (pendingFibers.size()) {
      pendingFibers.back().setValue(0);
      pendingFibers.pop_back();
    } else {
      loopController.stop();
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, whenNVoidThrow) {
  std::vector<FiberPromise<int>> pendingFibers;
  bool taskAdded = false;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          std::vector<std::function<void()>> funcs;
          for (size_t i = 0; i < 3; ++i) {
            funcs.push_back(
              [i, &pendingFibers]() {
                fiber::await([&pendingFibers](FiberPromise<int> promise) {
                    pendingFibers.push_back(std::move(promise));
                  });
                throw std::runtime_error("Runtime");
              }
            );
          }

          try {
            fiber::whenN(funcs.begin(), funcs.end(), 2);
          } catch (...) {
            EXPECT_EQ(1, pendingFibers.size());
          }
        }
      );
      taskAdded = true;
    } else if (pendingFibers.size()) {
      pendingFibers.back().setValue(0);
      pendingFibers.pop_back();
    } else {
      loopController.stop();
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, whenAll) {
  std::vector<FiberPromise<int>> pendingFibers;
  bool taskAdded = false;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          std::vector<std::function<int()>> funcs;
          for (size_t i = 0; i < 3; ++i) {
            funcs.push_back(
              [i, &pendingFibers]() {
                fiber::await([&pendingFibers](FiberPromise<int> promise) {
                    pendingFibers.push_back(std::move(promise));
                  });
                return i*2+1;
              }
            );
          }

          auto results = fiber::whenAll(funcs.begin(), funcs.end());
          EXPECT_TRUE(pendingFibers.empty());
          for (size_t i = 0; i < 3; ++i) {
            EXPECT_EQ(i*2+1, results[i]);
          }
        }
      );
      taskAdded = true;
    } else if (pendingFibers.size()) {
      pendingFibers.back().setValue(0);
      pendingFibers.pop_back();
    } else {
      loopController.stop();
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, whenAllVoid) {
  std::vector<FiberPromise<int>> pendingFibers;
  bool taskAdded = false;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          std::vector<std::function<void()>> funcs;
          for (size_t i = 0; i < 3; ++ i) {
            funcs.push_back(
              [i, &pendingFibers]() {
                fiber::await([&pendingFibers](FiberPromise<int> promise) {
                    pendingFibers.push_back(std::move(promise));
                  });
              }
            );
          }

          fiber::whenAll(funcs.begin(), funcs.end());
          EXPECT_TRUE(pendingFibers.empty());
        }
      );
      taskAdded = true;
    } else if (pendingFibers.size()) {
      pendingFibers.back().setValue(0);
      pendingFibers.pop_back();
    } else {
      loopController.stop();
    }
  };

  loopController.loop(std::move(loopFunc));
}

TEST(FiberManager, whenAny) {
  std::vector<FiberPromise<int>> pendingFibers;
  bool taskAdded = false;

  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  auto loopFunc = [&]() {
    if (!taskAdded) {
      manager.addTask(
        [&]() {
          std::vector<std::function<int()> > funcs;
          for (size_t i = 0; i < 3; ++ i) {
            funcs.push_back(
              [i, &pendingFibers]() {
                fiber::await([&pendingFibers](FiberPromise<int> promise) {
                    pendingFibers.push_back(std::move(promise));
                  });
                if (i == 1) {
                  throw std::runtime_error("This exception will be ignored");
                }
                return i*2+1;
              }
            );
          }

          auto result = fiber::whenAny(funcs.begin(), funcs.end());
          EXPECT_EQ(2, pendingFibers.size());
          EXPECT_EQ(2, result.first);
          EXPECT_EQ(2*2+1, result.second);
        }
      );
      taskAdded = true;
    } else if (pendingFibers.size()) {
      pendingFibers.back().setValue(0);
      pendingFibers.pop_back();
    } else {
      loopController.stop();
    }
  };

  loopController.loop(std::move(loopFunc));
}

namespace {
/* Checks that this function was run from a main context,
   by comparing an address on a stack to a known main stack address
   and a known related fiber stack address.  The assumption
   is that fiber stack and main stack will be far enough apart,
   while any two values on the same stack will be close. */
void expectMainContext(bool& ran, int* mainLocation, int* fiberLocation) {
  int here;
  /* 2 pages is a good guess */
  constexpr size_t DISTANCE = 0x2000 / sizeof(int);
  if (fiberLocation) {
    EXPECT_TRUE(std::abs(&here - fiberLocation) > DISTANCE);
  }
  if (mainLocation) {
    EXPECT_TRUE(std::abs(&here - mainLocation) < DISTANCE);
  }

  EXPECT_FALSE(ran);
  ran = true;
}
}

TEST(FiberManager, runInMainContext) {
  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  bool checkRan = false;

  int mainLocation;
  manager.runInMainContext(
    [&]() {
      expectMainContext(checkRan, &mainLocation, nullptr);
    });
  EXPECT_TRUE(checkRan);

  checkRan = false;

  manager.addTask(
    [&]() {
      int stackLocation;
      fiber::runInMainContext(
        [&]() {
          expectMainContext(checkRan, &mainLocation, &stackLocation);
        });
      EXPECT_TRUE(checkRan);
    }
  );

  loopController.loop(
    [&]() {
      loopController.stop();
    }
  );

  EXPECT_TRUE(checkRan);
}

TEST(FiberManager, addTaskFinally) {
  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  bool checkRan = false;

  int mainLocation;

  manager.addTaskFinally(
    [&]() {
      return 1234;
    },
    [&](Try<int>&& result) {
      EXPECT_EQ(result.value(), 1234);

      expectMainContext(checkRan, &mainLocation, nullptr);
    }
  );

  EXPECT_FALSE(checkRan);

  loopController.loop(
    [&]() {
      loopController.stop();
    }
  );

  EXPECT_TRUE(checkRan);
}

TEST(FiberManager, fibersPoolWithinLimit) {
  FiberManager::Options opts;
  opts.maxFibersPoolSize = 5;

  FiberManager manager(folly::make_unique<SimpleLoopController>(), opts);
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  size_t fibersRun = 0;

  for (size_t i = 0; i < 5; ++i) {
    manager.addTask(
      [&]() {
        ++fibersRun;
      }
    );
  }
  loopController.loop(
    [&]() {
      loopController.stop();
    }
  );

  EXPECT_EQ(5, fibersRun);
  EXPECT_EQ(5, manager.fibersAllocated());
  EXPECT_EQ(5, manager.fibersPoolSize());

  for (size_t i = 0; i < 5; ++i) {
    manager.addTask(
      [&]() {
        ++fibersRun;
      }
    );
  }
  loopController.loop(
    [&]() {
      loopController.stop();
    }
  );

  EXPECT_EQ(10, fibersRun);
  EXPECT_EQ(5, manager.fibersAllocated());
  EXPECT_EQ(5, manager.fibersPoolSize());
}

TEST(FiberManager, fibersPoolOverLimit) {
  FiberManager::Options opts;
  opts.maxFibersPoolSize = 5;

  FiberManager manager(folly::make_unique<SimpleLoopController>(), opts);
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  size_t fibersRun = 0;

  for (size_t i = 0; i < 10; ++i) {
    manager.addTask(
      [&]() {
        ++fibersRun;
      }
    );
  }

  EXPECT_EQ(0, fibersRun);
  EXPECT_EQ(10, manager.fibersAllocated());
  EXPECT_EQ(0, manager.fibersPoolSize());

  loopController.loop(
    [&]() {
      loopController.stop();
    }
  );

  EXPECT_EQ(10, fibersRun);
  EXPECT_EQ(5, manager.fibersAllocated());
  EXPECT_EQ(5, manager.fibersPoolSize());
}

TEST(FiberManager, remoteFiberBasic) {
  FiberManager manager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(manager.loopController());

  int result[2];
  result[0] = result[1] = 0;
  folly::Optional<FiberPromise<int>> savedPromise[2];
  manager.addTask(
    [&] () {
      result[0] = fiber::await([&] (FiberPromise<int> promise) {
          savedPromise[0] = std::move(promise);
        });
    });
  manager.addTask(
    [&] () {
      result[1] = fiber::await([&] (FiberPromise<int> promise) {
          savedPromise[1] = std::move(promise);
        });
    });

  manager.loopUntilNoReady();

  EXPECT_TRUE(savedPromise[0].hasValue());
  EXPECT_TRUE(savedPromise[1].hasValue());
  EXPECT_EQ(0, result[0]);
  EXPECT_EQ(0, result[1]);

  std::thread remoteThread0{
    [&] () {
      savedPromise[0]->setValue(42);
    }
  };
  std::thread remoteThread1{
    [&] () {
      savedPromise[1]->setValue(43);
    }
  };
  remoteThread0.join();
  remoteThread1.join();
  EXPECT_EQ(0, result[0]);
  EXPECT_EQ(0, result[1]);
  /* Should only have scheduled once */
  EXPECT_EQ(1, loopController.remoteScheduleCalled());

  manager.loopUntilNoReady();
  EXPECT_EQ(42, result[0]);
  EXPECT_EQ(43, result[1]);
}

TEST(FiberManager, addTaskRemoteBasic) {
  FiberManager manager(folly::make_unique<SimpleLoopController>());

  int result[2];
  result[0] = result[1] = 0;
  folly::Optional<FiberPromise<int>> savedPromise[2];

  std::thread remoteThread0{
    [&] () {
      manager.addTaskRemote(
        [&] () {
          result[0] = fiber::await([&] (FiberPromise<int> promise) {
              savedPromise[0] = std::move(promise);
            });
        });
    }
  };
  std::thread remoteThread1{
    [&] () {
      manager.addTaskRemote(
        [&] () {
          result[1] = fiber::await([&] (FiberPromise<int> promise) {
              savedPromise[1] = std::move(promise);
            });
        });
    }
  };
  remoteThread0.join();
  remoteThread1.join();

  manager.loopUntilNoReady();

  EXPECT_TRUE(savedPromise[0].hasValue());
  EXPECT_TRUE(savedPromise[1].hasValue());
  EXPECT_EQ(0, result[0]);
  EXPECT_EQ(0, result[1]);

  savedPromise[0]->setValue(42);
  savedPromise[1]->setValue(43);

  EXPECT_EQ(0, result[0]);
  EXPECT_EQ(0, result[1]);

  manager.loopUntilNoReady();
  EXPECT_EQ(42, result[0]);
  EXPECT_EQ(43, result[1]);
}

TEST(FiberManager, remoteHasTasks) {
  size_t counter = 0;
  FiberManager fm(folly::make_unique<SimpleLoopController>());
  std::thread remote([&]() {
    fm.addTaskRemote([&]() {
      ++counter;
    });
  });

  remote.join();

  while (fm.hasTasks()) {
    fm.loopUntilNoReady();
  }

  EXPECT_FALSE(fm.hasTasks());
  EXPECT_EQ(counter, 1);
}

TEST(FiberManager, remoteHasReadyTasks) {
  int result = 0;
  folly::Optional<FiberPromise<int>> savedPromise;
  FiberManager fm(folly::make_unique<SimpleLoopController>());
  std::thread remote([&]() {
    fm.addTaskRemote([&]() {
      result = fiber::await([&](FiberPromise<int> promise) {
        savedPromise = std::move(promise);
      });
      EXPECT_TRUE(fm.hasTasks());
    });
  });

  remote.join();
  EXPECT_TRUE(fm.hasTasks());

  fm.loopUntilNoReady();
  EXPECT_TRUE(fm.hasTasks());

  std::thread remote2([&](){
    savedPromise->setValue(47);
  });
  remote2.join();
  EXPECT_TRUE(fm.hasTasks());

  fm.loopUntilNoReady();
  EXPECT_FALSE(fm.hasTasks());

  EXPECT_EQ(result, 47);
}

static size_t sNumAwaits;

void runBenchmark(size_t numAwaits, size_t toSend) {
  sNumAwaits = numAwaits;

  FiberManager fiberManager(folly::make_unique<SimpleLoopController>());
  auto& loopController =
    dynamic_cast<SimpleLoopController&>(fiberManager.loopController());

  std::queue<FiberPromise<int>> pendingRequests;
  static const size_t maxOutstanding = 5;

  auto loop = [&fiberManager, &loopController, &pendingRequests, &toSend]() {
    if (pendingRequests.size() == maxOutstanding || toSend == 0) {
      if (pendingRequests.empty()) {
        return;
      }
      pendingRequests.front().setValue(0);
      pendingRequests.pop();
    } else {
      fiberManager.addTask([&pendingRequests]() {
          for (size_t i = 0; i < sNumAwaits; ++i) {
            auto result = fiber::await(
              [&pendingRequests](FiberPromise<int> promise) {
                pendingRequests.push(std::move(promise));
              });
            assert(result == 0);
          }
        });

      if (--toSend == 0) {
        loopController.stop();
      }
    }
  };

  loopController.loop(std::move(loop));
}

BENCHMARK(FiberManagerBasicOneAwait, iters) {
  runBenchmark(1, iters);
}

BENCHMARK(FiberManagerBasicFiveAwaits, iters) {
  runBenchmark(5, iters);
}
