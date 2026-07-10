#ifndef OSC_MACRO_FACTORY_REGISTRATION_H
#define OSC_MACRO_FACTORY_REGISTRATION_H

#include "osc_snippet.h"

#if defined(__clang__) || defined(__GNUC__)
#define OSC_MACRO_CONSTRUCTOR __attribute__((constructor))
#else
#define OSC_MACRO_CONSTRUCTOR
#endif

#define OSC_MACRO_CONCAT_INNER(a, b) a##b
#define OSC_MACRO_CONCAT(a, b) OSC_MACRO_CONCAT_INNER(a, b)

#define OSC_REGISTER_MACRO_RESPONSE_FACTORY(factory_name, factory_callback)                                   \
    static void OSC_MACRO_CONCAT(osc_macro_register_response_factory_, __LINE__)(void) OSC_MACRO_CONSTRUCTOR; \
    static void OSC_MACRO_CONCAT(osc_macro_register_response_factory_, __LINE__)(void)                        \
    {                                                                                                         \
        register_macro_response_factory_globally((factory_name), (factory_callback));                         \
    }

#endif // OSC_MACRO_FACTORY_REGISTRATION_H