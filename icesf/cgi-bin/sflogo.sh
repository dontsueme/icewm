#!/bin/sh

IMGURL='http://sourceforge.net/sflogo.php?group_id=31'
IMGPATH='../htdocs/sflogo88-1-270.png'

wget -qO/dev/null -o/dev/null -U"$HTTP_USER_AGENT" "$IMGURL" &

cat <<.
Cache-Control: no-cache, must-revalidate
Pragma: no-cache
Content-Lenght: `size $IMGPATH`
Content-Type: image/png"

.

exec cat "$IMGPATH"
