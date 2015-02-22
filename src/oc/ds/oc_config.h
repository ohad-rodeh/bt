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
#ifndef OC_CONFIG_H
#define OC_CONFIG_H

/** The size of the LUN, in bytes. Don't use this parameter directly,
 *  you need to use:
 *         uint64 oc_query_input_lun_lbas( uint32 lun )
 *  OC_LUN_SIZE is used only for emulator configurations where we do
 *  not get the LUN size from the Lodestone event system.
 */
#define OC_LUN_SIZE             (1024 * 1024 * 1024)

/** The maximum number of LUNs
 */
#define OC_MAX_LUN (16)

// The number of pages in the page-pool
#define OC_NUM_PAGES      (4096)

// Print out statistics? 
#define OC_STATS (FALSE)

/*****************************************************************************/

/* comment out OC_USE_MUTEX to use spinlocks */
#define OC_USE_MUTEX

/*****************************************************************************/
// SM section
// PM section

// The number of pages managed by the PM
#define OC_PM_NUM_PAGES                 (1000)

/* The maximal number of pages referenced at the same time
 * Can not be set from a script.
 */
#define OC_PM_MAX_REFS                  (20)

/*****************************************************************************/

// Naumber of concurrent tasks in the system
#define OC_CRT_NUM_TASKS                 (64)

/*****************************************************************************/
// IO section

#define OC_IO_DIRECTORY "/tmp"

#define OC_IO_THRD_BKEND_DEFAULT_NUM_THREADS (10)

/*****************************************************************************/

#endif 
