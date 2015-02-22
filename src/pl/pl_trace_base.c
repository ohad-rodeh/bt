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
/****************************************************************/
// Basic tracing facility
/****************************************************************/
#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

#include "pl_trace_base.h"
/****************************************************************/

#define TAG_MAX_SIZE (12)
#define TAG_MIN_SIZE (2)
#define LEVEL_MAX (4)
#define LEVEL_MIN (1)

#define MAX(x,y) ((x) > (y) ? (x) : (y))

// An array with a flag per tag. The flag is true if the tag should be traced. 
static unsigned char trace_tag_a[PL_TRACE_BASE_LAST_TAG];

/* An array that matches a tag with its string representation. This
 *  array is set up upon initialization and allows quickly matching
 *  between a user's command line trace requests and numeric ids of
 *  tags.
 */
static char trace_tag_str_a[PL_TRACE_BASE_LAST_TAG][TAG_MAX_SIZE];
static char trace_tag_str_a_prn[PL_TRACE_BASE_LAST_TAG][TAG_MAX_SIZE];

// The trace level, we will probably support only two levels. 
static int trace_level;
static int level_has_been_set;

static char *convert_tag_to_string(Pl_trace_base_tag tag);

/****************************************************************/
#define CASE(s) case s: return #s ; break

static char *convert_tag_to_string(Pl_trace_base_tag tag)
{
    switch (tag) {
        CASE(PL_TRACE_BASE_ALL);
        CASE(PL_TRACE_BASE_PL);
        CASE(PL_TRACE_BASE_HNS);
        
        // OC tags
        CASE(PL_TRACE_BASE_OC_CRT);
        CASE(PL_TRACE_BASE_OC_UTL);
        CASE(PL_TRACE_BASE_OC_IO);
        CASE(PL_TRACE_BASE_OC_BPT);
        CASE(PL_TRACE_BASE_OC_PM);
        CASE(PL_TRACE_BASE_OC_ALL);
        
    default:
        printf("tag %d does not have a conversion to string, exiting.\n", tag);
        assert(0);
    }
    
    /* We should not get here.
     * 
     * However, in kernel mode, I don't want to do a kernel-panic.
     */
    return "PL_TRACE_BASE_XXX";
}

#undef CASE
/****************************************************************/

void pl_trace_base_init(void)
{
    int i;
    
    memset(trace_tag_a, 0, sizeof(trace_tag_a));
    memset(trace_tag_str_a, 0, sizeof(trace_tag_str_a));
    memset(trace_tag_str_a_prn, 0, sizeof(trace_tag_str_a_prn));
    trace_level = 0;
    level_has_been_set = 0;
    
    // Set the array of string-representations of tags.
    for(i=0; i<PL_TRACE_BASE_LAST_TAG; i++) {
        char *s =convert_tag_to_string((Pl_trace_base_tag)i);
        int len;
        
        assert(strlen(s)-14 <= (TAG_MAX_SIZE-1) &&
               strlen(s)-14 >= TAG_MIN_SIZE );
        
        // We need to skip over the initial "PL_TRACE_BASE_" stuff. 
        strncpy(trace_tag_str_a[i], &s[14], TAG_MAX_SIZE-1);
        len = strlen(trace_tag_str_a[i]);
    
        // create a printable version of the tag
        memset(trace_tag_str_a_prn[i], ' ', TAG_MAX_SIZE);
        trace_tag_str_a_prn[i][0] = '<';
        memcpy(&trace_tag_str_a_prn[i][1],
               trace_tag_str_a[i],
               len);
        trace_tag_str_a_prn[i][1+len] = '>';
        trace_tag_str_a_prn[i][TAG_MAX_SIZE-1] = 0;
    }
    
    // We need to make sure that the maximal level can fit in a uint8
    assert(LEVEL_MAX <= 255);
}

void pl_trace_base_init_done(void)
{
    int i;
    
    /* Set the trace-level per tag to the maximum between the 
     * global trace level and the local trace-level. 
     */
    for (i=0; i<PL_TRACE_BASE_LAST_TAG; i++)
        trace_tag_a[i] = MAX(trace_tag_a[i], trace_level);
}


void pl_trace_base_set_level(int level)
{
    if (level_has_been_set) {
        printf("pl_trace_base: the trace level has already been set, exiting.\n");
        exit(1);
    }
    if (!(LEVEL_MIN<= level && level <= LEVEL_MAX)) {
        printf("pl_trace_base: the trace level can be between %d and %d.\n",
              LEVEL_MIN, LEVEL_MAX);
        printf("           Level %d is not supported.\n", level);
        exit(1);
    }

    level_has_been_set = 1;
    trace_level = level;
}
    
int pl_trace_base_is_set(Pl_trace_base_tag tag, int level)
{
    return trace_tag_a[tag] >= level;
}

/****************************************************************/

void pl_trace_base_add_tag_lvl(Pl_trace_base_tag tag, int level)
{
    assert(0<= tag && tag < PL_TRACE_BASE_LAST_TAG);
    assert(LEVEL_MIN<= level && level <= LEVEL_MAX);
    
    /* set the trace level [tag] to the maximum between the current
     * level, and [level].
     */
    trace_tag_a[tag] = MAX (trace_tag_a[tag], level);
}

