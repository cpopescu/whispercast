<?php

// we need directory "base" to contain the "rpc through php" base files
// usually located in ".../net/rpc/php"
include_once "base/rpc_base.php";

// files generated by rpc_parser
include_once "auto/types.php";
include_once "auto/wrappers.php";

InitializeLog(LOG_LEVEL_INFO);

$url = "http://localhost:8123/rpc";
$rpc_connection = new RpcHttpConnection($url);

// This function only purpose is to test float ~equality.
function MyEqual($a, $b) {
  if ( is_float($a) && is_float($b) ) {
    return abs($a - $b) < 0.00001;
  }
  if ( is_scalar($a) || is_null($a) || 
       is_scalar($b) || is_null($b) ) {
    return $a == $b; 
  }
  $a_keys = array();
  foreach($a as $k => $v) {
    array_push($a_keys, $k);
  }
  $b_keys = array();
  foreach($b as $k => $v) {
    array_push($b_keys, $k);
  }
  if ( count($a_keys) != count($b_keys) ) {
    return false;
  }
  for($i = 0; $i < count($a_keys); $i++){
    $ak = $a_keys[$i];
    $bk = $b_keys[$i];
    if ( !MyEqual($ak, $bk) ) {
      return false;
    }
    $av = $a->$ak;
    $bv = $b->$bk;
    if ( !MyEqual($av, $bv) ) {
      return false;
    }
  }
  return true;
}

function TEST_SUCCESS($call_description, $expected_result, $call_status, $file, $line) {
  if ( !$call_status->success_ ) {
    LOG_ERROR("$file:$line FAIL $call_description => RPC failed, error: $call_status->error_");
    exit();
    return;
  }
  $result_type = is_object($call_status->result_) ? get_class($call_status->result_) : gettype($call_status->result_);
  $expected_type = is_object($expected_result) ? get_class($expected_result) : gettype($expected_result);
  if ( $result_type !== $expected_type ) {
    LOG_ERROR("$file:$line FAIL $call_description => returned type: $result_type , differs from expected type: $expected_type");
    exit();
    return;
  }
  if ( !MyEqual($call_status->result_, $expected_result ) ) {
    LOG_ERROR("$file:$line FAIL $call_description => $call_status->result_ , differs from expected result: $expected_result");
    exit();
    return;
  }
  LOG_INFO("$file:$line PASS $call_description => $call_status->result_");
}

$service1 = new RpcServiceWrapperService1($rpc_connection, "service1");
$call_status = $service1->MirrorVoid();
TEST_SUCCESS("service1->MirrorVoid()", NULL, $call_status, __FILE__, __LINE__);
$call_status = $service1->MirrorBool(true);
TEST_SUCCESS("service1->MirrorBool(true)", true, $call_status, __FILE__, __LINE__);
$call_status = $service1->MirrorFloat(3.14);
TEST_SUCCESS("service1->MirrorFloat(3.14)", 3.14, $call_status, __FILE__, __LINE__);
$call_status = $service1->MirrorInt(7);
TEST_SUCCESS("service1->MirrorInt(7)", 7, $call_status, __FILE__, __LINE__);
$call_status = $service1->MirrorBigInt(2147483648);
TEST_SUCCESS("service1->MirrorBigInt(2147483648)", 2147483648, $call_status, __FILE__, __LINE__);
$call_status = $service1->MirrorBigInt(214748364812);
TEST_SUCCESS("service1->MirrorBigInt(214748364812)", 214748364812, $call_status, __FILE__, __LINE__);
var_dump($call_status->result_);
$call_status = $service1->MirrorString("abcdef");
TEST_SUCCESS("service1->MirrorString('abcdef')", "abcdef", $call_status, __FILE__, __LINE__);

?>
