#!/bin/sh -f 

for i in `find . -name "*.[ch]"`
do 
  echo "converting " $i

  sed -f /home/orodeh/U/git/orodeh/bt/src/tools/convert_files.sed $i > M
  mv M $i
done
