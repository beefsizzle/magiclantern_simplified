/** \file
 * Entry points into the firmware image.
 *
 * These are the functions that we can call from our tasks
 * in the Canon 1.0.0 firmware for the EOS M200.
 *
 * Derived from platform/M50.110/stubs.S. Stubs marked "M200:" were
 * located in the M200 ROM and checked; everything else is commented
 * out as TODO with the M50 address kept for reference only.
 *
 * LLM involvement: M200 addresses were located with help from an LLM
 * (Claude) driving Ghidra/QEMU; see platform/M200.100/README.md.
 *
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

#include <stub.h>

/* All functions are Thumb. */


/** Startup **/
NSTUB(MAIN_FIRMWARE_ADDR, firmware_entry)
THUMB_FN(0xE0040104, cstart)                      /* M200: firmware_entry -> b.w at E0040034; push {r4-r6,lr}, calls bzero32 and create_init_task */
THUMB_FN(0xE058E390, bzero32)                     /* M200: called from cstart at BR_BZERO32 (E0040152) */
THUMB_FN(0xE02F63C4, create_init_task)            /* M200: called from cstart at BR_CREATE_ITASK (E00401B4) */
THUMB_FN(0xE058CECC, dcache_clean)                /* M200: loop with MCR p15 c7,c10,1; DSB */
THUMB_FN(0xE005B1A4, dcache_clean_multicore)      /* M200: loop writing PA to 0xC11007B0 (PL310 clean by PA) then 0xC1100730 (sync); called right after dcache_clean in MMU table copy */
THUMB_FN(0xE058CFA0, icache_invalidate)           /* M200: loop with MCR p15 c7,c5,1; c7,c1,6; c7,c1,0; ISB */
THUMB_FN(0xE0040236, init_task)                   /* M200: USER_MEM size checking, dmSetup, termDriverInit, stdlibSetup strings */

/** Tasks **/
DATA_PTR(0x1010, current_interrupt)           /* M200: IRQ dispatcher FUN_e019a6bc stores GIC IAR id here */
DATA_PTR(0x1028, current_task)                /* M200: per-CPU struct; QEMU debugmsg task names verified with this */
THUMB_FN(0xE05778B6, msleep)                      /* M200: SleepTask, from the eventproc registration table */
THUMB_FN(0xE05B3182, task_create)                 /* M200: UiOpe_Initialize calls it with "GuiMainTask", 0x2000 stack */
THUMB_FN(0xE05B339A, task_create_ex)              /* M200: KerTask.c; called by KerMisc CreateTask-variant wrapper; M6II task_create+0x218 */
NSTUB(0x4118, task_max)                    /* M200: create_init_task -> FUN_e014a3d0 copies init struct field 7 to 0x4104+0x14 */

/** File I/O **/
THUMB_FN(0xE05726D2, _FIO_CreateFile)             /* M200: Storage.c "Create"; Open + 0xB2 like M50/M6II */
THUMB_FN(0xE0572620, _FIO_OpenFile)               /* M200: Storage.c "Open"; Canon Basic OpenFileRD/WR/RDWR call it with mode 0/1/2 */
THUMB_FN(0xE0572E78, FIO_CloseFile)               /* M200: Storage.c "Close"; Canon Basic CloseFile */
THUMB_FN(0xE0572AAA, _FIO_ReadFile)               /* M200: Storage.c "Read" */
THUMB_FN(0xE0572C5C, _FIO_WriteFile)              /* M200: Storage.c "Write"; used by Canon Basic WriteFileString */
THUMB_FN(0xE0572B9E, FIO_SeekSkipFile)            /* M200: Storage.c "Search"; Canon Basic SeekFileEnd calls it with whence 2 */
THUMB_FN(0xE0572A12, _FIO_RemoveFile)             /* M200: Storage.c "Remove"; Canon Basic RemoveFile */
// TODO M200: not found yet. M50 had: THUMB_FN(0xE0547926, _FIO_RenameFile)
THUMB_FN(0xE0573EFE, _FIO_CreateDirectory)        /* M200: Storage.c "CreateDir" */
THUMB_FN(0xE0574358, _FIO_FindFirstEx)            /* M200: Storage.c "FirstEnt" */
THUMB_FN(0xE0574454, FIO_FindNextEx)              /* M200: Storage.c "NextEnt" */
THUMB_FN(0xE0574516, FIO_FindClose)               /* M200: Storage.c "CloseEnt" */
// TODO M200: not found yet. M50 had: THUMB_FN(0xE0547F9A,  FIO_Flush)                            // to be called after FIO_CloseFile?
THUMB_FN(0xE0573012, _FIO_GetFileSize64)          /* M200: Storage.c "AcqSize" */

