<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" "http://www.w3.org/TR/REC-html40/loose.dtd">
<?php
  define(ICEWM_STABLE,	'1.2.3');
  define(ICEWM_DEVEL, 	'1.2.3');
  define(ICEWM_CHANGES,	'1.2.3');

  define(STABLE_DATE,	'2002-06-30');
  define(STABLE_ID,	97354);

  define(PREFIX,	'/home/groups/i/ic/icewm/');
  define(LIBDIR,	PREFIX.'libphp/');
  define(DOCDIR,	PREFIX.'htdocs/');
  define(CHANGELOG,	PREFIX.'cvs/icewm-1.2/CHANGES');
  define(BUGTRACKER,	'http://sourceforge.net/tracker/index.php?'.
			'func=detail&group_id=31&atid=100031');


  define(REQUEST,$REQUEST_URI[0] != '.' ? DOCDIR.$REQUEST_URI : DOCDIR);
  define(ICEWM_UPDATE, strftime('%x %X',max(filemtime(ICEWM_SCRIPT),
                                        max(filemtime(LIBDIR.'head.php'),
					    filemtime(LIBDIR.'tail.php')))));

  function nicenum($num) {
    return strlen($num) > 3 ?
      nicenum(substr($num, 0, strlen($num) - 3)).'.'.
      substr($num, strlen($num) - 3) : $num;
  }

  function cvsref($label, $file) {
    echo '<a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/icewm/'.
    	 "$file\">$label</a>";
  }

  function fileref($name, $extension) {
    echo '<a href="http://ftp1.sourceforge.net/icewm/'.
    	 "$name-".ICEWM_STABLE.".$extension\">".
    	 "$name-".ICEWM_STABLE.".$extension</a>";
  }

  $firstBlock=1;
 
  function beginBlock($title=0, $anchor=0) {
    global $firstBlock;

?>
  <table width="90%" align=center bgcolor="#666666"
   cellpadding=2 cellspacing=10>
   <tr><td width="100%">
    <table width="100%" bgcolor="#e0e0d0" cellpadding=8 cellspacing=0>
     <tr>
<?php if($title): ?>
      <th bgcolor="#444488">
       <a name=<?php echo $anchor ?>><font color="#ffffff">
	<?php echo $title?>
       </font></a>
      </th>
     </tr><tr>
<?php endif ?>
      <td>
<?php if($firstBlock): ?>
       <!--a href="http://sourceforge.net/"><img
        src="/cgi-bin/sflogo.sh" align=right width=31 height=88 border=0
        alt="Hosted by Sourceforge"></a-->
<?php $firstBlock=0; endif ?>
<?php
  }

  function endBlock() {
?>
     </td></tr>
    </table>
   </td></tr>
  </table>
<?php
  }

  function changes($stop = 0) {
    $clog = fopen(CHANGELOG, 'r');
    $rel = 0;
  
    while($clog && !feof($clog)) {
      $line = fgets($clog, 4096);
    
      if ($line[0] >= '0' &&$line[0] <= '9') {	// print release marker
        if ($rel) {
          echo "\n</ul>\n";
	  if ($rel == $stop) return;
	}
      
	echo "<li>$line<ul>";
	$rel = substr($line, 0, strpos($line, ':'));
      } else {					// print changed item
        $line = preg_replace('/#(\d+)/',
                             '<a href="'.BUGTRACKER.'&aid=\1">#\1</a>',
                             trim($line));

        if ($line[0] == '-')
          echo "\n<li>".substr($line,2);
        else if ($line[0] == "!")
          echo "\n<li><em>In progress: </em>".
	       substr($line,strpos($line,'-') + 1);
        else
          echo " $line";
      }
    }

    fclose($clog);
  }
?>

<html>
 <head>
  <title>[IceWM-Devel] <?php echo ICEWM_TITLE ?></title>
  <style type="text/css">
   a:link		{ color:#00f; text-decoration: underline } 
   a:visited		{ color:#008; text-decoration: underline }
   a:active		{ color:#f00; text-decoration: underline }
   a:hover		{ color:#f00; text-decoration: underline }
   
   a.header:link	{ color:#ddd; text-decoration: none } 
   a.header:visited	{ color:#ddd; text-decoration: none }
   a.header:active	{ color:#fff; text-decoration: underline }
   a.header:hover	{ color:#fff; text-decoration: underline }

   a.margin:link	{ color:#008; text-decoration: underline } 
   a.margin:visited	{ color:#008; text-decoration: underline }
   a.margin:active	{ color:#f00; text-decoration: underline }
   a.margin:hover	{ color:#f00; text-decoration: underline }
  </style>
 </head>

 <body bgcolor="#00aaff" text="#000000" 
  link="#0000ff" alink="#ff0000" vlink="#000080" marginwidth=0 marginheight=0>
  <table width="100%" bgcolor="#666666" border=0 cellpadding=0 cellspacing=0>
   <tr valign=top>
    <td><small>
     &nbsp;
     <a class=header href="http://www.icewm.org/">icewm.org</a>
     <a class=header href="http://icewm.themes.org/">themes.org</a>
    </td><td align=right><small>
     <a class=header href="http://linux.com/">linux.com</a>
     <a class=header href="http://openprojects.net/">openprojects.net</a>
     <a class=header href="http://sourceforge.net/">sourceforge.net</a>
     &nbsp;
    </td>
   </tr>
  </table>

  <br>

  <div align=right>
<a href="http://sourceforge.net"><img src="http://sourceforge.net/sflogo.php?group_id=31&amp;type=1" width="88" height="31" border="0" alt="SourceForge.net Logo"></a>
  </div>
  <div align=center><small>
   <a href="/"><img
     src="/icewm.png" width=360 height=80 border=0 alt="IceWM"></a><br>
   [ <a class=margin href="/#about">About</a>
   | <a class=margin href="/#devel">Download</a>
   | <a class=margin href="/#changes">Changelog</a>
   | <a class=margin href="/#mail">Mailing&nbsp;List</a>
   | <a class=margin href="/#irc">IRC</a>
   | <a class=margin href="/#links">Links</a>
   <!--| <a class=margin href="/preview">Previews</a-->
   | <a class=margin href="/nls.php">NLS</a> ]<br></small>
  </div><br>

<?php beginBlock() ?>
