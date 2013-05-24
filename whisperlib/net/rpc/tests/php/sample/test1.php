<?php

include "base/rpc_log.php";

class Abc {
  public function Hello() {
    echo "Abc::Hello \n";
  }
}

$s = "Abc1";
$p = new $s;
$p->Hello();

?>