/** Bitmap **/
// TODO M200: not found yet. M50 had: DATA_PTR(    0xFC20,  bmp_vram_info)                        /* first VRAM buffer, used right after DISP_SetUpdateOSDVram */

/** Debug messages **/
THUMB_FN(0xE058D494, DryosDebugMsg)               /* M200: most-called function (~42k BLs), verified via QEMU debugmsg plugin */

/** Dumper **/
// TODO M200: not found yet. M50 had: THUMB_FN(0xe00db9e6,  dump_file)                            // shouldn't be used, gone from newer models

/** Eventprocs (call by name) **/
THUMB_FN(0xE058855A, call)                        /* M200: ExecuteEventProcedure, from the eventproc registration table at E004E9DC */

/** GUI timers **/
THUMB_FN(0xE060C0AA, SetHPTimerNextTick)          /* M200: HPTimer.c, (last_expiry, offset, cbr, overrun_cbr, ctx); same worker */
THUMB_FN(0xE060C03A, SetHPTimerAfterNow)          /* M200: HPTimer.c, (delay_us, cbr, overrun_cbr, ctx); shares worker FUN_e060bf6e */
THUMB_FN(0xE056EBCC, SetTimerAfter)               /* M200: DlgErrCard.c calls it (8000, cbr, cbr, 0) then prints "SetTimerAfter error" */
THUMB_FN(0xE056EC82, CancelTimer)                 /* M200: used by CancelDateTimer; SetTimerAfter + 0xB6 like M50 */

/** Interrupts **/
THUMB_FN(0xE00E6340, send_software_interrupt)     /* M200: writes (cpu_mask << 16) | int to 0xC1001F00 (GICD_SGIR, movw/movt), DSB; DryOS calls it with int 0xA (wake cpu) / 0xB */

THUMB_FN(0xDF002C52, _request_RPC)                /* M200: RAM blob copied from ROM E13FBB34 (DF002C00); (func, arg) wrapper with IRQs off; same address as M6II */
                                   // to the other for RPC.
                                   // Passing a NULL func pointer, will suspend the other
                                   // CPU until it becomes non-NULL.
                                   // DryOS seems to use this to deliberately sleep a CPU
                                   // in some exceptional cases (e.g. asserts).
                                   //
                                   // Do not call this directly, because if you do,
                                   // and you forget to call clear_RPC_request(),
                                   // the target CPU will repeatedly call the func pointer
                                   // as fast as possible, forever.
                                   // Instead use request_RPC().

THUMB_FN(0xDF002C72, clear_RPC_request)           /* M200: _request_RPC + 0x20 like M50/850D/M6II; zeroes the request with IRQs off, returns 0 */
                                         // the function forever

// MMU related, required by CONFIG_MMU features
THUMB_FN(0xE058D0DC, change_mmu_tables)           /* M200: TTBR0=r0, TTBR1=r1, ISB, TLBIALL, BPIALL, DSB, ISB (what DryOS calls at runtime via E005B110); ignores ML's 3rd arg */
//THUMB_FN(0xe001eca4, copy_mmu_tables) // copies from one location to another,
                                      // fixing up addresses held in table start,
                                      // and doing cache maintenance

