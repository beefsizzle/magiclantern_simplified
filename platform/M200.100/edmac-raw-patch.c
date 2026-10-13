/** \file
 * Raw LV EDMAC row length hook for 10-bit raw on M200 (CONFIG_EDMAC_RAW_PATCH).
 *
 * Based on theBilalFakhouri's M50_rawvideo branch (src/raw.c,
 * raw_lv_setedmac_hook / edmac_raw_adjust_pitch).
 *
 * Mem1Path (E07CB1D8) programs the raw EDMAC (write channel 3) every frame
 * with a row length of width * 14 / 8, whatever the PackUnpack bit depth.
 * 12-bit can be had by storing a different width (raw.c, mem1_set_bpp), but
 * at 2096 wide no width gives the exact 10-bit row length. So we hook
 * edmac_set_size (E0570E22) and shorten the row length of channel 3 while
 * 10-bit is active. The hook is installed (MMU remap) on the first request
 * and does nothing at 14-bit.
 */

#include <dryos.h>
#include <edmac.h>
#include <patch.h>

#if defined(CONFIG_EDMAC_RAW_PATCH)

#define EDMAC_SET_SIZE_ADDR 0xE0570E22  /* edmac_set_size(channel, struct edmac_info *) */
#define EDMAC_RAW_CHANNEL   3

/* bit depth the raw EDMAC row length is adjusted for; 14 = no change */
static volatile int edmac_raw_patch_bpp = 14;
static int edmac_raw_patch_installed = 0;

/* called from the hook with edmac_set_size's arguments */
void edmac_raw_adjust_pitch(uint32_t channel, struct edmac_info *info)
{
    int bpp = edmac_raw_patch_bpp;
    if (channel != EDMAC_RAW_CHANNEL || bpp == 14 || !info->xb || !info->yb)
        return;

    /* Canon's row length is for 14-bit */
    uint32_t width = info->xb * 8 / 14;
    info->xb = width * bpp / 8;
}

/* edmac_set_size starts with (8 bytes, replaced by our jump):
 *   E0570E22  push.w {r4-r11, lr}
 *   E0570E26  mov    r5, r0
 *   E0570E28  ldr    r0, [pc, ...]     ; = *(E057106C) = 0xE0E4B23C
 * and continues at E0570E2A. r3 is free there (2-argument function). */
void __attribute__((naked, noinline, aligned(4))) raw_lv_setedmac_hook(void)
{
    asm volatile(
        "push   {r0-r11, lr}\n"
        "sub    sp, #4\n"                   /* keep the stack 8-byte aligned */
        "movw   r3, #:lower16:edmac_raw_adjust_pitch\n"
        "movt   r3, #:upper16:edmac_raw_adjust_pitch\n"
        "blx    r3\n"                       /* edmac_raw_adjust_pitch(r0, r1) */
        "add    sp, #4\n"
        "pop    {r0-r11, lr}\n"

        /* the instructions we overwrote */
        "push   {r4-r11, lr}\n"
        "mov    r5, r0\n"
        "movw   r0, #0xB23C\n"
        "movt   r0, #0xE0E4\n"

        /* back to E0570E2A, Thumb */
        "movw   r3, #0x0E2B\n"
        "movt   r3, #0xE057\n"
        "bx     r3\n"
    );
}

static struct function_hook_patch edmac_raw_hook_def = {
    .patch_addr = EDMAC_SET_SIZE_ADDR,
    .orig_content = { 0x2d, 0xe9, 0xf0, 0x4f, 0x05, 0x46, 0x90, 0x48 },
    .target_function_addr = 0,      /* set at install time */
    .description = "raw EDMAC row length (10-bit)",
};
static struct patch edmac_raw_patch;
static uint8_t edmac_raw_hook_code[8];

/* returns 0 if the hook is in place */
int edmac_raw_patch_install()
{
    if (edmac_raw_patch_installed)
        return 0;

    edmac_raw_hook_def.target_function_addr = (uint32_t)raw_lv_setedmac_hook;
    if (convert_f_patch_to_patch(&edmac_raw_hook_def, &edmac_raw_patch, edmac_raw_hook_code))
        return -1;

    int err = apply_patches(&edmac_raw_patch, 1);
    if (err)
    {
        printf("raw EDMAC hook failed (%x)\n", err);
        return err;
    }

    edmac_raw_patch_installed = 1;
    return 0;
}

/* set the row length adjustment; call with interrupts off together with the
 * PackUnpack bit depth, so Canon never programs one without the other */
void edmac_raw_patch_set_bpp(int bpp)
{
    edmac_raw_patch_bpp = bpp;
}

int edmac_raw_patch_get_bpp()
{
    return edmac_raw_patch_installed ? edmac_raw_patch_bpp : 14;
}

#endif
