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

// We can't yet rely on image capture.
#define CONFIG_IMAGE_CAPTURE_NOT_WORKING

#define CONFIG_CRASH_LOG
#define CONFIG_AUTOBACKUP_ROM

// No property writes on new ports until each one is checked
#undef CONFIG_PROP_REQUEST_CHANGE
