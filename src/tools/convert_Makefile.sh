#!/bin/sh -f 

for i in `find . -name "files.mk"`
do 
  echo "converting " $i

  ~/store/OSD/SA/src/tools/convert_Makefile.awk $i > M
  mv M $i
done
