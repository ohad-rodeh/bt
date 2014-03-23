#!/bin/tcsh

if ( $#argv <> 1 ) then 
  echo "usage: $0 ROOT"
  exit -1
endif

set root        = "$argv[1]"
set tardir      = /tmp/tarcr_osd

# Create the target directory, and copy the 
# distribution over there. 
echo "preparing target"
rm -rf $tardir
mkdir -p $tardir
cp -r $root/src $tardir

echo "Cleaning junk"
make -C $tardir/src realclean

echo "Removing some files"
find $tardir -name "*~*" | xargs rm -f
find $tardir -name "*.#*" | xargs rm -f
find $tardir -name ".svn" | xargs rm -rf 
rm -f $tardir/src/.depend
rm -rf $tardir/src/legal
rm -rf $tardir/src/tar
rm -rf $tardir/src/cscope*
rm -f $tardir/src/TAGS

echo "Creating a tar file"
pushd $tardir
tar -cf osd.tar src
gzip osd.tar
zip -b -r . osd.zip src
popd

echo "Copying tar-balls to the origin location"
cp $tardir/osd.tar.gz . 
cp $tardir/osd.zip . 

#rm -rf $tardir
