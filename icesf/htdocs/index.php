<?php
  define(ICEWM_TITLE,  'DevelZone');
  define(ICEWM_SCRIPT, $SCRIPT_FILENAME);

  include('../libphp/head.php');
?>
       <p>
	IceWM is a window manager for the <a href="http://www.xfree86.org"
	>X11 Window System</a>. It it has been coded from scratch in C++
	for performance and size and attempts to achieve the following goals:
       <ul>
	<li><b>Feel good and fast to use, be simple and don't get in the way</b>
	<li>Default configuration should be fully usable without tweaking
	<li>Mouse is optional
	<li>Combine the best features of other window managers and GUIs
	<li>Themes can customize the look, user can customize the feel.
       </ul><p>
	IceWM is release under the terms of the <a href="COPYING.LIB"
	><b>GNU Library General Public License</b></a>.
<?php endBlock() ?>

<!--
<P>
IceWM allows reasonably configurable look using 
<A HREF="http://icewm.themes.org">themes</A> and is fully <A
HREF="http://www.gnome.org/">GNOME</A> and partially <A
HREF="http://www.kde.org/">KDE</A> compliant, but can also be used as a
full featured standalone environment.
<P>
A small screenshot displaying the default look is <A HREF="screenshot.jpg">here</A>.
-->

<?php beginBlock() ?>
      <p>
	Information valueable for users of IceWM can be found at <a
	href="http://www.icewm.org/"><b>www.icewm.org</b></a>. Themes are
	available at <a href="http://icewm.themes.org">icewm.themes.org</a>.
	This site mainly is intended for people interested in developing
	IceWM.
      </p><p>
	If you find an unfixed bug file it into the <a
	href="http://sourceforge.net/tracker/?group_id=31&atid=100031"
	>bug-tracking system</a>, please. When you have implemented an
	enhancement it is a good idea to tell the <a
	href="http://sourceforge.net/tracker/?group_id=31&atid=300031">patch
	manager</a> about it. Submissions are forwarded to the development
	mailinglist automatically.
      </p>
<?php endBlock() ?>
<?php beginBlock('Stable Release: '.ICEWM_STABLE.' ('.STABLE_DATE.')<br>'.
	         '<small>Unstable Release: '.ICEWM_DEVEL.'</small>', devel) ?>
       <table width="100%">
        <tr>
	 <th align=left>Source code</th>
	 <th align=left>Mirrors</th>
	</tr><tr>
	 <td valign=top>
	  <ul>
	   <li><?php fileref('icewm','tar.gz')?>
	  </ul><br>
	 </td><td valign=top rowspan=2>
	  <ul>
	   <li><a href="http://sourceforge.net/project/showfiles.php?group_id=31&release_id=<?php echo STABLE_ID ?>">Sourceforge</a>
	   <li><a href="http://rpm.digitalprojects.com/">rpm.digitalprojects.com</a>
	   <li><a href="http://www.maol.yi.org/icewm/download/">maol.yi.org</a>
	  </ul>
	 </td>
	</tr><tr>
	 <th align=left>Binaries</th>
	</tr><tr>
	 <td valign=top>
	  <ul>
	   <li><?php fileref('icewm','i386.rpm')?>
	   <li><?php fileref('icewm-themes','i386.rpm')?>
	   <li><?php fileref('icewm-l10n','i386.rpm')?>
	   <li><?php fileref('icewm-menu-gnome1','i386.rpm')?>
	  </ul><br>
	 </td>
	</tr><tr>
	 <th align=left colspan=2>CVS Repository</th>
	</tr><tr>
	 <td colspan=2>
	  <ul>
	   <li>CVSROOT=:pserver:anonymous@cvs.icewm.sourceforge.net:/cvsroot/icewm
	   <li>WebCVS: <?php cvsref('IceWM 1.2', 'icewm-1.2') ?> (stable tree)
	   <!--li>WebCVS: <?php cvsref('IceWM 1.1', 'icewm') ?> (experimental tree)-->
	  </ul>
	 </td>
	</tr>
       </table>
