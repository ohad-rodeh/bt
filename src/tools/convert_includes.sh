#!/bin/sh -f 

for i in `find . -name "*.[ch]"`
do 
  echo "converting " $i

  ~/store/OSD/SA/src/tools/convert_includes.awk $i > M
  mv M $i
done
