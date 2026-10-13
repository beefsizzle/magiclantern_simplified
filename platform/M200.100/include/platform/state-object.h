#ifndef __platform_state_object_h
#define __platform_state_object_h

// M200: "EVF_STATE" (LiveView::EvfCapState.c) is created by E0124216 at
// E00EA1E4 and stored at 0x8AD0, but that is Canon's newer state framework
// (0x3C byte object), NOT a CreateStateObject (E0577ACE) struct state_object.
// Hooking it froze the camera, so it is not defined here and
// CONFIG_STATE_OBJECT_HOOKS is off. (M50: 0x6cec.)
//#define EVF_STATE (*(struct state_object **)0x8AD0)

#endif // __platform_state_object_h
