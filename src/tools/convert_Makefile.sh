#!/bin/sh -f 

#for i in `find . -name "files.mk"`
for i in `find . -name "Makefile"`
do 
  echo "converting " $i

  #~/store/OSD/SA/src/tools/convert_Makefile.awk $i > M
  #mv M $i
  sed -f /home/orodeh/U/git/orodeh/bt/src/tools/convert_files.sed $i > M
  mv M $i
done
