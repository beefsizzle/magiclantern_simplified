/*
 *  EOS M200 1.0.0 consts
 *
 *  Started from platform/M50.110/consts.h. Values with an M200 comment were
 *  found in the M200 ROM (Canon's own callers / strings) and checked on the
 *  camera; values marked WRONG are placeholders, as on other D8 ports.
 */

/* Card LED not identified yet (LEDDrv.c is table driven). Leaving
 * CARD_LED_ADDRESS undefined means ML never pokes an unverified GPIO. */
//#define CARD_LED_ADDRESS
//#define LEDON
//#define LEDOFF

#define BR_DCACHE_CLN_1     0xE0040068   /* first call to dcache_clean, before cstart */
#define BR_ICACHE_INV_1     0xE0040072   /* first call to icache_invalidate, before cstart */
#define BR_DCACHE_CLN_2     0xE00400A0   /* second call to dcache_clean, before cstart */
#define BR_ICACHE_INV_2     0xE00400AA   /* second call to icache_invalidate, before cstart */
#define BR_BR_CSTART        0xE00400C0   // ldr r0,=cstart; orr r0,#1; bx r0 - patched to relative branch
#define BR_BZERO32          0xE0040152   /* called from cstart */
#define BR_CREATE_ITASK     0xE00401B4   /* called from cstart */

// cstart ends at E00401E8 (literal pool), helper FUN_e00401e8 follows,
// init_task starts at E0040236. Copy everything up to there.
#define FIRMWARE_ENTRY_LEN 0x240
// Ximr context: refreshVrmsSurface passes *0x748F4 to XCM_MakeContext, and
// XCM_GetContext(xcm, 0) returns xcm + 0x10. Same layout as M6 II.
#define XCM_PTR *(unsigned int *)0x748F4
#define XIMR_CONTEXT ((unsigned int *)(XCM_PTR + 0x10))
#define CANON_ORIG_MMU_TABLE_ADDR 0xe0000000 // M200: copy_mmu_tables (E005B090) copies 0x4900 bytes
                                             // from 0xE0000000 to the runtime tables (0xBFF20000).

#define DRYOS_SGI_HANDLERS_PTR 0x402c // M200: IRQ dispatcher FUN_e019a6bc reads GIC handler table from *(0x4028+4), 8-byte {func,arg} entries

/* "Malloc Information" */
#define MALLOC_STRUCT_ADDR 0x29E6C // M200: malloc_info (E03C025C) -> FUN_e05bd974 reads this struct; total +8, allocated +0x1C
//#define MALLOC_FREE_MEMORY (MEM(MALLOC_STRUCT + 8) - MEM(MALLOC_STRUCT + 0x1C)) // "Total Size" - "Allocated Size"

/* high confidence */
#define DRYOS_ASSERT_HANDLER        0x4000               // M200: assert FUN_e0677818 calls *(0x4000) if set

#define CURRENT_GUI_MODE (*(int*)0x7BC4)  // M200: SetGUIRequestMode base 0x7B60; FUN_e05442f2 commits the mode to +0x64
                                          // after checking transitions from 0x2002/0x2003 (requested mode is at +0x6C)

// M200 uses the new GUI mode encoding like M6 II (values compared in FUN_e05442f2)
#define GUIMODE_PLAY 0x2002
#define GUIMODE_MENU 0x2003
// In LV use the LV Q-dialog mode 0xC048 (as M6II): Canon's LiveView Q dialogs
// (DlgLiveViewMetering/AfMode/Wb) return to it; 27 SetGUIRequestMode call sites.
// Plain menu mode in LV stops LiveView and crashes Canon's EvfCap task
// (EvfCapState.c:405/510 nullptr asserts).
#define GUIMODE_ML_MENU (lv ? 0xC048 : GUIMODE_MENU)

#define CANON_SHUTTER_RATING 100000

#define GMT_FUNCTABLE               0xE092E6EC           // M200: gui_main_task (E022FB34) copies 0x1C bytes from here
#define GMT_NFUNCS                  0x7                  // M200: 0x1C / 4; gui_main_task asserts event type < 7

