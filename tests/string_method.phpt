--TEST--
String conversion test
--SKIPIF--
<?php if(!extension_loaded("objectifier")) print "skip"; ?>
--FILE--
<?php
class Objectifier_String {
  protected $str;

  public function __construct($str) {
    $this->str = (string)$str;
  }
  public function length() {
    return strlen($this->str);
  }
  public function upper() {
    return strtoupper($this->str);
  }
  public function __toString() {
    return $this->str;
  }
}
objectifier_register(function($str) { return new Objectifier_String($str); });
$s = "Hello World";
var_dump($s);
var_dump($s . "!");
var_dump($s->length());
var_dump($s->upper());
var_dump((string)$s);
--EXPECT--
string(11) "Hello World"
string(12) "Hello World!"
int(11)
string(11) "HELLO WORLD"
string(11) "Hello World"
