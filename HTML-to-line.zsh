#!/usr/bin/zsh

result=`cat site-design.html | tr -d '\n' | tr -d '\t' | sed 's/\"/\\\"/g'`
echo $result
echo $result > compressed-site.html
echo $result | xclip -sel clip