<?php endBlock() ?>
<?php beginBlock('Changelog', changes) ?>
<?php changes(ICEWM_CHANGES) ?>
       <p>
	View the file <a href="changes.php">CHANGES</a> of the CVS repository
	for information about previous and future versions.
<?php endBlock() ?>
<?php beginBlock('Mailing Lists', mail) ?>
       <p>
	A mailing list has been set up for the development of IceWM.
       <p>
	There are two ways to subscribe to a group: via e-mail or via
	the web on the group's home page.
       <p>
	To subscribe via e-mail: 
       <ul>
	<li>Send an e-mail to: <a
	 href="mailto:icewm-devel-request@lists.sourceforge.net"
	 >icewm-devel-request@lists.sourceforge.net</a> and include the
	 word "subscribe" in the subject or body.
       <p>
	To subscribe via the group's home page: 
       <ol>
	<li>Go to the group home page: <a
	 href="http://lists.sourceforge.net/mailman/listinfo/icewm-devel"
	 >http://lists.sourceforge.net/mailman/listinfo/icewm-devel</a>
	<li>Click on the "Subscribe" button and follow the instructions to
	 complete subscription. You can choose to receive messages via
	 e-mail (in individual or daily digest format) or only by accesing
	 them via the web on the group's home page.
       <p>
	Other groups are icewm-user and icewm-announce, the procedure to
	subscribe is the same as listed above.
<?php endBlock() ?>
<?php beginBlock('IRC', irc) ?>
	The official IRC channel is #icewm at <a
	href="http://www.freenode.info/">freenode</a>.
<?php endBlock() ?>
<!--
 <TR>
  <TD WIDTH="100%">
   <TABLE WIDTH="100%" BGCOLOR="#CCCCCC" CELLPADDING=2 CELLSPACING=0>
    <TR>
     <TH BGCOLOR="#444488">
      <A NAME="links"><FONT COLOR="#FFFFFF">Links</FONT></A>
     </TH>
    </TR>
    <TR>
     <TD BGCOLOR="#E0E0D0">
-->     
<!--TABLE BORDER=0 CELLSPACING=0 WIDTH="100%" BGCOLOR="#0000CC"><TR><TD ALIGN=CENTER>
<A NAME="links"><FONT COLOR="#FFFFFF"><B>Links</B></FONT></A>
</TD></TR></TABLE--><!--
<UL>
    <li><a href="http://www.icewm.org/">www.icewm.org</a> - the <b>NEW</b> and complete IceWM site
    <LI><A HREF="http://icewm.themes.org/">icewm.themes.org</A> site part of <A HREF="http://themes.org/">themes.org</A>
    <LI><A HREF="http://icewmfaq.cjb.net/">Icewm FAQ site</A> on <A HREF="http://icewm.cjb.net">icewm.cjb.net</A>
    <LI><A HREF="http://www.plig.org/xwinman/">Window Managers for X</A> (<B>you can now vote for icewm</B>)
    <LI><A HREF="http://members.xoom.com/SaintChoj/icepref.html">IcePref</A> - icewm configuration utility (using PyGtk).
    <LI><A HREF="http://c54820-a.carneg1.pa.home.com/programs/index.html">Convert fvwm2 menus to icewm menus</A>
    <LI><A HREF="http://wuzwuz.ucg.ie/~lale/python/kde2ice.html">KDE to icewm menu converter</A>
  
    <LI>Desktop Environments
    <UL>
        <LI><A HREF="http://www.gnome.org/">GNOME</A>
        <LI><A HREF="http://www.kde.org/">KDE</A>
    </UL>
    <LI><A HREF="http://dfm.online.de">Desktop File Manager</A>
    <LI><A HREF="http://freshmeat.net">freshmeat.net</A>
</UL>
-->
<?php include('../libphp/tail.php') ?>
