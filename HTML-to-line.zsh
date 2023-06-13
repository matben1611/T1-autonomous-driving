#!/usr/bin/zsh

result=`cat site-design.html | tr -d '\n' | tr -d '\t' | sed 's/\"/\\\"/g'`
echo $result
echo $result > compressed-site.html
echo -n $result | xclip -sel clip
