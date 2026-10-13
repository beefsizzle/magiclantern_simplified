/** \file
 * LiveView vsync for M200 (DIGIC 8) without state object hooks.
 *
 * ML normally gets one call per LV frame from a hook in Canon's EVF state
 * object (state-object.c). On M200 LiveView uses Canon's newer state
 * framework, so those hooks don't apply. Instead a high priority task polls
 * Canon's own LV frame counter (LV_FRAME_COUNTER_ADDR, see consts.h) and
 * runs the CBR_VSYNC module callbacks in task context. Read only. The
 * timing jitters by up to one DryOS tick (~10 ms).
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

    TASK_LOOP
    {
        if (!lv)
        {
            last = LV_FRAME_COUNTER;
            msleep(50);
            continue;
        }

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
