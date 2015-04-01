/*
 *  Copyright (c) 2015, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "util.h"

#include <assert.h>
#include <utime.h>
#include <pthread.h>

#include <random>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <openssl/md5.h>

#include <folly/FileUtil.h>
#include <folly/json.h>
#include <folly/Random.h>
#include <folly/ScopeGuard.h>
#include <folly/SpookyHashV2.h>

namespace facebook { namespace memcache {

std::string randomString(size_t minLen, size_t maxLen,
                         folly::StringPiece range) {

  assert(minLen <= maxLen);
  assert(!range.empty());

  static const int seed = folly::randomNumberSeed();
  typedef std::mt19937 RandomT;
  static RandomT rng(seed);
  std::uniform_int_distribution<size_t> lenRange(minLen, maxLen);
  std::uniform_int_distribution<size_t> charRange(0, range.size() - 1);

  std::string result;
  result.resize(lenRange(rng));

  for (char& c : result) {
    c = range[charRange(rng)];
  }
  return result;
}

uint32_t getMemcacheKeyHashValue(folly::StringPiece key) {
  return
    folly::hash::SpookyHashV2::Hash32(key.begin(), key.size(), /* seed= */ 0);
}

bool determineIfSampleKeyForViolet(uint32_t routingKeyHash,
                                   uint32_t sample_period) {
  assert(sample_period > 0);
  const uint32_t m = std::numeric_limits<uint32_t>::max();
  uint32_t keyHashMax = m/sample_period;

  return routingKeyHash <= keyHashMax;
}

std::string Md5Hash(folly::StringPiece input) {
  unsigned char result[MD5_DIGEST_LENGTH];
  MD5(reinterpret_cast<const unsigned char*>(input.data()), input.size(),
      result);

  std::string ret;
  const std::string kHexBytes = "0123456789abcdef";
  for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
    // Hex should print, for this, in a weird bytewise order:
    // If the bytes are b0b1b2b3..., we print:
    //       hi(b0) lo(b0) hi(b1) lo(b1) ...
    // Where hi(x) is the hex char of its upper 4 bits, and lo(x) is lower 4
    ret += kHexBytes[(result[i] >> 4) & 0x0F];
    ret += kHexBytes[result[i] & 0x0F];
  }

  return ret;
}

namespace {
bool writeToFile(folly::StringPiece contents, const std::string& path,
                 int flags) {
  int fd = folly::openNoInt(path.data(), flags, 0664);
  if (fd == -1) {
    return false;
  }
  auto written = folly::writeFull(fd, contents.data(), contents.size());
  if (folly::closeNoInt(fd) != 0) {
    return false;
  }
  return written >= 0 && size_t(written) == contents.size();
}
}

bool writeStringToFile(folly::StringPiece contents, const std::string& path) {
  return writeToFile(contents, path, O_CREAT | O_WRONLY | O_TRUNC);
}

bool appendStringToFile(folly::StringPiece contents, const std::string& path) {
  return writeToFile(contents, path, O_CREAT | O_WRONLY | O_APPEND);
}

bool atomicallyWriteFileToDisk(folly::StringPiece contents,
                               const std::string& absFilename) {
  boost::filesystem::path tempFilePath;
  auto tempFileGuard = folly::makeGuard([&tempFilePath]() {
    if (!tempFilePath.empty()) {
      boost::system::error_code ec;
      boost::filesystem::remove(tempFilePath.c_str(), ec);
    }
  });

  try {
    const boost::filesystem::path filePath(absFilename);
    auto fileDir = filePath.parent_path();
    if (fileDir.empty()) {
      return false;
    }
    auto tempFileTempl = filePath.filename().string() + ".temp-%%%%%%%%%%";
    tempFilePath = fileDir / boost::filesystem::unique_path(tempFileTempl);

    boost::filesystem::create_directories(fileDir);

    if (!writeStringToFile(contents, tempFilePath.string())) {
      return false;
    }

    boost::filesystem::rename(tempFilePath, filePath);
    return true;
  } catch (const boost::filesystem::filesystem_error& e) {
    return false;
  } catch (const boost::system::system_error& e) {
    return false;
  }
}

bool touchFile(const std::string& path) {
  struct stat fileStats;
  if (stat(path.data(), &fileStats)) {
    if (!writeStringToFile("", path)) {
      return false;
    }
  }
  return utime(path.data(), nullptr) == 0;
}

// one day we should move it to folly/ThreadName.h
std::string getThreadName() {

#if defined(__GLIBC__) && !defined(__APPLE__)
#if __GLIBC_PREREQ(2, 12)

  char threadName[32];
  if (pthread_getname_np(pthread_self(), threadName, sizeof(threadName)) == 0) {
    return threadName;
  }

#endif
#endif

  return "unknown";
}

folly::dynamic parseJsonString(folly::StringPiece s,
                               bool allow_trailing_comma) {
  folly::json::serialization_opts opts;
  opts.allow_trailing_comma = allow_trailing_comma;
  return folly::parseJson(s, opts);
}

}} // facebook::memcache