// M200: LVAE eventprocs are registered from RAM code copied from ROM
// (RAM = ROM - 0xDF0E2B70). All handlers use the same base, 0x4B850:
//   lvae_setcontrolbv          E132AF86: str flag,[+0x28]; strh bv,[+0x3C]
//   lvae_setcontrolaeparam     E132B028: strh tv/av/iso,[+0x3E/+0x40/+0x42]; zero,[+0x44]
//   lvae_setcontrolaccumh      E132B158: strh,[+0x44]
//   lvae_setdispgain           E132B652: strh,[+0x70]
//   lvae_setmoviemanualcontrol E132B1EE: str,[+0x24]
// Same offsets as M6II.111.
#define LVAE_STRUCT                 0x4B850
#define CONTROL_BV      (*(uint16_t*)(LVAE_STRUCT+0x28)) // via "lvae_setcontrolbv"
#define CONTROL_BV_TV   (*(uint16_t*)(LVAE_STRUCT+0x3E)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_AV   (*(uint16_t*)(LVAE_STRUCT+0x40)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ISO  (*(uint16_t*)(LVAE_STRUCT+0x42)) // via "lvae_setcontrolaeparam"
#define CONTROL_BV_ZERO (*(uint16_t*)(LVAE_STRUCT+0x44)) // via "lvae_setcontrolaccumh"
#define LVAE_DISP_GAIN  (*(uint16_t*)(LVAE_STRUCT+0x70)) // via "lvae_setdispgain"
#define LVAE_MOV_M_CTRL (*(uint8_t* )(LVAE_STRUCT+0x24)) // via "lvae_setmoviemanualcontrol"

/*
 * kitor: ISO related stuff is not in LVAE struct anymore?
 * iso-related stuff calls 0x02275de which returns pointer at 0x02276168 to 0x6b818
 */
// M200: M50 value, unverified and unused by src/; left undefined.
//#define LVAE_ISO_STRUCT 0x8e660
//#define LVAE_ISO_MIN    (*(uint8_t* )LVAE_ISO_STRUCT + 0x0E ) // via string: ISOMin:%d

//#define LVAE_ISO_HIS    (*(uint8_t* )(LVAE_STRUCT+0xXX)) // no idea, not referenced in ./src?!
//#define LVAE_ISO_SPEED  (*(uint8_t* )(LVAE_STRUCT+0xXX))  //WRONG, not sure how to follow

/*
 * kitor: DIGIC 8 has no PROP_LV_OUTPUT_TYPE (PROP_HOUTPUT_TYPE in ML source)
 * I looked around LiveViewApp and found `LvInfoToggle_Update()` which updates
 * variable to represent currently display overlays. Look at R conts.h for more
 * details.
 */
// LvInfoToggle_Update (E0714740) toggle index lives at MEM(0x14F74). INFO
// cycles through 1, 2, 3, 6, 0; 3 is Canon's clean screen (as on R) and 6 the
// info-only screen without LV image.
#define M200_LV_INFO_TOGGLE MEM(0x14F74)
#define LV_OVERLAYS_MODE M200_LV_INFO_TOGGLE

/* There's no `DispOperator_PropertyMasterSetDisplayTurnOffOn()` like on EOSR.
 * PropID 80040083 is also not referenced anywhere.
 * GUILockTask does DISP_MuteOnOSDLineVram() and DISP_MuteOnOSDPanelVram()
 * so maybe values stored by those two combined can be used instead. */
#define DISPLAY_IS_ON               1

/*
 * On M50 UYVY Image buffers are hardcoded to be (see VramRead e009ccd6):
 * for EVF   : 1024x768
 * for Panel : 736x480, 720x480 is active image area
 * for HDMI  : 736x480, 1280x720, 1920x1080, 3840x2160 depending on output.
 *             HDMI resolution is selected based on type field from DispDev
*              structure (8e00 + 0x18)
 * HDMI is referenced as "Line" in Canon functions.
 *
 * Buffers used for regular display are available in smemShowFix output.
 * IMG_VRAM1, IMG_VRAM2, IMG_VRAM3
 *
 * Different to R there's no Clean HDMI output. But DispVram looks similar to
 * R, so who knows...
 */

#define YUV422_LV_BUFFER_1   0x7F41C000
#define YUV422_LV_BUFFER_2   0x7F810800
#define YUV422_LV_BUFFER_3   0x7FC05000

// Offsets found via DISP_GetImageAddress() helper that prints
// "CurrentImgAddr : %#08x"
#define DISP_VRAM_STRUCT_PTR *(unsigned int *)0xA3F8            // M200: DISP_GetImageAddress-like helper FUN_e00be312 reads *0xA3F8
#define DV_DISP_TYPE  *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xC))   // Display type mask
#define DV_VRAM_LINE  *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xA4))  // M200: FUN_e00be312 slot 2; same offsets as M6II
#define DV_VRAM_PANEL *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xAC))  // M200: FUN_e00be312 slot 0; same offsets as M6II
#define DV_VRAM_EVF   *((unsigned int *)(DISP_VRAM_STRUCT_PTR + 0xB4))  // M200: FUN_e00be312 slot 1 (M200 has no EVF port)

/* Hardcoded to Panel for now. It would be easier if we can replace this with a
 * function call that would be put into functon_overrides.c. Then we could just
 * define full structs there instead of playing with pointers */
