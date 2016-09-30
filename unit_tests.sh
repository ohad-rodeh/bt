#!/bin/bash -x

#-----------------------------------------------------------------
# Read command line parameters into -flags-
extra_flags=$*

ocroot=.
oc_bpt_test_st=$ocroot/bin/oc_bpt_test_st
oc_bpt_test_mt=$ocroot/bin/oc_bpt_test_mt
oc_bpt_test_clone_st=$ocroot/bin/oc_bpt_test_clone_st
oc_bpt_test_clone_mt=$ocroot/bin/oc_bpt_test_clone_mt

if test ! -x $oc_bpt_test_st
then
	echo Error: executable $oc_bpt_test_st not found
	echo aborting
	exit 1
fi

if test ! -x $oc_bpt_test_mt
then
	echo Error: executable $oc_bpt_test_mt not found
	echo aborting
	exit 1
fi

if test ! -x $oc_bpt_test_clone_st
then
	echo Error: executable $oc_bpt_test_clone_st not found
	echo aborting
	exit 1
fi

#if test ! -x $oc_bpt_test_clone_mt
#then
#	echo Error: executable $oc_bpt_test_clone_mt not found
#	echo aborting
#	exit 1
#fi

#-----------------------------------------------------------------

function run_st_test
{
    exec_flags=$@

    echo "Running $oc_bpt_test_st $exec_flags $extra_flags"
    $oc_bpt_test_st $exec_flags $extra_flags

    if test $? -ne 0
	then
	echo "-----------------------------------------------"
	echo "Failure of test: $oc_bpt_test_st $exec_flags $extra_flags"
	echo "-----------------------------------------------"
	exit 1
    fi
}

function run_mt_test ()
{
    exec_flags=$@

    echo "Running $oc_bpt_test_mt $exec_flags $extra_flags"
    $oc_bpt_test_mt $exec_flags $extra_flags

    if test $? -ne 0
	then
	echo "-----------------------------------------------"
	echo "Failure of test: $oc_bpt_test_mt $exec_flags $extra_flags"
	echo "-----------------------------------------------"
	exit 1
    fi
}

function run_clone_st_test ()
{
    exec_flags=$@

    echo "Running $oc_bpt_test_clone_st $exec_flags $extra_flags"
    $oc_bpt_test_clone_st $exec_flags $extra_flags

    if test $? -ne 0
	then
	echo "-----------------------------------------------"
	echo "Failure of test: $oc_bpt_test_clone_st $exec_flags $extra_flags"
	echo "-----------------------------------------------"
	exit 1
    fi
}

function run_clone_mt_test ()
{
    exec_flags=$@

    echo "Running $oc_bpt_test_clone_mt $exec_flags $extra_flags"
    $oc_bpt_test_clone_mt $exec_flags $extra_flags

    if test $? -ne 0
	then
	echo "-----------------------------------------------"
	echo "Failure of test: $oc_bpt_test_clone_mt $exec_flags $extra_flags"
	echo "-----------------------------------------------"
	exit 1
    fi
}

#-----------------------------------------------------------------
# running the test with various error-injection parameters

std="yes"
multi_threaded_tests="no"

if [[ $std == "yes" ]]
    then
    for fanout in 5 11 19
      do
      for max_int in 100 1000 2000
	do
	run_st_test "-max_int $max_int -max_non_root_fanout $fanout -max_root_fanout 5 -test small_trees"

	for num_rounds in 100 1000 2000
	  do
	  run_st_test "-max_int $max_int -num_rounds $num_rounds -max_non_root_fanout $fanout -max_root_fanout 5 -test large_trees"
	done
      done
    done
fi

if [[ $std == "yes" ]]
    then
    for fanout in 5 11 19
      do
      for max_int in 100 1000 2000
	do
	for num_rounds in 100 1000 2000
	  do
	  run_mt_test "-max_int $max_int -num_rounds $num_rounds -max_non_root_fanout $fanout -max_root_fanout 5 -num_tasks 40"
	done
      done
    done
fi

if [[ 1 ]]
    then
    for fanout in 5 11 19
      do
      for max_int in 100 1000 2000
	do
	for num_rounds in 100 1000 2000
	  do
	  run_clone_st_test "-max_int $max_int -num_rounds $num_rounds -max_non_root_fanout $fanout -max_root_fanout 5 -max_num_clones 10"
	done
      done
    done
fi

if [[ $multi_threaded_tests == "yes"  ]]
    then
    for fanout in 5 11 19
      do
      for max_int in 100 1000 2000
	do
	for num_rounds in 100 1000 2000 10000
	  do
	  run_clone_mt_test "-max_int $max_int -num_rounds $num_rounds -max_non_root_fanout $fanout -max_root_fanout 5 -max_num_clones 4 -num_tasks 5"
	  run_clone_mt_test "-max_int $max_int -num_rounds $num_rounds -max_non_root_fanout $fanout -max_root_fanout 5 -max_num_clones 20 -num_tasks 5"
	  run_clone_mt_test "-max_int $max_int -num_rounds $num_rounds -max_non_root_fanout $fanout -max_root_fanout 5 -max_num_clones 10 -num_tasks 30"
	done
      done
    done
fi

#-----------------------------------------------------------------