// slightly MMU related, used by CONFIG_MMU_REMAP
THUMB_FN(0xE031FE04, memcpy_dryos)                /* M200: Canon "memcpy" eventproc; self-contained (no calls), Thumb, in main FW ROM */


/** Memory allocation **/
THUMB_FN(0xE057B0A4, _AllocateMemory)             /* M200: eventproc AllocateMemory branches here; prints [MEM] NG AllocateMemory */
THUMB_FN(0xE057B25C, _FreeMemory)                 /* M200: eventproc FreeMemory branches here; _AllocateMemory + 0x1B8 like M50/M6II */
THUMB_FN(0xE057AF2E, GetMemoryInformation)        /* M200: called in _AllocateMemory before "[MEM] ERROR GetMemoryInformation" */
THUMB_FN(0xE057AF02, GetSizeOfMaxRegion)          /* M200: called in _AllocateMemory before "[MEM] ERROR GetSizeOfMaxRegion" */
THUMB_FN(0xE057B864, _alloc_dma_memory)           /* M200: alloc aligned 0x20 then GetUncacheableMemoryAddress; used by "@@@ ERROR NOT_ENOUGH_MEMORY" func */
THUMB_FN(0xE057B86A, _free_dma_memory)            /* M200: GetCacheableMemoryAddress then free; paired with the above */
THUMB_FN(0xE03B6FD0, _malloc)                     /* M200: used by CreateMemorySuite (0x18) and the "can't malloc" helper */
THUMB_FN(0xE03B7008, _free)                       /* M200: used by DeleteMemorySuite; _malloc + 0x38 like M6II */

/** Memory info **/
// TODO M200: not found yet. M50 had: THUMB_FN(0xE009DEB8,  memmap_info)                          /* Exception vector, DRYOS system memory etc */
// TODO M200: not found yet. M50 had: THUMB_FN(0xE0146FD6,  smemShowFix)                          /* Common Lower, Common Upper etc */
// TODO M200: not found yet. M50 had: THUMB_FN(0xE00F1C18,  malloc_info)                          /* Malloc Information */
// TODO M200: not found yet. M50 had: THUMB_FN(0xE00F1CB0,  sysmem_info)                          /* System Memory Information */

/** Misc **/
THUMB_FN(0xE057859C, uart_printf)                 /* M200: prints "[EEP_T] SetCS2..." which appears on the QEMU serial log */
THUMB_FN(0xE00602D8, vsnprintf)                   /* M200: called by LogFile_printf (FUN_e0060e0e, "*Data Lost*") as (buf, 0x60, fmt, ap) */

/** Startup **/
// additional_version: CONFIG_ADDITIONAL_VERSION disabled in features.h (as on M6II)
/* kitor: Please note that this points at '\n' char in char* with FW version */

/** Tasks **/
DATA_PTR(0x4154, task_dispatch_hook)          /* M200: task_max + 0x3C (all D7/D8 ports); called as (*0x4154)(1, old, new) in FUN_e01a5312 */
THUMB_FN(0xE00B7B58, task_trampoline)             /* M200: task setup FUN_e00b7b78 passes 0xE00B7B59 to the context init; loads current task, calls [task+0xC](task[+0x10]), then exits task */
/*
 * kitor: task_create calls internal function (e05f5fb4) which set up basic
 * task structure, and part of that is call to task_trampoline_caller (e00e96ac)
 * (it is followed by check of caller return value).
 * task_trampoline_caller will pass task_trampoline+1 as parameter to another
 * function.
 */
THUMB_FN(0xE01A5800, _get_task_info_by_id)        /* M200: called as (task_id, &info) right before " CORE : %d" in exception dump */
/*
 * kitor: via extask function. While on 5D3 it was called directly, now helper
 * is used. Find extask via string formats. Trace variables holding task id,
 * you will quickly find where it is verified (this is the mentioned helper).
 * In the helper code you will find call to our stub.
 */

