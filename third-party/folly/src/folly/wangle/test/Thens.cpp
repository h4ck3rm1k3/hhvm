// This file is @generated by thens.rb

#include "Thens.h"

TEST(Future, thenVariants) {
  SomeClass anObject;
  Executor* anExecutor;

  {Future<B> f = someFuture<A>().then(aFunction<Future<B>, Try<A>&&>);}
  {Future<B> f = someFuture<A>().then(&SomeClass::aStaticMethod<Future<B>, Try<A>&&>);}
  {Future<B> f = someFuture<A>().then(&anObject, &SomeClass::aMethod<Future<B>, Try<A>&&>);}
  {Future<B> f = someFuture<A>().then(aStdFunction<Future<B>, Try<A>&&>());}
  {Future<B> f = someFuture<A>().then([&](Try<A>&&){return someFuture<B>();});}
  {Future<B> f = someFuture<A>().then(aFunction<B, Try<A>&&>);}
  {Future<B> f = someFuture<A>().then(&SomeClass::aStaticMethod<B, Try<A>&&>);}
  {Future<B> f = someFuture<A>().then(&anObject, &SomeClass::aMethod<B, Try<A>&&>);}
  {Future<B> f = someFuture<A>().then(aStdFunction<B, Try<A>&&>());}
  {Future<B> f = someFuture<A>().then([&](Try<A>&&){return B();});}
}