#define YUV422_LV_BUFFER_DISPLAY_ADDR DV_VRAM_PANEL
#define YUV422_LV_PITCH               736       // depends on display type

#define YUV422_HD_BUFFER_DMA_ADDR 0x0 // it expects this to be shamem_read(some_DMA_ADDR)

// LiveView frame counter (read only). Canon's EvfDev registers a base frame
// rate callback (RegisterBaseFrameRateCBR E067BB46, from evfdevSyncStart
// E0081BBA) at E0081528, which increments 0x9308+0xC (FUN_e068db3a) once per
// LV frame and posts event 0xD to EVFDEV. Cleared on evfdevSetPath
// (FUN_e068db2c), read by FUN_e068db34. Used as vsync source (lv-vsync.c).
#define LV_FRAME_COUNTER_ADDR 0x9314

/* WRONG! */
#define HALFSHUTTER_PRESSED         0
/* kitor: I was unable to find any related stuff from 200D
 * Working theory: since R is LV all-the-time, maybe it's not special anymore
 * and is handled by MPU now?
 */

//Replaced by CONFIG_NO_BFNT in internals.h
//#define BFNT_CHAR_CODES             0x00000000
//#define BFNT_BITMAP_OFFSET          0x00000000
//#define BFNT_BITMAP_DATA            0x00000000

#define NUM_PICSTYLES 11

#define AUDIO_MONITORING_HEADPHONES_CONNECTED 0
#define INFO_BTN_NAME               "INFO"
#define Q_BTN_NAME                  "Q/SET"

#define MIN_MSLEEP 11
#define PLAY_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_PLAY)
#define MENU_MODE (gui_state == GUISTATE_PLAYMENU && CURRENT_GUI_MODE == GUIMODE_MENU)

/* WRONG: copied straight from 200d/50d */
// Definitely wrong / hacks / no testing at all:
    #define IMGPLAY_ZOOM_LEVEL_ADDR (0x2CBC) //wrong

extern int winsys_bmp_dirty_bit_neg;
#define WINSYS_BMP_DIRTY_BIT_NEG MEM(&winsys_bmp_dirty_bit_neg) // faked via function_overrides.c
#define FOCUS_CONFIRMATION 0 // wrong, but safe, no focus lock
    #define LV_BOTTOM_BAR_DISPLAYED 0x0 // wrong, fake bool
// this block all copied from 50D, and probably wrong, though likely safe
    #define FASTEST_SHUTTER_SPEED_RAW 160
    #define MAX_AE_EV 2
    #define FLASH_MAX_EV 3
    #define FLASH_MIN_EV -10
    #define COLOR_FG_NONLV 80
    #define AF_BTN_HALFSHUTTER 0
    #define AF_BTN_STAR 2
// another block copied from 50D
    #define GUIMODE_WB 5
    #define GUIMODE_FOCUS_MODE 9
    #define GUIMODE_DRIVE_MODE 8
    #define GUIMODE_PICTURE_STYLE 4
    #define GUIMODE_Q_UNAVI 0x18
    #define GUIMODE_FLASH_AE 0x22
    #define GUIMODE_PICQ 6

// M200: MVR struct not identified yet. The M50 pointer (*(void**)0x1ed8)
// points at unrelated memory here, so these read as 0.
    #define MVR_190_STRUCT ((void *)0) // TODO M200: look in MVR_Initialize for the AllocateMemory result
#define div_maybe(a,b) ((a)/(b))
    // see mvrGetBufferUsage, which is not really safe to call => err70
    // macros copied from arm-console
    #define MVR_BUFFER_USAGE 70 /* obviously wrong, don't try and record video
           // div_maybe(-100*MEM(236 + MVR_190_STRUCT) - \
           // 100*MEM(244 + MVR_190_STRUCT) - 100*MEM(384 + MVR_190_STRUCT) - \
           // 100*MEM(392 + MVR_190_STRUCT) + 100*MEM(240 + MVR_190_STRUCT) + \
           // 100*MEM(248 + MVR_190_STRUCT), \
           // - MEM(236 + MVR_190_STRUCT) - MEM(244 + MVR_190_STRUCT) + \
           // MEM(240 + MVR_190_STRUCT) +  MEM(248 + MVR_190_STRUCT)) */
    #define MVR_FRAME_NUMBER 0   // TODO M200: (*(int*)(220 + MVR_190_STRUCT)) once the struct is known
    //#define MVR_LAST_FRAME_SIZE (*(int*)(512 + MVR_752_STRUCT))
    #define MVR_BYTES_WRITTEN 0  // TODO M200: MEM((212 + MVR_190_STRUCT))