/** Recursive locks **/
THUMB_FN(0xE058CC80, CreateRecursiveLock)         /* M200: KerMisc.c wrapper; called with "PCOMNET_WINE CBMan" like RP */
THUMB_FN(0xE056EF80, AcquireRecursiveLock)        /* M200: KerRLock.c, (lock, timeout); same layout as M50 */
THUMB_FN(0xE056F04A, ReleaseRecursiveLock)        /* M200: KerRLock.c, Acquire + 0xCA like M50 */

/** Semaphores **/
THUMB_FN(0xE058D0FC, create_named_semaphore)      /* M200: KerSem.c, take_semaphore_now - 0xCC like M50 */
THUMB_FN(0xE058D272, give_semaphore)              /* M200: KerSem.c, take_semaphore + 0x7A like M50 */
THUMB_FN(0xE058D1F8, take_semaphore)              /* M200: KerSem.c, (sem, timeout), returns 9/0xB on timeout */
THUMB_FN(0xE058D1C8, take_semaphore_now)          /* M200: KerSem.c, fixed timeout, take_semaphore - 0x30 like M50 */

/** GUI **/
DATA_PTR(0x10868, gui_task_list)               /* M200: WindowSystem::CtrlMan.c base struct (DAT_e070efb4); "CtrlMan: Use RecursiveLock" name at +0x14; winsys_sem - 0x164 like M6II */
THUMB_FN(0xE054452E, SetGUIRequestMode)           /* M200: logs "SetGUIRequestMode(0x%08x)" with its argument */
THUMB_FN(0xE022FB34, gui_main_task)               /* M200: entry passed to task_create("GuiMainTask") in UiOpe_Initialize */
THUMB_FN(0xE0558542, dialog_redraw)               /* M200: Dialog.c; locks winsys, WINSYS_DisplayWindow(dlg->window) -> refreshVrmsSurface; called right after CreateDialogBox in Start*App */
// TODO M200: not found yet. M50 had: THUMB_FN(0xe037719e,  gui_massive_event_loop)               // various "GUICMD_"* strings
THUMB_FN(0xE022FC66, gui_enqueue_message)         /* M200: "warning! QueueLength=%d" and "GUI::GUI.c" in one function */
// gui_init_end: empty implementation in function_overrides.c, as on M6II                         // via "EndGuiInit" string
DATA_PTR(0x4F10, gui_main_struct)             /* M200: UiOpe_Initialize stores GuiMainQueue at +0x1C; gui_enqueue_message uses the same struct */

/*
 * kitor Similar to R180 I was unable to find GUI_Control either. If you compare
 * 200D.101 surrounding functions, it just seems to DNE on R and M50.
 */

/** GUI timers **/
THUMB_FN(0xE05D5EBE, StartDateTimer)              /* M200: SetTimerAfter + "MenuMain.c StartDateTimer(%#x)" on error */
THUMB_FN(0xE05D5F72, CancelDateTimer)             /* M200: logs "CancelDateTimer" then CancelTimer */

/** MPU communication **/
/*
 * kitor: skipped for now. InitializeIntercom looks identical to R180?!
 * I thought M50 has no MPU, yet at least buffers are initialized.
 */

/** Queue **/
THUMB_FN(0xE0560440, msg_queue_create)            /* M200: CreateMessageQueue("GuiMainQueue", 0x400) in UiOpe_Initialize */
THUMB_FN(0xE0560504, msg_queue_receive)           /* M200: KerQueue.c, create + 0xC4 like M50 */
THUMB_FN(0xE0560662, msg_queue_post)              /* M200: KerQueue.c, used in gui_enqueue_message, create + 0x222 like M50 */
THUMB_FN(0xE0560698, msg_queue_count)             /* M200: KerQueue.c, returns count via out param, create + 0x258 like M50 */

