<?php
  define(ICEWM_TITLE,  'ChangeLog');
  define(ICEWM_SCRIPT, $SCRIPT_FILENAME);
  define(CHANGELOG,    '../cvs/icewm-1.2/CHANGES');
  define(STOP_CHANGES, '1.2.0');
  define(BUGTRACKER,	'http://sourceforge.net/tracker/index.php?'.
			'func=detail&group_id=31&atid=100031');
    
  include('../libphp/head.php');

?>

<h2>Change Log</h2>

<?php
  function changes($stop = 0) {
    $clog = fopen(CHANGELOG, 'r');
    $rel = 0;
    $have_item = 0;
  
    echo "\n<ul>\n";
    while($clog && !feof($clog)) {
      $line = fgets($clog, 4096);
    
      if ($line[0] >= '0' && $line[0] <= '9') {	// print release marker
        if ($have_item) { echo "\n  </li>";  $have_item = 0; }
        if ($rel) {
          echo "\n  </ul>";
          echo "\n  </li>\n";
	  if ($rel == $stop)
            break;
	}
      
	echo "  <li>$line  <ul>";
	$rel = substr($line, 0, strpos($line, ':'));
      } else {					// print changed item
        $line = preg_replace('/#(\d+)/',
                             '<a href="'.BUGTRACKER.'&aid=\1">#\1</a>',
                             trim($line));
        $line = preg_replace('&', '&amp;', $line);
        $line = preg_replace('<', '&lt;', $line);

        if ($line[0] == '-') {
          if ($have_item) { echo "\n  </li>";  $have_item = 0; }
          echo "\n    <li>".substr($line,2);
        } else if ($line[0] == "!") {
          if ($have_item) { echo "\n  </li>";  $have_item = 0; }
          echo "\n    <li><em>In progress: </em>".
	       substr($line, strpos($line, '-') + 1);
        } else
          echo " $line";
        $have_item = 1;
      }
    }
    if ($have_item) { echo "\n  </li>";  $have_item = 0; }
    echo "\n</ul>\n";
    fclose($clog);
  }

  changes(STOP_CHANGES);
?>

<?php
  include('../libphp/tail.php');
?>
