#!/bin/csh

if ( $#argv <> 1 ) then 
  echo "usage: $0 tarfile
  exit -1
endif

set file = $argv[1]
echo "tarcheck:file=$file"

/bin/rm -f .tmpcheck
zcat $file | tar tf - > .tmpcheck

echo "tarcheck:looking for .err files"
grep "\.err"	.tmpcheck
echo "tarcheck:looking for ~ files"
grep "~"	.tmpcheck 
echo "tarcheck:looking for bu dirs"
grep "/bu/"	.tmpcheck 
echo "tarcheck:looking for files with old in name"
grep "old"	.tmpcheck
echo "tarcheck:looking for files with tmp in name"
grep "tmp"	.tmpcheck
/bin/rm -f .tmpcheck
echo "tarcheck:done"
