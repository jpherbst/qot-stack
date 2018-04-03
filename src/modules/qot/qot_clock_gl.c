/*
 * @file qot_clock_gl.c
 * @brief Interface to clock management for global timelines in the QoT stack
 * @author Sandeep D'souza
 *
 * Copyright (c) Carnegie Mellon University 2018.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <linux/hrtimer.h>

#include "qot_clock_gl.h"
#include "qot_admin.h"

/* Spinlock for Global Clock */
static spinlock_t qot_clock_gl_lock;

/* Global clock parameters */
tl_translation_t clkgl_params;

/* Public functions */

/* read the disciplined global time */
qot_return_t qot_clock_gl_get_time(utimepoint_t *utp)
{
    ktime_t now_kt;
    s64 u_timelinetime;
    s64 l_timelinetime;
    unsigned long flags;
    utimelength_t sync_uncertainty; 
    s64 now, ns;
    if (!utp)
        return QOT_RETURN_TYPE_ERR;

    /* Get a measurement of global core time (CLOCK_REALTIME)*/
    spin_lock_irqsave(&qot_clock_gl_lock, flags);
    now_kt = ktime_get_real();
    ns = ktime_to_ns(now_kt);
    now = clkgl_params.nsec + (ns - clkgl_params.last)
          + div_s64(clkgl_params.mult * (ns - clkgl_params.last),1000000000L); 
    TP_FROM_nSEC(utp->estimate, now);
    TL_FROM_uSEC(utp->interval.below, 0);
    TL_FROM_uSEC(utp->interval.above, 0);

    /* Calculate sync uncertainty */
    u_timelinetime = div_s64(clkgl_params.u_mult*(ns - clkgl_params.last),1000000000L) + clkgl_params.u_nsec;
    l_timelinetime = div_s64(clkgl_params.l_mult*(ns - clkgl_params.last),1000000000L) + clkgl_params.l_nsec;

    spin_unlock_irqrestore(&qot_clock_gl_lock, flags);
    
    sync_uncertainty.estimate.sec = 0;
    sync_uncertainty.estimate.asec = 0;

    if(u_timelinetime > 0)
        TL_FROM_nSEC(sync_uncertainty.interval.above, u_timelinetime);
    else
        TL_FROM_nSEC(sync_uncertainty.interval.above, 0);

    if(l_timelinetime < 0)
        TL_FROM_nSEC(sync_uncertainty.interval.below, -l_timelinetime);
    else
        TL_FROM_nSEC(sync_uncertainty.interval.below, 0);

    utimepoint_add(utp, &sync_uncertainty);
    /* Success */
    return QOT_RETURN_TYPE_OK;
}

/* Read the raw CLOCK_REALTIME */
qot_return_t qot_clock_gl_get_time_raw(timepoint_t *tp)
{
    struct timespec now;
    if (!tp)
        return QOT_RETURN_TYPE_ERR;
    /* Get a measurement of core time */
    ktime_get_real_ts(&now);
    timepoint_from_timespec(tp, &now);
    /* Success */
    return QOT_RETURN_TYPE_OK;
}

/* Adjust the global clock frequency */
qot_return_t qot_clock_gl_adjfreq(s32 ppb)
{
    timepoint_t tp;
    s64 ns;
    unsigned long flags;
    spin_lock_irqsave(&qot_clock_gl_lock, flags);
    if (qot_clock_gl_get_time_raw(&tp))
    {
        spin_unlock_irqrestore(&qot_clock_gl_lock, flags);
        return QOT_RETURN_TYPE_ERR;
    }
    ns = TP_TO_nSEC(tp);
    clkgl_params.nsec += (ns - clkgl_params.last)
        + div_s64(clkgl_params.mult * (ns - clkgl_params.last),1000000000L); // ULL Changed to L -> Sandeep
    clkgl_params.last = ns;
    clkgl_params.mult = (s64) ppb; 
    spin_unlock_irqrestore(&qot_clock_gl_lock, flags);
    pr_info("qot_clock_gl: Frequency adjusted to %ld\n", ppb);
    return QOT_RETURN_TYPE_OK;
}

