#ifndef OSC_MACRO_MAIN_LOOP_REGISTRATION_H
#define OSC_MACRO_MAIN_LOOP_REGISTRATION_H

#include "osc_macro_main_loop.h"

#if defined(__clang__) || defined(__GNUC__)
#define OSC_MACRO_CONSTRUCTOR __attribute__((constructor))
#else
#define OSC_MACRO_CONSTRUCTOR
#endif

#define OSC_MACRO_CONCAT_INNER(a, b) a##b
#define OSC_MACRO_CONCAT(a, b) OSC_MACRO_CONCAT_INNER(a, b)

#define OSC_REGISTER_MAIN_LOOP_HOOK(hook_name, hook_callback)                                               \
    static void OSC_MACRO_CONCAT(osc_macro_register_main_loop_hook_, __LINE__)(void) OSC_MACRO_CONSTRUCTOR; \
    static void OSC_MACRO_CONCAT(osc_macro_register_main_loop_hook_, __LINE__)(void)                        \
    {                                                                                                       \
        register_main_loop_hook_globally((hook_name), (hook_callback));                                     \
    }

#endif // OSC_MACRO_MAIN_LOOP_REGISTRATION_H