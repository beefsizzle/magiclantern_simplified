#define FEATURE_VRAM_RGBA

// XCM v2 compositor, like M6 II (not pure Ximr like M50)
#define CONFIG_COMPOSITOR_XCM
#define CONFIG_COMPOSITOR_DEDICATED_LAYER
#define CONFIG_COMPOSITOR_XCM_V2

#define FEATURE_SHOW_SHUTTER_COUNT

// working but incomplete, some allocators don't report
// anything yet as they're faked / not yet found
#define FEATURE_SHOW_FREE_MEMORY

#define CONFIG_TSKMON
#define FEATURE_SHOW_TASKS
#define FEATURE_SHOW_CPU_USAGE
#define FEATURE_SHOW_GUI_EVENTS

#define FEATURE_GLOBAL_DRAW
#define FEATURE_CROPMARKS

// Zebras from the LV image (luma/RGB); focus peaking needs display filters
#define FEATURE_ZEBRA

// Remap ROM pages to RAM (for code patches). Canon copies its 0x4900-byte
// MMU tables from 0xE0000000 (copy_mmu_tables E005B090), TTBR0 = table +
// 0x4800 + cpu * 0x80 (E005B110 -> change_mmu_tables E058D0DC).
#define CONFIG_SGI_HANDLERS
#define CONFIG_MMU_REMAP

// Raw LV via Mem1Path / EDMAC ch3 (see consts.h)
#define CONFIG_RAW_LIVEVIEW
// mlv_lite needs the picture style getters
#define FEATURE_PICSTYLE

// We can't yet rely on image capture.
#define CONFIG_IMAGE_CAPTURE_NOT_WORKING

#define CONFIG_CRASH_LOG
#define CONFIG_AUTOBACKUP_ROM

// Property writes: on DIGIC 6/7/8 only the properties in
// property_whitelist.h prop_write_allow[] are written (property.c)
#define CONFIG_PROP_REQUEST_CHANGE