/****************************************************************/

void pl_trace_base(Pl_trace_base_tag tag,
                   int level,
                   const char *format_p,
                   ...)
{
    va_list args;
    char buf [200];
    
    if (trace_tag_a[tag] >= level) {
        snprintf(buf, sizeof(buf), "%s ", trace_tag_str_a_prn[tag]);
                
        va_start(args, format_p);
        //TODO: improve this function to take up less stack space. 
        vsnprintf(&buf[strlen(buf)], sizeof(buf)-strlen(buf),
		  format_p, args);
        va_end(args);

	// make sure buf is large enough
	assert(strlen(buf)<(sizeof(buf)-2));

        printf("%s", buf);
        fflush(stdout);
    }
}

/****************************************************************/


/* Add [tag_p], which is in string format, to the list of traced tags.
 * For example, to trace tag PL_TRACE_BASE_OC_PM with level 1 you'll
 * need to call:
 *
 *     pl_trace_base_add_tag("OC_PM", 1);
 * 
 */
static void pl_trace_base_add_string_tag(char *tag_p, int level)
{
    int i;

    for(i=0; i<PL_TRACE_BASE_LAST_TAG; i++)
        if (0 == strcmp(tag_p, trace_tag_str_a[i])) {
            /* Some tags are for groups. This means that they allow tracing
             * a whole group of other tags. 
             */
            switch (i) {
            case PL_TRACE_BASE_ALL:
                pl_trace_base_add_tag_lvl(PL_TRACE_BASE_PL, level);
                pl_trace_base_add_tag_lvl(PL_TRACE_BASE_HNS, level);
                pl_trace_base_add_tag_lvl(PL_TRACE_BASE_OC_CRT, level);
                pl_trace_base_add_tag_lvl(PL_TRACE_BASE_OC_UTL, level);
                pl_trace_base_add_tag_lvl(PL_TRACE_BASE_OC_IO, level);
                pl_trace_base_add_tag_lvl(PL_TRACE_BASE_OC_BPT, level);
                pl_trace_base_add_tag_lvl(PL_TRACE_BASE_OC_PM, level);
                break;
            case PL_TRACE_BASE_OC_ALL:
                pl_trace_base_add_tag_lvl(PL_TRACE_BASE_OC_CRT, level);
                pl_trace_base_add_tag_lvl(PL_TRACE_BASE_OC_UTL, level);
                pl_trace_base_add_tag_lvl(PL_TRACE_BASE_OC_IO, level);
                pl_trace_base_add_tag_lvl(PL_TRACE_BASE_OC_BPT, level);
                pl_trace_base_add_tag_lvl(PL_TRACE_BASE_OC_PM, level);
                break;
            }

            pl_trace_base_add_tag_lvl((Pl_trace_base_tag)i, level);
            return;
        }

    printf("pl_trace_base: tag=%s is not supported.\n", tag_p);
    exit(1);
}

/* Add a tag, or a tag:level to the traces.
 *
 * There are two supported formats:
 *  <tag>  <tag>:<level>
 *
 * The first format is assumed to be for level=1.
 */
void pl_trace_base_add_string_tag_full(char *str_p)
{
    int len = strlen(str_p);

    if (len < TAG_MIN_SIZE) {
        printf("pl_trace_base: Tag too short, has to be at least of size %d\n", 
              TAG_MIN_SIZE);
        exit(1);
    }
    if (len >= TAG_MAX_SIZE) {
        printf("pl_trace_base: Tag too long, has to be shorter than %d\n", 
              TAG_MAX_SIZE);
        exit(1);
    }

    // We check if this string combines a tag and a level
    if (str_p[len-2] == ':') {
        // Complex case, this is (tag:level) pair
        char lcopy[TAG_MAX_SIZE];
        int level;
        
        // split the string into two sub-strings
        memset(lcopy, 0, sizeof(lcopy));
        strncpy(lcopy, str_p, sizeof(lcopy)-1);
        lcopy[len-2] = '\0';

        // handle the level part
        level = atoi((char*)&lcopy[len-1]);
        if (!(level <= LEVEL_MAX && level >= LEVEL_MIN)) {
            printf("pl_trace_base: Level %d not supported\n", level);
            exit(1);
        }
        
        // handle the tag part
        pl_trace_base_add_string_tag(lcopy, level);
        
    } else {
        // Regular case, this is a simple tag
        pl_trace_base_add_string_tag(str_p, 1);
    }
}

void pl_trace_base_print_tag_list(void)
{
    int i;

    printf("    The set of possible tags is:\n");
    for (i=0; i<PL_TRACE_BASE_LAST_TAG; i++) 
        printf("\t %s\n", trace_tag_str_a[i]);
    printf("    A level per-tag can be specified with <tag:level>\n");
    printf("    For example:\n");
    printf("\t OC_JL:2 \n");
    printf("    will trace tag OC_JL at levels 1 and 2\n");
}

/****************************************************************/
