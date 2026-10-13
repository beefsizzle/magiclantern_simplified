/** \file
 * LiveView vsync for M200 (DIGIC 8) without state object hooks.
 *
 * ML normally gets one call per LV frame from a hook in Canon's EVF state
 * object (state-object.c). On M200 LiveView uses Canon's newer state
 * framework, so those hooks don't apply. Instead a high priority task runs
 * the CBR_VSYNC module callbacks in task context, timed by:
 * - with raw LV: the end of each raw frame (Mem1Path's EDMAC completion
 *   hook, see below), so mlv_lite's copy from Canon's single raw buffer
 *   starts right after the frame is complete;
 * - otherwise: Canon's own LV frame counter (LV_FRAME_COUNTER_ADDR), polled,
 *   which jitters by up to one DryOS tick (~10 ms). Read only.
 */
/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <dryos.h>
#include <tasks.h>
#include <module.h>
#include <propvalues.h>
#include <consts.h>

#if defined(CONFIG_LV_VSYNC_POLL) && !defined(CONFIG_STATE_OBJECT_HOOKS)

#define LV_FRAME_COUNTER (*(volatile uint32_t *)LV_FRAME_COUNTER_ADDR)

static volatile int vsync_counter = 0;
static volatile int last_frame_ms = 0;

#ifdef RAW_LV_MEM1_STATE_ADDR
/* Raw frame done. Mem1Path's EDMAC ch3 completion callback (E07CACDC) does
 * if (S+0x64) S+0x64(S+0x68). Canon never sets that slot (no writer in the
 * ROM, 0 at runtime), so ML can use it without patching code. It runs in
 * the EDMAC interrupt right after each raw frame is in memory. Timing the
 * copy from the polled frame counter instead tore ~40% of the frames: the
 * copy crossed Canon writing the next frame into the same buffer. */
#define MEM1_DONE_HOOK (RAW_LV_MEM1_STATE_ADDR + 0x64)
#define MEM1_DONE_CTX  (RAW_LV_MEM1_STATE_ADDR + 0x68)

static struct semaphore * raw_done_sem = NULL;
static volatile uint32_t raw_done_count = 0;

static void mem1_frame_done(void * ctx)
{
    raw_done_count++;
    give_semaphore(raw_done_sem);
}

/* take the free hook slot (again, if Canon ever clears its struct) */
static void mem1_hook_check()
{
    if (MEM(MEM1_DONE_HOOK) == 0)
    {
        uint32_t old = cli();
        MEM(MEM1_DONE_CTX) = 0;
        MEM(MEM1_DONE_HOOK) = (uint32_t) &mem1_frame_done;
        sei(old);
    }
}
#endif

/* one LV frame: run the module callbacks */
static void lv_vsync_frame()
{
    last_frame_ms = get_ms_clock();
    vsync_counter++;

    #if defined(CONFIG_MODULES)
    module_exec_cbr(CBR_VSYNC);
    #endif
}

static void lv_vsync_task()
{
    uint32_t last = LV_FRAME_COUNTER;

    #ifdef RAW_LV_MEM1_STATE_ADDR
    raw_done_sem = create_named_semaphore("raw_done", 0);
    mem1_hook_check();
    uint32_t raw_seen = raw_done_count;
    int raw_last_ms = 0;
    #endif

    TASK_LOOP
    {
        if (!lv)
        {
            last = LV_FRAME_COUNTER;
            msleep(50);
            continue;
        }

        #ifdef RAW_LV_MEM1_STATE_ADDR
        mem1_hook_check();

        /* raw frames arriving: time the frames from the raw hook */
        if (raw_done_count != raw_seen || get_ms_clock() - raw_last_ms < 200)
        {
            if (take_semaphore(raw_done_sem, 100) != 0)
                continue;               /* none for 100 ms: back to the counter soon */

            int missed = (int)(raw_done_count - raw_seen) - 1;
            raw_seen = raw_done_count;
            raw_last_ms = get_ms_clock();
            last = LV_FRAME_COUNTER;

            /* gives queued while we were busy are late: drop them */
            if (missed > 0)
                while (take_semaphore(raw_done_sem, 1) == 0) {}

            lv_vsync_frame();
            continue;
        }
        #endif

        /* the counter is reset on Canon's evfdevSetPath (mode changes) */
        uint32_t now = LV_FRAME_COUNTER;
        if (now != last)
        {
            last = now;
            lv_vsync_frame();
        }

        msleep(1);
    }
}

TASK_CREATE("lv_vsync_task", lv_vsync_task, 0, 0x12, 0x2000);

/* waits for N LiveView frames; returns 0 on timeout or if LV was closed */
int wait_lv_frames(int num_frames)
{
    int fps = video_mode_fps > 0 ? video_mode_fps : 30;
    int frame_ms = 1000 / fps + 1;

    /* no frame seen recently (counter not running in this LV mode?):
     * fall back to timing */
    if (get_ms_clock() - last_frame_ms > 500)
    {
        msleep(num_frames * frame_ms + 20);
        return 1;
    }

    int start = vsync_counter;
    int timeout = get_ms_clock() + num_frames * frame_ms * 2 + 100;
    while (vsync_counter - start < num_frames)
    {
        if (!lv || get_ms_clock() > timeout)
            return 0;
        msleep(5);
    }
    return 1;
}

#endif
