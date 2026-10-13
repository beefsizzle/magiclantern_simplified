# EOS M200
Platform support for EOS M200 1.0.0 (ICU 4.1.1, model 0x812).

The M200 never got a firmware update. Users report that different internal
"1.0.0" builds may exist; this port was made on the one with the firmware
signature `0x5E5F218C` (from 0xE0040000).

## Status
Working: ML menu (from the shooting screens), global draw, cropmarks,
zebras, info bars with recording time / bitrate, MMU remap, and raw
video with mlv_lite (14 / 12 / 10-bit), including 1:1 crops from the 4K
mode (raw 4368x2202) and from magnify (raw 4288x1202 at the movie frame
rate, centred on what Canon shows) with a framing preview. Not working /
not done: image capture from ML, focus peaking, screenshots, other
property writes.

## Notes
- No INFO, Q or Trash button. The ML menu opens with a long press of INFO
  (which is DOWN); long press SET is Q inside the ML menu. In playback a
  long INFO press stays Canon's (display brightness boost), so the ML menu
  is opened from LiveView.
- In LiveView the ML menu sits on Canon's LV Q-dialog mode (0xC048), as on
  M6 II. Canon closes that dialog ~10 s after the last user activity event,
  so ML sends one (0xA5) every 5 s while the menu is open.
- Long press RIGHT in LiveView: zoom 1 -> 5 (-> 10) -> 1. The only
  property ML writes (PROP_LV_DISPSIZE).
- Raw video: Canon writes all frames into one buffer; ML copies each frame
  right after Canon's raw EDMAC completes (lv-vsync.c). 12-bit uses the
  Mem1Path width (no code patch), 10-bit an edmac_set_size hook.
- The ROM dumps fine with Canon Basic (playback mode + SET).
