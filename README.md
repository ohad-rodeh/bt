bt
==

COW b-trees

This repository implements the COW friendly b-trees, as described in the paper:
     Ohad Rodeh
     B-trees, Shadows, and Clones
     ACM Transactions on Storage 2008

The code contains a lot of scaffolding, but the main directory is
src/oc/bpt. BPT is short for "B Plus Tree". The license is BSD, which
should allow incorporating the code into other open source and commercial projects.

Directory naming
================

The src directory contains these sub-directories:

    oc      object code
    oc/bpt  b-tree
    oc/crt  co-routines
    oc/ds   data structures
    oc/utl  utilities
    oc/xt   extent tree
    pl      platform
