/**************************************************************/
/*
 * Copyright (c) 2014-2015, Ohad Rodeh, IBM Research
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies, 
 * either expressed or implied, of IBM Research.
 * 
 */
/**************************************************************/
/******************************************************************/
/* Description: basic memory functions and pools
 */
/******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "oc_utl.h"
#include "oc_crt_int.h"

/******************************************************************/

Oc_utl_config oc_utl_conf_g;

/******************************************************************/
    
/** Update a 32-bit Linear Redundancy Check */

uint32 oc_utl_lrc_update(uint32 lrc, char * buf, int len)
{
    int n, non_align_len;
    uint32 zero_buf = 0;
    uint32 * p;

    /* must be aligned on a 32-bit boundary */
    oc_utl_assert(((uint32)buf & 3) == 0);

    /* must be multiple of 32 bits */
    //oc_utl_assert((len & 3) == 0);
    
    p = (uint32 *) buf;
    for (n = 0; n < len / 4; n++) {
        lrc ^= *p++;
    }
    
    // Handle non aligned end
    non_align_len = len%4;
    if (non_align_len > 0) {
        memcpy(&zero_buf, p, non_align_len);
        lrc ^= zero_buf;
    }

    return lrc;
}

/**************************************************************/

#define CASE(s) case s: return #s ; break

const char *oc_utl_string_of_subcomponent_id(Oc_subcomponent_id id)
{
    switch (id) {
        CASE(OC_CRT);
        CASE(OC_IO);
        CASE(OC_FS);
        CASE(OC_BT);
        CASE(OC_SN);
        CASE(OC_CAT);
        CASE(OC_OCT);
        CASE(OC_PCT);
        CASE(OC_RM);
        CASE(OC_SM);
        CASE(OC_OH);
        CASE(OC_PH);
        CASE(OC_RT);
        CASE(OC_PM);
        CASE(OC_JL);
        CASE(OC_RC);

    default:
        ERR(("bad case"));
    }
}

#undef CASE
 
/**************************************************************/


void oc_utl_init_full(Oc_utl_config *conf_pi)
{
    memcpy(&oc_utl_conf_g,  conf_pi, sizeof(Oc_utl_config));

    printf("====================\n");
    printf("UTL configuration:\n");
    printf("   lun size: %Lu(Mbyte), or, %Lu(Kbyte)\n",
           oc_utl_conf_g.lun_size /(1024 * 1024),
           oc_utl_conf_g.lun_size / 1024);
    printf("   OSD version: %lu\n", (uint32)oc_utl_conf_g.version);    
    printf("   data dev: %s\n", oc_utl_conf_g.data_dev);    
    printf("   ljl dev: %s\n", oc_utl_conf_g.ljl_dev);    
    printf("====================\n");
}

void oc_utl_default_config(Oc_utl_config *conf_po)
{
    memset(conf_po, 0, sizeof(Oc_utl_config));
    conf_po->lun_size = OC_LUN_SIZE;
        
    memset(conf_po->data_dev, 0, 60);
    memset(conf_po->ljl_dev, 0, 60);
}

void oc_utl_init(void)
{
    Oc_utl_config conf;

    oc_utl_default_config(&conf);
    oc_utl_init_full(&conf);
}

void oc_utl_free_resources()
{
    //oc_utl_page_free_resources();
}

/**************************************************************/

uint64 oc_query_input_lun_size( uint32 lun )
{
    return oc_utl_conf_g.lun_size;
}

/**************************************************************/

uint64 oc_utl_get_req_id(void)
{
    static uint64 req_id = 1;
    req_id++;
    return req_id;
}

/**************************************************************/

uint32 oc_utl_log2(uint32 num)
{
    int i;

    if (0 == num || 1 == num) return 0;
    
    for (i=1; i<32; i++) {
        num = num>>1;
        if (0 == num) return (i-1);
    }

    return 32;
}

/**************************************************************/
// statistics


