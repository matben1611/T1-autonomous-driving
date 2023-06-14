#!/usr/bin/sh

result=`cat site-design.html | tr -d '\n' | tr -d '\r' | tr -d '\t' | sed 's/\"/\\\"/g'`
echo $result
echo -n $result > compressed-site.html
echo -n $result | xclip -sel clip
