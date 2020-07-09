#!/bin/sh

COLORS=${HOME}/.cache/wal/colors.sh
test -f $COLORS || (echo "No wal colors found under $COLORS" && exit)
. $COLORS

COL_LIST="$foreground$background$color8$foreground$color10$foreground"
echo $COL_LIST

echo "c$COL_LIST" | dwmc
