#!/bin/sh -f 

for i in `find . -name "*.[ch]"`
do 
  echo "converting " $i

  sed -f ~/Useful/Btree/course/tools/convert_files.sed $i > M
  mv M $i
done
