BEGIN {
}

# The line is:
#   oc_bt_op.o : 
#
#{
#    if ($1 ~ /(.+)\.o:/)
#        print "${OBJDIR}/" $0;
#    else
#        print $0;
#}

{
    for (i=1; i<=NF; i++) {
        if ( $i ~ /(.+)\.o:/ )
            printf ("\n${OBJDIR}/%s ", $i);
        if ( $i ~ /(.+)\.h/ )
            printf ("${OSDROOT}/src/%s ", $i);
        if ( $i == "\\" )
            printf ("\\");
    }
    printf("\n");
}


END {
}
