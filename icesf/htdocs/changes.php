<?php
  define(ICEWM_TITLE,  'ChangeLog');
  define(ICEWM_SCRIPT, $SCRIPT_FILENAME);
    
  include('../libphp/head.php');
  changes($stop);
  include('../libphp/tail.php');
?>
