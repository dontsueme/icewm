<?php
  define(ICEWM_TITLE,  'PreviewArea');
  define(ICEWM_SCRIPT, $SCRIPT_FILENAME);

  include('../../libphp/head.php');
?>
     
<?php if (is_dir(REQUEST) && ($dir = opendir (REQUEST))): ?>
	  <table width="100%" border=0>
	    <tr>
              <th bgcolor="#444488"><font color="#ffffff">Filename</font></th>
              <th bgcolor="#444488"><font color="#ffffff">Last modification</font></th>
              <th bgcolor="#444488"><font color="#ffffff">Size</font></th>
	    </tr>
<?php
    while ($file = readdir ($dir))
      if ($file [0] != '.' && $file != 'index.php')
        echo '	    <tr>'.
	  '<td><a href="'.$file.'">'.$file.'</a></td>'.
	  '<td align=right>'.strftime('%x %X',filemtime($file)).'</td>'.
	  '<td align=right>'.nicenum(filesize($file)).'&nbsp;Byte</td>'.
	  "</tr>\n";
    closedir ($dir);
?>
	  </table>
<?php else:?>Sorry, but currently there a no preview releases.<?php endif;?>    


	  <hr noshade size=2 color="#cccccc">

          <div align=center>
	    [ <?php cvsref(ChangeLog, 'icewm-1.0/icewm-1.0/CHANGES') ?> |
	      ReleaseCriterias | RoadMap ]
	  </div>

<?php include('../../libphp/tail.php') ?>
