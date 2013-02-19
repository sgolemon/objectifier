--TEST--
Ensure object method calls still function
--SKIPIF--
<?php if(!extension_loaded("objectifier")) print "skip"; ?>
--FILE--
<?php
class foo {
  public function __construct($arg1, $arg2) {
    var_dump($arg1, $arg2);
  }
  public function bar($arg3) {
    var_dump($arg3);
  }
}
$f = new foo(123, "hello");
$f->bar(3.14);
--EXPECT--
int(123)
string(5) "hello"
float(3.14)