/** Memory **/
THUMB_FN(0xE05B2C94, AllocateMemoryResource)      /* M200: RscWrap.c table slot 0 (m_pfAllocMemoryCBR) */
THUMB_FN(0xE05B2CC2, AllocateContinuousMemoryResource) /* M200: RscWrap.c slot 1; guiSyncAllocateContinuousMemoryResource error string */
THUMB_FN(0xE05B2CF0, FreeMemoryResource)          /* M200: RscWrap.c slot 2 (m_pfFreeMemoryCBR) */
// TODO M200: not found yet. M50 had: THUMB_FN(0xe0553c5c,  AddMemoryChunk)                       // Same as in 200D (matched by offsets), may be wrong as multiple variants exists.
                                                            // Other options: e054c3da e054cf5a
THUMB_FN(0xE0579E24, GetFirstChunkFromSuite)      /* M200: PackMem.c, suite signature, returns first chunk; used before GetMemoryAddressOfMemoryChunk */
THUMB_FN(0xE057A644, GetNextMemoryChunk)          /* M200: PackMem.c, (suite, chunk), checks both signatures, follows next link */
                                                            // Similar function is at e05547a0, but with just a few xrefs and different offset used
THUMB_FN(0xE0579BE6, GetMemoryAddressOfMemoryChunk) /* M200: PackMem.c, chunk signature; its failure prints "...GetMemoryAddressOfMemoryChunk" */
THUMB_FN(0xE0579FF0, DeleteMemorySuite)           /* M200: contains "DeleteMemorySuite 0x%x"; checks suite signature, frees via _free */
THUMB_FN(0xE057A610, CreateMemorySuite)           /* M200: PackMem.c; _malloc(0x18), empty list, suite signature */
// TODO M200: not found yet. M50 had: THUMB_FN(0xe055374c,  CreateMemoryChunk)                    // called from CreateMemorySuite
//kitor: disabled using CONFIG_MEMORY_SRM_NOT_WORKING
//THUMB_FN(0xe056a5c6,  SRM_AllocateMemoryResourceFor1stJob)  // "pMessage" and "pPriorMessage" with "RscMgr.c". Unsure - there seems to be multiple identical functions
                                                            // Matched like in R by structure params to 200D stub 
//THUMB_FN(0xe056da22,  SRM_FreeMemoryResourceFor1stJob)      // find refs to "pMessage", there are several similar looking functions using it,
                                                            // Matched to 200D stub, just buffer size differs (0x34 vs 0x30)

/** Uncategorized **/
DATA_PTR(0x134EC, sd_device)                   /* M200: SD_DeviceCreate stores the device pointer here (*DAT_e00f8dcc = pStgDev) */
// _LoadCalendarFromRTC: no synchronous version on M200; function_overrides.c wraps the async read below
THUMB_FN(0xE033AB04, _RTC_GetCalendarAsync)         /* M200: FA_GetCalendar (E006E43A) calls it as (0, 0x10, 0, cbr, buf) and waits on a semaphore; cbr E006E40E does memcpy(buf, data, 0x28) + give_semaphore */
THUMB_FN(0xE0779CAA, ErrForCamera_handler)        /* M200: handler passed to CreateDialogBox in StartErrForCameraApp (literal E0779F38); DlgErrForCamera.c events */

/** Properties **/
THUMB_FN(0xE057BFCC, _prop_cleanup)               /* M200: PropertyMgr.c; (token, prop), posts msg 5; prop_register_slave + 0x5AA like M6II */
THUMB_FN(0xE057BA22, prop_register_slave)         /* M200: PropertyMgr.c; (list, count, cbr, priv, token_cbr), posts msg 3; M6II spacing to _prop_cleanup */
THUMB_FN(0xE057BAF0, _prop_request_change)        /* M200: PropertyMgr.c; (prop, addr, len), used by SetGUIRequestMode; register_slave + 0xCE like M50 */

/**
 * RGBA / Compositor functions and structures
 */

/**
 * Things needed for CONFIG_COMPOSITOR_XCM / CONFIG_COMPOSITOR_DEDICATED_LAYER.
 * M200 uses the same XCM v2 compositor as M6 II (see M6II.111/stubs.S).
 */
