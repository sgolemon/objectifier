--TEST--
String conversion test
--SKIPIF--
<?php if(!extension_loaded("objectifier")) print "skip"; ?>
--FILE--
<?php
class Objectifier_Stream {
  protected $stream;

  public function __construct($stream) {
    $this->stream = $stream;
  }
  public function fgets() {
    return fgets($this->stream);
  }
}
objectifier_register(function($res) {
  if (is_resource($res) && get_resource_type($res) == "stream") {
    return new Objectifier_Stream($res);
  }
  return $res;
});
$fp = fopen("data:text/plain,This is a test%0AThis is only a test.", "r");
var_dump(trim($fp->fgets()));
var_dump($fp->fgets());
--EXPECT--
string(14) "This is a test"
string(20) "This is only a test."
