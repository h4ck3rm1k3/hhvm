<?hh // strict
/**
 * Copyright (c) 2014, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the "hack" directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 */

class X {
  public ?int $x;

  public function foo(): int {
    $obj = new X();
    if($obj->x) {
      return $obj->x;
    }
    return 0;
  }
}
