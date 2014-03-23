#!/bin/gawk -f

BEGIN {
}

# The line is:
#   #include<ss_*.h>
#   #include<pl*.h>
#   #include<oc_*.h>
#
$1 == "#include" {
    if ($2 ~ /<ss_.+h>/ ||
        $2 ~ /<pl.+h>/  ||
        $2 ~ /<oc_.+h>/ ||
        $2 == "<generic_dbg.h>" ) {
        sub(/</, "\"", $2); 
        sub(/>/, "\"", $2); 
        print $1 " " $2
    } else {
        print
    }
}

$1 != "#include" {
    print
}

END {
}
