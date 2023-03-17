#!/usr/bin/zsh

result=`cat site-design.html |  tr -d '\n' | tr -d '\t'`
echo $result
echo $result > compressed-site.html
echo $result | xclip -sel clip
