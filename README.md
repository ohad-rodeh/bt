# bt

COW b-trees

This repository implements the COW friendly b-trees, as described in the paper:
     Ohad Rodeh
     B-trees, Shadows, and Clones
     ACM Transactions on Storage 2008

The code contains a lot of scaffolding, but the main directory is
src/oc/bpt. BPT is short for "B Plus Tree". The license is BSD, which
should allow incorporating the code into other open source and commercial projects.

### Build & run tests
<a href="https://travis-ci.org/ohad-rodeh/bt"><img src="https://travis-ci.org/ohad-rodeh/bt.svg?branch=master"/></a>
The project depends on standard Linux tools: `gcc`, `make`, `awk`, `sed`.

In order to compile the project, from the root directory `ROOT`, do:
```
cd src
make all
```

This will create three directories under `bin`, `lib`, and `obj`. They
contain generated binaries, libraries, and object files.

The sources for the b-tree tests are in directory `src/oc/bpt/test`. The test
script that exercises all the tests is `run_tests.sh`.


### Directory naming

The src directory contains these sub-directories:

    oc      object code
    oc/bpt  b-tree
    oc/crt  thread and lock support, based on pthreads
    oc/ds   data structures
    oc/utl  utilities
    oc/xt   extent tree
    pl      platform

The locking and thread support is currently based on Linux
pthreads. The name "crt" is due to an
internal co-routine library that was used prior to this, and never released.

### B-tree implementation

Directory oc/bpt holds the core b-tree code, the files are as follows:

    oc_bpt_int.h                   main interface file
    oc_bpt_label.[ch]              labeling b-tree nodes, for debugging
    oc_bpt_nd.[ch]                 structure of a b-tree node (page)
    oc_bpt_op_insert.[ch]          insert key algorithm
    oc_bpt_op_insert_range.[ch]    insert a key range algorithm
    oc_bpt_op_lookup.[ch]          key lookup algorithm
    oc_bpt_op_output_dot.[ch]
    oc_bpt_op_output_clones_dot.[ch] generate output file in dot format
    oc_bpt_op_remove_key.[ch]      remove key algorithm
    oc_bpt_op_remove_range.[ch]    remove a key range algorithm
    oc_bpt_op_stat.[ch]
    oc_bpt_op_validate.[ch]        validate a btree, for debugging
    oc_bpt_op_validate_clones.[ch] validate a btree, for debugging

The most complicated algorithm is remove-range. The main issue is that
removing a range from the middle of a tree causes significant
difficulties when trying to rebalance it. The output-dot files are
used to generate b-tree descriptions in dot format
(http://www.graphviz.org/). This allows using the dot program to
visualize test outputs, for example:

    dot -Tjpg X.dot -o X.jpg

The `Oc_wu` structure that is passed around by the code, describe a
"work-unit". This is probably equivalent to a transaction in the
caller code.