THUMB_FN(0xE02D2D2C, refreshVrmsSurface)            /* M200: "VMIX_TransferRectangleToVram" error string; XCM_MakeContext + XimrExe loop */
DATA_PTR(   0x109E0, display_refresh_needed)        /* M200: checked and cleared in refreshVrmsSurface (0x109C8 + 0x18) */
DATA_PTR(   0x74780, RENDERER_LayersMetadata)       /* M200: layer array filled in RENDERER_InitializeScreen; _pXCM - 0x174 like M6II */
THUMB_FN(0xE00D151C, XCM_SetSourceSurface)          /* M200: "XCM_SetSourceSurface layerIndex = %d" / "Illegal handle" */
THUMB_FN(0xE00D17D0, XCM_GetSourceSurface)          /* M200: "XCM_GetSourceSurface layerIndex = %d" */
THUMB_FN(0xE00D1804, XCM_SetSourceArea)             /* M200: "XCM_SetSourceArea layerIndex = %d"; GetSourceSurface + 0x34 like M6II */
THUMB_FN(0xE02D30BA, XOC_SetLayerEnable)            /* M200: (p1, p2, layer, enable), "XOC_SetLayerEnable Illegal handle"; refreshVrmsSurface + 0x38E like M6II */
DATA_PTR(   0x748F4, _pXCM)                         /* M200: refreshVrmsSurface passes *0x748F4 to XCM_MakeContext / XCM_GetContext */
THUMB_FN(0xE00D1472, XCM_GetContext)                /* M200: (pXCM, i) -> pXCM + 0x10 + i*0x31C while i < count@+0xC (max 3), then chained XCM@+0xA08; "XCM_GetContext Illegal handle"; refreshVrmsSurface XimrExe's each until NULL */

/** General FEATURE_VRAM_RGBA stubs **/
THUMB_FN(0xE019F490, XimrExe)                     /* M200: copies the 0x31C-byte Ximr context and submits it; called per XCM context in refreshVrmsSurface, failure prints "XimrExe" */
DATA_PTR(0x109CC, winsys_sem)                  /* M200: semaphore taken in refreshVrmsSurface (VMIX_TransferRectangleToVram) around the Ximr update; display_refresh_needed - 0x14 like M6II */

/*
 * Pointer to first Canon RGBA buffer (GUI). On M50 this is set up by
 * VDrv_InitializeScreen function, 2nd param is saved there.
 */
DATA_PTR(0x10994, _rgb_vram_info)              /* M200: RENDERER_InitializeScreen stores RENDERER_LayersMetadata[0] here (0x10984 + 0x10); winsys_sem - 0x38 like M6II */

/** Wrong on purpose **/
DATA_PTR(       0x0,  LCD_Palette)                          // D8 has no indexed RGB buffers.


/** App handlers **/
/**
 * Those can usually be found by looking at function address passed to
 * WINSYS_CreateDialogBox_wrapper / WINSYS_CreateDialogBox, just after some
 * debug message */
THUMB_FN(0xE0528BE8, PlayMain_handler)            /* M200: handler passed to CreateDialogBox in PlayMain_StartPlayMain; push.w prologue */
THUMB_FN(0xE06815A4, ShootOlcApp_handler)         /* M200: handler passed to CreateDialogBox in StartShootOlcApp; DlgOlcMainCreative.c events */
THUMB_FN(0xE0425182, LiveViewApp_handler)         /* M200: handler passed to WINSYS_CreateDialogBox right after "StartLiveViewApp(%#x)" */
//THUMB_FN(0xe0338a10,  LiveViewShutterApp_handler)         // msg: "StartLiveViewShutterApp"
THUMB_FN(0xE05920BC, PlayMovieGuideApp_handler)   /* M200: handler passed to CreateDialogBox in PlayMovieGuide_Start; push.w prologue */