/* Adjust the global clock time */
qot_return_t qot_clock_gl_adjtime(s64 delta)
{
    timepoint_t tp;
    s64 ns;
    unsigned long flags;
    spin_lock_irqsave(&qot_clock_gl_lock, flags);
    if (qot_clock_gl_get_time_raw(&tp))
    {
        spin_unlock_irqrestore(&qot_clock_gl_lock, flags);
        return QOT_RETURN_TYPE_ERR;
    }
    ns = TP_TO_nSEC(tp);
    clkgl_params.nsec += delta; 
    spin_unlock_irqrestore(&qot_clock_gl_lock, flags);
    pr_info("qot_clock_gl: Offset added %lld\n", delta);
    return QOT_RETURN_TYPE_OK;
}

/* Set the global core clock time*/
qot_return_t qot_clock_gl_settime(timepoint_t tp)
{
    s64 ns;
    timepoint_t now_tp;
    unsigned long flags;

    spin_lock_irqsave(&qot_clock_gl_lock, flags);
    if (qot_clock_gl_get_time_raw(&now_tp))
    {
        spin_unlock_irqrestore(&qot_clock_gl_lock, flags);
        return QOT_RETURN_TYPE_ERR;
    }

    ns = TP_TO_nSEC(now_tp);
    clkgl_params.last = ns;
    clkgl_params.nsec = TP_TO_nSEC(tp);
    spin_unlock_irqrestore(&qot_clock_gl_lock, flags); 
    return 0;
}

/* Set the global clock uncertainty */
qot_return_t qot_clock_gl_set_uncertainty(qot_bounds_t bounds)
{
    unsigned long flags;
    spin_lock_irqsave(&qot_clock_gl_lock, flags);
    clkgl_params.u_mult = (s32) bounds.u_drift; 
    clkgl_params.l_mult = (s32) bounds.l_drift; 
    clkgl_params.u_nsec = (s64) bounds.u_nsec;
    clkgl_params.l_nsec = (s64) bounds.l_nsec;
    spin_unlock_irqrestore(&qot_clock_gl_lock, flags);  
    return QOT_RETURN_TYPE_OK;
}

/* Get the global timeline mapping parameters */
qot_return_t qot_clock_gl_get_params(tl_translation_t *params)
{
    unsigned long flags;
    if (!params)
        return QOT_RETURN_TYPE_ERR;

    spin_lock_irqsave(&qot_clock_gl_lock, flags);
    *params = clkgl_params;
    spin_unlock_irqrestore(&qot_clock_gl_lock, flags);  
    return QOT_RETURN_TYPE_OK;
}


/* Project from global clock to global timeline */
qot_return_t qot_gl_loc2rem(int period, s64 *val)
{
    unsigned long flags;
    spin_lock_irqsave(&qot_clock_gl_lock, flags);
    if (period)
        *val += div_s64(clkgl_params.mult * (*val), 1000000000L);
    else
    {
        *val -= (s64) clkgl_params.last;
        *val  = clkgl_params.nsec + (*val) + div_s64(clkgl_params.mult * (*val), 1000000000L);
    }
    spin_unlock_irqrestore(&qot_clock_gl_lock, flags);  
    return QOT_RETURN_TYPE_OK;
}

/* Project from global timeline to global clock */
qot_return_t qot_gl_rem2loc(int period, s64 *val)
{
    u32 rem;
    unsigned long flags;
    spin_lock_irqsave(&qot_clock_gl_lock, flags);

    if (period)
    {
        *val = (s64) div_u64_rem((u64)(*val), (u32) (clkgl_params.mult + 1000000000LL), &rem)*1000000000LL ; 
        *val += (s64) rem; 
    }
    else
    {
        u64 diff = (u64)(*val - clkgl_params.nsec);
        u64 quot = div_u64_rem(diff, (u32)(clkgl_params.mult + 1000000000LL), &rem); 
        *val = clkgl_params.last + (s64)(quot * 1000000000ULL) + (s64) rem; 
    }
    spin_unlock_irqrestore(&qot_clock_gl_lock, flags);  
    return QOT_RETURN_TYPE_OK;
}

/* Cleanup the global core clock */
qot_return_t qot_clock_gl_cleanup(void)
{
    return QOT_RETURN_TYPE_OK;
}

/* Initialize the global clock */
qot_return_t qot_clock_gl_init(void)
{
    /* Initialize Global Clock Mapping Parameters */
    clkgl_params.mult = 0;
    clkgl_params.last = 0;
    clkgl_params.nsec = 0;
    clkgl_params.u_nsec = 0;
    clkgl_params.l_nsec = 0;
    clkgl_params.u_mult = 0;
    clkgl_params.l_mult = 0;
    spin_lock_init(&qot_clock_gl_lock);
    return QOT_RETURN_TYPE_OK;
}


