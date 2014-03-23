/**************************************************************/
/*
 * Copyright (c) 2014, Ohad Rodeh, IBM Research
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
/**********************************************************************/
/* A set of basic functions used by the target
 */
/**********************************************************************/
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "pl_base.h"
#include "pl_int.h"
#include "pl_utl.h"

/**********************************************************************/

uint32 pl_rand_uint32(uint32 num)
{
    return (rand () % num);
}

uint32 pl_rand_uint32_r(uint32 *seed_p, uint32 num)
{
    return (rand_r((unsigned int*)seed_p) % num);
}

/* test that all bytes in a buffer equal byte_value.
 * return 0 if equal, -1 otherwise.
 */
int pl_memtest(void* buf_p, uint8 byte_value, int length)
{
    int i;
    uint8* bytes_p=(uint8*)buf_p;

    for (i=0;i<length;i++) {
        if (bytes_p[i]!=byte_value) {
            return -1;
        }
    }

    return 0;
}

void pl_init(void)
{
    pl_mm_init();
}


static char pl_base_user_dir[100];

char* pl_create_base_dir(void)
{
    char* user_p;
    char pl_base_dir[100];
    
    // make base dir

    strcpy(pl_base_dir, "/tmp");

    if ( (mkdir(pl_base_dir, 0755)) &&
         (errno!=EEXIST) )
    {
        printf("error creating base directory \"%s\": errno %i\n", 
               pl_base_dir, errno);
        assert(0);
    }
    
    // Add user name
    user_p = getenv("USER");  
    assert (user_p != NULL);
    sprintf(pl_base_user_dir, "%s/%s", pl_base_dir, user_p);
   
    if ( (mkdir(pl_base_user_dir, 0755)) &&
         (errno!=EEXIST) ) {
        printf("error creating directory \"%s\" : errno %i\n", 
               pl_base_user_dir, errno);
        assert(0);
    }
    
    return pl_base_user_dir;
}

char *pl_get_base_dir(void)
{
    return pl_base_user_dir;
}


