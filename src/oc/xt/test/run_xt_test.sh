#!/bin/sh

#-----------------------------------------------------------------
# Read command line parameters into -flags-
flags=$*

ocroot=../../../..
oc_xt_test_st=$ocroot/bin/oc_xt_test_st
oc_xt_test_mt=$ocroot/bin/oc_xt_test_mt

if test ! -x $oc_xt_test_st
then
	echo Error: executable $oc_xt_test_st not found
	echo aborting
	exit 1
fi

#-----------------------------------------------------------------
# running the test with various error-injection parameters

for fanout in 5 11 19
  do
  for max_int in 100 1000 2000
    do
    exec_flags="-max_int $max_int -max_non_root_fanout $fanout -max_root_fanout 5  -test small_trees $flags"
    echo "Running $oc_xt_test_st $exec_flags"
    $oc_xt_test_st  $exec_flags 
    
    if test $? -ne 0
	then 
	echo "-----------------------------------------------"
	echo "Failure of test: $oc_xt_test_st $exec_flags"
	echo "-----------------------------------------------"
	exit 1
    fi

    exec_flags="-max_int $max_int -max_non_root_fanout $fanout -max_root_fanout 5  -test small_trees_w_ranges $flags"
    echo "Running $oc_xt_test_st $exec_flags"
    $oc_xt_test_st $exec_flags 
    
    if test $? -ne 0
	then 
	echo "-----------------------------------------------"
	echo "Failure of test: $oc_xt_test_st $exec_flags"
	echo "-----------------------------------------------"
	exit 1
    fi

    exec_flags="-max_int $max_int -max_non_root_fanout $fanout -max_root_fanout 5  -test small_trees_mixed $flags"
    echo "Running $oc_xt_test_st $exec_flags"
    $oc_xt_test_st $exec_flags 
    
    if test $? -ne 0
	then 
	echo "-----------------------------------------------"
	echo "Failure of test: $oc_xt_test_st $exec_flags"
	echo "-----------------------------------------------"
	exit 1
    fi
    
    for num_rounds in 100 1000 2000
      do
      exec_flags="-max_int $max_int -num_rounds $num_rounds -max_non_root_fanout $fanout -max_root_fanout 5 -test large_trees $flags "
      echo "Running $oc_xt_test_st $exec_flags"
      $oc_xt_test_st $exec_flags 
      
      if test $? -ne 0
	  then 
	  echo "-----------------------------------------------"
	  echo "Failure of test: $oc_xt_test_st $exec_flags"
	  echo "-----------------------------------------------"
	  exit 1
      fi
    done
  done
done

#for fanout in 5 11 19
#  do
#  for max_int in 100 1000 2000
#    do
#    for num_rounds in 100 1000 2000
#      do
#      exec_flags="-max_int $max_int -num_rounds $num_rounds -max_non_root_fanout $fanout -max_root_fanout 5 -num_tasks 40 $flags"
#      echo "Running $oc_xt_test_mt $exec_flags"
#      $oc_xt_test_mt $exec_flags
#      
#      if test $? -ne 0
#	  then 
#	  echo "-----------------------------------------------"
#	  echo "Failure of test: $oc_xt_test_mt $exec_flags"
#	  echo "-----------------------------------------------"
#	  exit 1
#      fi
#    done
#  done
#done

#-----------------------------------------------------------------


