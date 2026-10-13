# EOS M200
Platform support for EOS M200 1.0.0 (ICU 4.1.1, model 0x812).

The M200 never got a firmware update. Users report that different internal
"1.0.0" builds may exist; this port was made on the one with the firmware
signature `0x5E5F218C` (from 0xE0040000).

## Status
Initial support: Magic Lantern boots, the ML menu works from the shooting
screens (LiveView), Canon photo and movie functions keep working.

## Notes
- No INFO, Q or Trash button. The ML menu opens with a long press of INFO
  (which is DOWN); long press SET is Q inside the ML menu. In playback a
  long INFO press stays Canon's (display brightness boost), so the ML menu
  is opened from LiveView.
- In LiveView the ML menu sits on Canon's LV Q-dialog mode (0xC048), as on
  M6 II. Canon closes that dialog ~10 s after the last user activity event,
  so ML sends one (0xA5) every 5 s while the menu is open.
- The ROM dumps fine with Canon Basic (playback mode + SET).
