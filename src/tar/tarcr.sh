#!/bin/tcsh

if ( $#argv <> 2 ) then 
  echo "usage: $0 version ROOT"
  exit -1
endif

set version     = "$argv[1]"
set year	= `date +%Y`
set root        = "$argv[2]"
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
rm -rf $tardir/src/cscope*
rm -f $tardir/src/TAGS

echo "Setting up the unknown file"
set unknown = /tmp/.unknown
/bin/rm -f $unknown
touch $unknown

set files = `find $tardir -print`

foreach i ( $files )
  if ( ( -d $i ) || ( ! ( -w $i ) ) ) continue

  set base = `basename $i`
  set type = ""
  switch($base)
    case *.pdf:
    case *.dot:
    case *.png:
    case *.conf:
    case *.header:
    case *.base:
    case *.suffix:
    case copyright*:
    case patch-*:
    case *.zip:
    case *.tar:
    case *.src:
    case *.awk:
    case *.sh:
    case regressions:
      set type = ""
      continue
      breaksw

    case *.[Cch]:
    case *.cc:
    case *.hh:
    case *.flex:
    case *.y:
      set type = C
      breaksw

    case *.sed:
    case ntify.bat:
    case ntify.csh:
    case *_DOC:
    case FUTURE:
    case *.html:
    case *.htm:
    case *.jpg:
    case rpm.spec:
    case dbdefault:
      set type = ""
      breaksw

    case Makefile:
    case Makefile.*:
    case *.oc:
    case *.ini:
    case *.mk:
    case README:
    case INSTALL*:
    case .depend:
    case BUGS:
    case TODO:
      set type = mk
      breaksw

    case *.tex:
    case *.bib:
      set type = tex
      breaksw

    default:
      set type = ""
      echo $i >> $unknown
      endif
      breaksw
  endsw

  echo "File: $i ($type)"
  if ( $type != "" ) then
    (sed -e "s|%%VERSION%%|$version|" -e "s|%%DATE%%|$year|" $tardir/src/tar/copyright.$type ;\
     cat $i) > M
    mv -f M $i
  endif
end

echo "Unknown files:"
cat $unknown
/bin/rm -f $unknown

echo "Creating a tar file"
pushd $tardir
tar -cf bt-$version.tar src
gzip bt-$version.tar
popd

echo "Copying tar-balls to the origin location"
cp $tardir/bt-$version.tar.gz . 

#rm -rf $tardir