// See R180 stubs.s for details.
//DATA_PTR(  0xe708,  PlayMain_dialog)                      // in StartPlayMain()
//DATA_PTR( 0x13fa4,  ShootOlcApp_dialog)                   // in StartShootOlcApp()
DATA_PTR(0xE028, LiveViewApp_dialog)          /* M200: where StartLiveViewApp stores the CreateDialogBox result (struct 0xE024 + 4) */
//DATA_PTR( 0x1c434,  LiveViewShutterApp_dialog)            // in StartLiveViewShutterApp(), not sure if needed
//DATA_PTR( 0x14fa8,  PlayMovieGuideApp_dialog)             // in StartPlayMovieGuideApp()



/**
 * STUFF BELOW IS WRONG!
 **/

// fsuDecodePartitionTable: implemented in function_overrides.c (inlined on D8), as on M6II             // inlined like in EOS R. See R180 function_overrides.c for WIP alternative

/** EDMAC mem2mem (edmac-d8.c), names from theBilalFakhouri's M50_rawvideo
 * branch. M200 functions identified from Canon's own users: Mem1Path (raw LV,
 * LiveViewDrive::Mem1Path.c ~E07CB270), Engine::MemoryToMemoryEsub5.c
 * (init E066B2C8, start E066B382, term E066B352) and Warp::CommonLib.c
 * (E0667544 lock, E0667608/E0667610 wake/suspend, E066756E unlock).
 * All asserts in Engine::Edmac.c. */
THUMB_FN(0xE0570E16, edmac_set_address)             /* (ch, addr): writes ch+0xA0; Mem1Path (3, S+0x58) */
THUMB_FN(0xE0570E22, edmac_set_size)                /* (ch, edmac_info*): writes ch+0x48..+0x70; Mem1Path */
THUMB_FN(0xE0571284, edmac_set_transfer_mode)       /* (ch, 0..3): ch+0xC0 low bits; esub5 size step */
THUMB_FN(0xE0570C36, StartEDmac_maybe)              /* (ch): esub5 start, WR then RD */
THUMB_FN(0xE05703BA, ConnectReadEDmac_maybe)        /* (rd ch): esub5 start after StartEDmac(RD) */
THUMB_FN(0xE05704CA, edmac_reset_channel)           /* (ch): esub5 init, both channels */
THUMB_FN(0xE057093E, edmac_reset_boomer_vdkick)     /* (ch): esub5 init, WR before select */
THUMB_FN(0xE05709B6, edmac_select_boomer)           /* (ch, selector): esub5 init (0xC2060A + n), Mem1Path (3, S+0xC) */
THUMB_FN(0xE05705D0, RegisterEDmacCompleteCBR)      /* (ch, cbr, ctx): ch+0x3C = 1, hooks interrupt; esub5 init */
THUMB_FN(0xE05708F0, UnregisterEDmacCompleteCBR)    /* (ch): ch+0x3C = 0, default cbr; esub5 term */
THUMB_FN(0xE0570A8E, edmac_reset_packunpack_mode)   /* (ch): esub5 init, both channels */
THUMB_FN(0xE0570A4A, edmac_stop_boomer_maybe)       /* (ch): esub5 term, WR */
THUMB_FN(0xE0571D4C, SetPackUnpackForEdmacWriteChannel) /* (ch, config[5]): Mem1Path (3, S+0x6C) */
THUMB_FN(0xE0577E12, CreateResLockEntry)            /* (res ids, count): CommonLib E0667544, Mem1Path EngRscLock */
THUMB_FN(0xE0577FFA, LockEngineResources)           /* (entry): CommonLib E0667544 */
THUMB_FN(0xE0578094, UnLockEngineResources)         /* (entry): CommonLib E066756E, ShtVfx TermMem2MemPath */
THUMB_FN(0xE06E7DFA, PwrMng_WakeSubChips)           /* (list, 7-terminated): CommonLib E0667608, MvRawProPath */
THUMB_FN(0xE06E7E28, PwrMng_SuspendSubChips)        /* (list): CommonLib E0667610 */
