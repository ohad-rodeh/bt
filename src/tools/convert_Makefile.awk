#!/bin/gawk -f

BEGIN {
}

# The line is:
#   ${OCROOT}/cp/oc_cp${OBJ_SUFF} \
#   ${OCROOT}/cp/oc_cp_recv${OBJ_SUFF} \
#
# convert into
#   ${OBJDIR}/oc_cp${OBJ_SUFF} \
#   ${OBJDIR}/oc_cp_recv${OBJ} \
#

{
    if ($1 ~ /(.+)OCROOT(.+)/)	 {
        split ($1,t,"\/");	
        sub(/OBJ_SUFF/, "OBJ", t[3]);     
        printf "\t" "${OBJDIR}/" t[3] " 	\\ \n";
    } else {
        print;
    }
}

END {
    printf "\n"
}
