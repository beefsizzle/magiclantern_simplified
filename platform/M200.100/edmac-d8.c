/** \file
 * EDMAC mem2mem copy for M200 (DIGIC 8), the part of edmac-memcpy.c that
 * mlv_lite needs.
 *
 * Based on platform/M50.110/edmac-d8.c from theBilalFakhouri's M50_rawvideo
 * branch (kitor / Magic Lantern Team), which uses R180 EFsVcopy channels.
 * M200 instead mirrors Canon's own mem2mem use on this camera:
 * DarkCurCor_CopyOB (FUN_e0651a9a) via Engine::MemoryToMemoryEsub5.c
 * (init FUN_e066b2c8, start FUN_e066b382, term FUN_e066b352), with its ROM
 * constants: channels and resources at 0xE0FB64D4/0xE0FB64DC, boomer
 * selector base 0xC2060A + (read channel - 0x3A), sub chip list at
 * 0xE0FB6844. Completion is signalled with a semaphore instead of an
 * event flag (no event flag stubs needed).
 */
/*
 * Copyright (C) 2024 Magic Lantern Team
 *
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
#include <edmac.h>

/* Sub chips must be woken before touching their EDMAC MMIO, or the camera
 * hard locks. Lists are terminated with 7. */
extern void PwrMng_WakeSubChips(const uint32_t *list);
extern void PwrMng_SuspendSubChips(const uint32_t *list);

extern void edmac_reset_channel(uint32_t channel);
extern void edmac_reset_boomer_vdkick(uint32_t channel);
extern void edmac_reset_packunpack_mode(uint32_t channel);
extern void edmac_set_address(uint32_t channel, void *addr);
extern void edmac_set_size(uint32_t channel, struct edmac_info *edmac_info);
extern void edmac_set_transfer_mode(uint32_t channel, uint32_t mode);
extern void edmac_select_boomer(uint32_t channel, uint32_t flags);
extern void StartEDmac_maybe(uint32_t channel);
extern void ConnectReadEDmac_maybe(uint32_t rd_channel);
extern void edmac_stop_boomer_maybe(uint32_t channel);

/* Canon's DarkCurCor_CopyOB mem2mem setup on M200 */
static const uint32_t mem2mem_devices[2] = {4, 7};
static uint32_t mem2mem_resources[2] = {0x5001F, 0x50023};
#define MEM2MEM_RD_CH 0x3D
#define MEM2MEM_WR_CH 0x18
#define MEM2MEM_BOOMER_SELECTOR (0xC2060A + (MEM2MEM_RD_CH - 0x3A))
#define MEM2MEM_MODE 0x0 // as M50 edmac-d8.c (1 - 32bit, 2 - 64bit, 3 - 128bit)
#define MEM2MEM_WAIT_MS 50

static struct LockEntry *mem2mem_lock;
static struct semaphore *mem2mem_done;
static volatile uint32_t mem2mem_status;

static void mem2mem_CBR(void *arg)
{
    uint32_t old_irq = cli();
    mem2mem_status |= (uint32_t)arg;
    uint32_t status = mem2mem_status;
    sei(old_irq);
    if (status == 3)
        give_semaphore(mem2mem_done);
}

/* returns 0 on success */
uint32_t mem2mem_edmac_copy_d8(void *src, void *dst, struct edmac_info *src_info, struct edmac_info *dst_info)
{
    if (!mem2mem_done)
        mem2mem_done = create_named_semaphore("mem2mem_d8", 0);

    /* same order as Canon's MemoryToMemoryEsub5 init */
    edmac_reset_channel(MEM2MEM_WR_CH);
    edmac_reset_channel(MEM2MEM_RD_CH);
    edmac_reset_boomer_vdkick(MEM2MEM_WR_CH);
    edmac_select_boomer(MEM2MEM_WR_CH, MEM2MEM_BOOMER_SELECTOR);

    mem2mem_status = 0;
    RegisterEDmacCompleteCBR(MEM2MEM_WR_CH, mem2mem_CBR, (void *)2);
    RegisterEDmacCompleteCBR(MEM2MEM_RD_CH, mem2mem_CBR, (void *)1);
    edmac_reset_packunpack_mode(MEM2MEM_WR_CH);
    edmac_reset_packunpack_mode(MEM2MEM_RD_CH);

    edmac_set_address(MEM2MEM_WR_CH, dst);
    edmac_set_address(MEM2MEM_RD_CH, src);
    edmac_set_size(MEM2MEM_WR_CH, dst_info);
    edmac_set_size(MEM2MEM_RD_CH, src_info);
    edmac_set_transfer_mode(MEM2MEM_WR_CH, MEM2MEM_MODE);
    edmac_set_transfer_mode(MEM2MEM_RD_CH, MEM2MEM_MODE);

    StartEDmac_maybe(MEM2MEM_WR_CH);
    StartEDmac_maybe(MEM2MEM_RD_CH);
    ConnectReadEDmac_maybe(MEM2MEM_RD_CH);

    take_semaphore(mem2mem_done, MEM2MEM_WAIT_MS);

    /* same order as Canon's term */
    UnregisterEDmacCompleteCBR(MEM2MEM_RD_CH);
    UnregisterEDmacCompleteCBR(MEM2MEM_WR_CH);
    edmac_stop_boomer_maybe(MEM2MEM_WR_CH);

    return mem2mem_status == 3 ? 0 : 1;
}

void edmac_memcpy_res_lock()
{
    mem2mem_lock = CreateResLockEntry(mem2mem_resources, COUNT(mem2mem_resources));
    LockEngineResources(mem2mem_lock);
    PwrMng_WakeSubChips(mem2mem_devices);
}

void edmac_memcpy_res_unlock()
{
    PwrMng_SuspendSubChips(mem2mem_devices);
    UnLockEngineResources(mem2mem_lock);
}

// reimplement this wonderful function with mem2mem_edmac_copy_d8
void* edmac_copy_rectangle_cbr_start(void *dst, void *src,
                                     int src_width, int src_x, int src_y,
                                     int dst_width, int dst_x, int dst_y,
                                     int w, int h,
                                     void (*cbr_r)(void *), void (*cbr_w)(void *), void *cbr_ctx)
{
    if ((src == NULL) || (dst == NULL))
        return NULL;

    /* clean the cache before reading from regular (cacheable) memory */
    if (src != UNCACHEABLE(src))
        sync_caches();

    /* see M50 edmac-d8.c: masking with 0x1FFFFFFF gave corrupted frames */
    uint32_t src_adjusted = ((uint32_t)src) + src_x + src_y * src_width;
    uint32_t dst_adjusted = ((uint32_t)dst) + dst_x + dst_y * dst_width;

    struct edmac_info src_region = {
        .off1b = src_width - w,
        .xb = w,
        .yb = h - 1,
    };

    struct edmac_info dst_region = {
        .off1b = 0,
        .xb = w,
        .yb = h - 1,
    };

    uint32_t err = mem2mem_edmac_copy_d8((void *)src_adjusted, (void *)dst_adjusted, &src_region, &dst_region);
    if (cbr_w)
        cbr_w(cbr_ctx); // clears edmac_active, we have no CBR to do this
    return err ? NULL : dst;
}

// dummy stubs to make mlv_lite happy
void edmac_copy_rectangle_adv_cleanup()
{
}

uint32_t edmac_read_chan = 0x6; // dummy, don't use
uint32_t edmac_write_chan = 0x6; // dummy, don't use
