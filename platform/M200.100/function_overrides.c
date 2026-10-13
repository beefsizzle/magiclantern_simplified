/** \file
 * Function overrides needed for M200 1.0.0
 */
/*
 * Copyright (C) 2021 Magic Lantern Team
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
#include <property.h>
#include <bmp.h>
#include <config.h>
#include <consts.h>
#include <lens.h>
#include <edmac.h>

/*
 * Partition tables stuff. Got inlined in new generations, but this is a pretty standard one.
 * Struct copied from bootflags.c as there's no header to include right now.
 */

struct chs_entry
{
    uint8_t head;
    uint8_t sector; //sector + cyl_msb
    uint8_t cyl_lsb;
}__attribute__((packed));

struct partition
{
    uint8_t  state;
    struct   chs_entry start;
    uint8_t  type;
    struct   chs_entry end;
    uint32_t start_sector;
    uint32_t size;
}__attribute__((aligned,packed));

struct partition_table
{
    uint8_t  state; // 0x80 = bootable
    uint8_t  start_head;
    uint16_t start_cylinder_sector;
    uint8_t  type;
    uint8_t  end_head;
    uint16_t end_cylinder_sector;
    uint32_t sectors_before_partition;
    uint32_t sectors_in_partition;
}__attribute__((packed));

void fsuDecodePartitionTable(void * partIn, struct partition_table * pTable){
    struct partition * part = (struct partition *) partIn;
    pTable->state      = part->state;
    pTable->type       = part->type;
    pTable->start_head = part->start.head;
    pTable->end_head   = part->end.head;
    pTable->sectors_before_partition = part->start_sector;
    pTable->sectors_in_partition     = part->size;

    //tricky bits - TBD
    pTable->start_cylinder_sector = 0;
    pTable->end_cylinder_sector   = 0;
}

/** GUI **/

// From M6II.111: gui_init_end stub not identified on M6 II/M200 either.
void gui_init_end(void){ }

//Not sure if all args are uint.
extern void gui_enqueue_message(uint32_t, uint32_t, uint32_t, uint32_t);
void GUI_Control(int bgmt_code, int obj, int arg, int unknown){
   gui_enqueue_message(0, bgmt_code, obj, arg);
}

// fake WINSYS_BMP_DIRTY_BIT_NEG
int winsys_bmp_dirty_bit_neg = 0;

/* M200 has no synchronous _LoadCalendarFromRTC(tm, 0, 0, 16). Wrap the async
 * read the same way Canon's FA_GetCalendar (E006E43A) does: request with
 * (0, 0x10, 0, cbr, buf), the callback copies 0x28 bytes (DryOS struct tm)
 * into buf and gives a semaphore. buf is static so a callback arriving after
 * a timeout can't write into a stale stack frame. */
extern void _RTC_GetCalendarAsync(int a, int b, int c,
                                  void (*cbr)(void *dst, const void *src),
                                  void *dst);
static struct semaphore *rtc_done_sem = NULL;
static struct semaphore *rtc_lock = NULL;
static struct tm rtc_buf;

static void rtc_calendar_cbr(void *dst, const void *src)
{
    memcpy(dst, src, sizeof(struct tm));
    give_semaphore(rtc_done_sem);
}

void LoadCalendarFromRTC(struct tm *tm)
{
    if (!rtc_lock)
    {
        rtc_lock = create_named_semaphore("ml_rtc_lock", 1);
        rtc_done_sem = create_named_semaphore("ml_rtc_done", 0);
    }

    memset(tm, 0, sizeof(*tm));
    if (take_semaphore(rtc_lock, 2000) != 0)
        return;

    _RTC_GetCalendarAsync(0, 0x10, 0, rtc_calendar_cbr, &rtc_buf);
    if (take_semaphore(rtc_done_sem, 1000) == 0)
        memcpy(tm, &rtc_buf, sizeof(*tm));

    give_semaphore(rtc_lock);
}

int get_task_info_by_id(int unknown_flag, int task_id, void *task_attr)
{
    // task_id is something like two u16s concatenated.  The flag argument,
    // present on D45 but not on D678 allows controlling if the task info request
    // uses the whole thing, or only the low half.
    //
    // ML calls with this set to 1, meaning task_id is used as is,
    // if 0, the high half is masked out first.
    //
    // D678 doesn't have the 1 option, we use the low half as index
    // to find the full value.
    struct task *task = first_task + (task_id & 0xffff);
    return _get_task_info_by_id(task->taskId, task_attr);
}

/** File I/O **/

/**
* _FIO_GetFileSize returns now 64bit size in form of struct.
* This probably should be integrated into fio-ml for CONFIG_DIGIC_VIII
*/
extern int _FIO_GetFileSize64(const char *, void *);
int _FIO_GetFileSize(const char * filename, uint32_t * size){
   uint32_t size64[2];
   int code = _FIO_GetFileSize64(filename, &size64);
   *size = size64[0]; //return "lower" part
   return code;
}

/** WRONG: temporary overrides to get CONFIG_HELLO_WORLD working **/

void SetEDmac(unsigned int channel, void *address, struct edmac_info *ptr, int flags)
{
   return;
}

void ConnectWriteEDmac(unsigned int channel, unsigned int where)
{
   return;
}

void ConnectReadEDmac(unsigned int channel, unsigned int where)
{
   return;
}

void StartEDmac(unsigned int channel, int flags)
{
   return;
}

void AbortEDmac(unsigned int channel)
{
   return;
}

void RegisterEDmacAbortCBR(int channel, void (*cbr)(void*), void* cbr_ctx)
{
   return;
}

void UnregisterEDmacAbortCBR(int channel)
{
   return;
}

void RegisterEDmacPopCBR(int channel, void (*cbr)(void*), void* cbr_ctx)
{
   return;
}

void UnregisterEDmacPopCBR(int channel)
{
   return;
}

void _EngDrvOut(uint32_t reg, uint32_t value)
{
   return;
}

// No shamem on D8; read the registers directly (as M50_rawvideo).
// Only 0xDxxxxxxx (D8 MMIO); anything else comes from legacy code, log it.
uint32_t shamem_read(uint32_t addr)
{
    if (addr >> 28 != 0xD)
    {
        DryosDebugMsg(0, 15, "shamem_read from %08x - aborted", addr);
        return 0;
    }
    return *(volatile uint32_t *)addr;
}

void _engio_write(uint32_t* reg_list)
{
   return;
}
