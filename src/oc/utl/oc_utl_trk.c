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
/* OC_UTL_TRK.C  
 *
 * Tracking resources, mostly locks to start with. 
 */
/**********************************************************************/
#include "oc_utl_trk.h"
#include "oc_crt_int.h"
#include "oc_rm_s.h"
#include "oc_utl.h"

static void add_ref(Oc_wu *wu_pi, Oc_crt_rw_lock *lock_p);
static void rmv_ref(Oc_wu *wu_pi, Oc_crt_rw_lock *lock_p);
/**********************************************************************/

static void add_ref(Oc_wu *wu_pi, Oc_crt_rw_lock *lock_p)
{
    Oc_utl_trk_ref_set *refs_p = &wu_pi->rm_p->utl_rm.refs;
    
    // oc_pm_trace_wu(OC_EV_PM_REFS_ADD, wu_pi, pcb_p->page_id.lba, 0);
    oc_utl_assert(refs_p->sum <= OC_UTL_TRK_MAX_REFS);
        
    if (refs_p->sum == refs_p->cursor) {
        // add the lock in at the end
        refs_p->locks[refs_p->cursor] = lock_p;
        refs_p->cursor++;
        refs_p->sum++;
        
        // oc_pm_trace_wu(OC_EV_PM_REFS_ADD2, wu_pi, refs_p->cursor, refs_p->sum);
    } else {
        // there is an empty slot in the beginning: add the lock there
        int i;

        oc_utl_assert(refs_p->cursor-1 >= 0);
        for (i=refs_p->cursor-1; i>=0; i--)
            if (NULL == refs_p->locks[i]) {
                refs_p->locks[i] = lock_p;
                refs_p->sum++;
                // oc_pm_trace_wu(OC_EV_PM_REFS_ADD2, wu_pi, refs_p->cursor, refs_p->sum);
                return;
            }

        ERR(("sanity: did not manage to add a lock into the list"));
    }
}


void rmv_ref(Oc_wu *wu_pi, Oc_crt_rw_lock *lock_p)
{
    Oc_utl_trk_ref_set *refs_p = &wu_pi->rm_p->utl_rm.refs;

    // oc_pm_trace_wu(OC_EV_PM_REFS_RMV, wu_pi, pcb_p->page_id.lba, 0);
    oc_utl_debugassert(refs_p->sum > 0);
    oc_utl_debugassert(refs_p->cursor > 0);

    refs_p->sum--;
    
    if (refs_p->locks[refs_p->cursor-1] == lock_p) {
        /* optimization: the removed lock was the last one to be taken.
         * This is useful when the user has taken locks in LIFO (or stack)
         * pattern. The 
         */
        refs_p->locks[refs_p->cursor-1] = NULL;
        refs_p->cursor--;
        
        while (refs_p->cursor >= 1 &&
               NULL == refs_p->locks[refs_p->cursor-1])
            refs_p->cursor--;
        
        // oc_pm_trace_wu(OC_EV_PM_REFS_RMV2, wu_pi, refs_p->cursor, refs_p->sum);
    } else {
        int i;

        /* Optimization didn't work, we need to search for the lock. 
         * Since we assume a stack usage pattern, start the search from the end. 
         */
        oc_utl_assert(refs_p->cursor >= 1);
        for (i=refs_p->cursor-1; i>=0; i--) {
            if (refs_p->locks[i] == lock_p) {
                refs_p->locks[i] = NULL;
                // oc_pm_trace_wu(OC_EV_PM_REFS_RMV2, wu_pi, refs_p->cursor, refs_p->sum);
                return;
            }
        }

        ERR(("lock=%p was released; however, it was never taken.", lock_p));
    }
}

/**********************************************************************/
void oc_utl_trk_crt_lock_read(Oc_wu *wu_p, Oc_crt_rw_lock *lock_p)
{
    oc_utl_debugassert(wu_p->rm_p);
    oc_crt_lock_read(lock_p);
    add_ref(wu_p, lock_p);
}

void oc_utl_trk_crt_lock_write(Oc_wu *wu_p, Oc_crt_rw_lock *lock_p)
{
    oc_utl_debugassert(wu_p->rm_p);
    oc_crt_lock_write(lock_p);
    add_ref(wu_p, lock_p);
}

void oc_utl_trk_crt_unlock(Oc_wu *wu_p, Oc_crt_rw_lock *lock_p)
{
    oc_utl_debugassert(wu_p->rm_p);
    oc_crt_unlock(lock_p);
    rmv_ref(wu_p, lock_p);
}

// release the set of locks held by this work-unit
void oc_utl_trk_abort(Oc_wu *wu_p)
{
    int i;
    Oc_utl_trk_ref_set *refs_p;

    oc_utl_debugassert(wu_p->rm_p);
    refs_p = &wu_p->rm_p->utl_rm.refs;

    if (0 == refs_p->cursor)
        return;
    
    for(i=0; i<refs_p->cursor; i++) {
//        printf("MODE: %d", refs_p->locks[i]->mode);
        if (refs_p->locks[i] != NULL) {
            //oc_utl_assert (refs_p->locks[i]->mode != CRT_RWSTATE_NONE);
            oc_crt_unlock(refs_p->locks[i]);
            refs_p->locks[i] = NULL;
        }
    }
}

void oc_utl_trk_finalize(Oc_wu *wu_pi)
{
    Oc_utl_trk_ref_set *refs_p = &wu_pi->rm_p->utl_rm.refs;

    oc_utl_assert(refs_p->sum == 0);
    oc_utl_assert(refs_p->cursor == 0);
}

/**********************************************************************/

