<?php

class A {
}

namespace B {
  class B {
  }
}

function test() {
  new A;
}

function test_namespace() {
  new B\A;
}
