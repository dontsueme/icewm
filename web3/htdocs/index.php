<?php
  define(ICEWM_TITLE,  'icewm');
  define(ICEWM_SCRIPT, $SCRIPT_FILENAME);

  include('../libphp/head.php');
?>
<div style="margin-right: 200">
<p> IceWM is a window manager for the <a
href="http://www.xfree86.org/">X11 Window System</a>.
The goal of IceWM is speed, simplicity,
and not getting in the user's way.</p>
</div>
<h2>News<br>
</h2>
<ul>
  <li>Devel version <a href="http://sourceforge.net/project/showfiles.php?group_id=31">1.2.7pre3</a> released (2003-03-02)
  <li>Unofficial Slackware packages provided by David Lechnyr <a href="http://hr.uoregon.edu/davidrl/icewm.html">here</a>.
  <li>Devel version <a href="http://sourceforge.net/project/showfiles.php?group_id=31">1.2.7pre2</a> released (2003-02-26)
  <li>Stable version <a href="icewm-1_2_6.php">1.2.6</a> released
(2003-01-19)<br>
  </li>
</ul>
<h2>Features</h2>
<ul>
  <li>Fully usable with keyboard</li>
  <li>Alt+Tab window switching</li>
  <li>Efficient resource usage</li>
  <li>Task bar (optional)</li>
  <li>Multiple workspaces</li>
  <li>Themes</li>
  <li>Usable with GNOME and KDE environments</li>
</ul>

<h2>Screenshots</h2>

<h2>Themes</h2>
<ul>
  <li><a href="http://themes.freshmeat.net/">themes.freshmeat.net</a>
</ul>
TODO: page with some of the best icewm themes (contrib).

<h2>Translations</h2>

<ul>
  <li><b>Catalan</b> (Toni Cu&ntilde;at i Alario)</li>
  <li><b>Croatian</b> (Vlatko Kosturjak)</li>
  <li><b>Czech</b> (Jan Hor&aacute;k)</li>
  <li><b>English</b></li>
  <li><b>Finnish</b> (Mika Lepp&auml;nen)</li>
  <li><b>French</b> (Laurent Pouillet, Frederic Dubuy)</li>
  <li><b>German</b> (Mathias Hasselmann)</li>
  <li><b>Hungarian</b> (Gabor Suveg)</li>
  <li><b>Italian</b> (Riccardo Murri)</li>
  <li><b>Japanese</b> (Yoichi ASAI)</li>
  <li><b>Lithuanian</b> (Martynas Jocius, Gediminas Paulauskas)</li>
  <li><b>Norwegian</b> (Petter Johan Olsen) </li>
  <li><b>Polish</b> (Przemys&#0322;aw Su&#0322;ek)</li>
  <li><b>Portuguese</b> (pt_BR) (Fernando Brunelli)</li>
  <li><b>Romanian</b> (Tiberiu Micu) </li>
  <li><b>Russian</b> (Alone, Anton Farygin)</li>
  <li><b>Slovenian</b> (Jernej Kovacic) </li>
  <li><b>Spanish</b> (Antonio de la Torre)</li>
  <li><b>Swedish</b> (Per Larsson)</li>
  <li><b>Ukranian</b> (Volodymyr M. Lisivka)</li>
  <li><b>Traditional Chinese - zh_CN.GB2312</b> (Li Wei Jih, Liu Zhuyuan)</li>
  <li><b>Traditional Chinese - zh_TW.Big5</b> (Li Wei Jih)</li>
</ul>

<h2>Documentation</h2>
TODO: link to docs, FAQ here.

<hr size="1"/>
<h2>Bug Tracking</h2>
<p>If you have a patch, a bug report or a feature request to submit,
please do so at the <a
href="http://sourceforge.net/projects/icewm/">icewm project page</a> at
SourceForge.</p>

<h2>Mailing Lists</h2>
<ul>
  <li>icewm-user - icewm user discussion
    <p>
To subscribe to the mailing list send an email to:
<a href="icewm-user-request@lists.sourceforge.net">icewm-user-request@lists.sourceforge.net</a>
with body or subject "subscribe", or visit
<a href="http://lists.sourceforge.net/mailman/listinfo/icewm-user">http://lists.sourceforge.net/mailman/listinfo/icewm-user</a>.
    </p>
    <br/>
  </li>

  <li>icewm-devel - discussion of icewm development</li>
    <p>
To subscribe to the mailing list send an email to:
<a href="icewm-devel-request@lists.sourceforge.net">icewm-devel-request@lists.sourceforge.net</a>
with body or subject "subscribe", or visit
<a href="http://lists.sourceforge.net/mailman/listinfo/icewm-devel">http://lists.sourceforge.net/mailman/listinfo/icewm-devel</a>.
    </p>
    <br/>
  </li>
  <li>Old yahoo/egroups list <a href="http://groups.yahoo.com/group/icewm/messages">archive</a>.
</ul>
<h2>IRC</h2>

<p>The official IRC channel is #icewm at <a
 href="http://www.freenode.info/">freenode</a>.</p>

<h2>CVS</h2>
<ul>
  <li><a href="changes.php">View Change Log</a>
    <br/>
    <br/>
  </li>
  <li>Anonymous CVS<br />
Use the following commands to check out the latest icewm source from CVS:
    <pre style="margin-left: 40px;">
cvs -d :pserver:anonymous@cvs.icewm.sourceforge.net:/cvsroot/icewm login
# blank password
cvs -d :pserver:anonymous@cvs.icewm.sourceforge.net:/cvsroot/icewm co -r icewm-1-2-BRANCH icewm-1.2
</pre>
    <br/>
  </li>
  <li><a
 href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/icewm/icewm-1.2">Browse
CVS</a></li>
</ul>

<hr size="1"/>
<h2>Credits</h2>
<ul>
<li>Mathias Hasselmann for maintaining icewm in 2001.</li>
<li>Markus Ackermann for <a href="fish.txt">maintaing</a> icewm.org web pages</li>
<li>The mountain image is from <a href="http://artwiz.artramp.org/">artwiz</a>.</li>
<li><a href="http://beret.net/">Beret</a>, <a
href="http://www.brabbel.ch/">brabbel</a> and <a
href="http://www.rubis.ch/~beat/">bit</a> provide the domain and DNS.</li>
<li>... (many others)</li>
</ul>

<h2>License</h2>
<p>
IceWM is released under the terms of the GNU Library General Public License.
</p>

<?php include('../libphp/tail.php') ?>
