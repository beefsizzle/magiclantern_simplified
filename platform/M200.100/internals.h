/**
 * Camera internals for EOS M200 1.0.0
 */

/** This camera has a DIGIC VIII chip */
#define CONFIG_DIGIC_VIII

/** Push every XCM context to XimrExe, like Canon's refreshVrmsSurface, not
 * only the first one: after switching scene / art mode <-> P/Tv/Av/M the LV
 * image stayed black otherwise. */
#define CONFIG_XCM_EXEC_ALL_CONTEXTS

/** Attach ML's XCM layer only while LiveView runs (see compositor.c) */
#define CONFIG_COMPOSITOR_LAYER_LV_ONLY

/** Only write changed pixels to the RGBA layer (bmp.c) */
#define CONFIG_RGBA_REFRESH_DIFF

/** ML menu redraw and RGBA refresh exclude each other (bmp.c) */
#define CONFIG_RGBA_REFRESH_LOCKED

/** H.264 is encoded in hardware: redraw zebras more often while recording */
#define ZEBRA_RECORDING_SLOWDOWN 2

/** LiveView's EVF_STATE uses Canon's newer state framework, not
 * CreateStateObject, so the state object hooks don't apply. */
//#define CONFIG_STATE_OBJECT_HOOKS

// has inter-core RPC (so far this has always been dependent on SGI, 0xc)
#define CONFIG_RPC

// Cam has MMU (by itself, does nothing, see CONFIG_MMU_REMAP)
#define CONFIG_MMU

/** Digic 8 does not have bitmap font in ROM, try to load it from card **/
#define CONFIG_NO_BFNT

/** disable SRM for now */
#define CONFIG_MEMORY_SRM_NOT_WORKING

/** Q and SET are one button: long press SET in the ML menu = Q (submenus) */
#define CONFIG_LONG_PRESS_SET_MENU

/** The ML menu sits on Canon's LV Q-dialog mode, which Canon closes after
 * ~10 s without user activity; menu.c sends BGMT_USER_ACTIVITY (gui.h). */
#define CONFIG_MENU_TIMEOUT_FIX

/* has LV */
#define CONFIG_LIVEVIEW

#define CONFIG_MALLOC_STRUCT_V2

#define CONFIG_TASK_STRUCT_V2_SMP
#define CONFIG_TASK_ATTR_STRUCT_V5
