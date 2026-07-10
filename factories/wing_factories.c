#include "osc_macro_factory_registration.h"

#include <string.h>
#include <stdio.h>

/** Usage: wing_talk_to_single_bus("A|B" <bus_id>) */
static bool response_factory__wing_talk_to_single_bus(tosc_message_batch *batch, tosc_message_argument args[], size_t arg_count)
{
    if (arg_count < 2)
        return false; // See usage

    if (args[0].argType != TOSC_ARGUMENT_STRING || args[1].argType != TOSC_ARGUMENT_INT32)
        return false;

    const char *address_prefix = NULL;
    if (strcmp(args[0].argValue.asString, "A") == 0)
    {
        address_prefix = "/cfg/talk/A";
    }
    else if (strcmp(args[0].argValue.asString, "B") == 0)
    {
        address_prefix = "/cfg/talk/B";
    }
    else
    {
        return false; // See usage
    }

    uint32_t bus_id = args[1].argValue.asInt;
    if (bus_id < 1 || bus_id > 16)
    {
        return false; // The Behringer WING only has 16 busses
    }

    char address_buffer[20];

    for (uint32_t i = 1; i <= 16; ++i)
    {
        // Busses
        snprintf(address_buffer, sizeof(address_buffer), "%s/B%d", address_prefix, i);
        tosc_messageBatchAdd(batch, address_buffer, "i", (i == bus_id) ? 1 : 0);

        // Matrices
        if (i <= 8)
        {
            snprintf(address_buffer, sizeof(address_buffer), "%s/MX%d", address_prefix, i);
            tosc_messageBatchAdd(batch, address_buffer, "i", 0);
        }

        // Mains
        if (i <= 4)
        {
            snprintf(address_buffer, sizeof(address_buffer), "%s/M%d", address_prefix, i);
            tosc_messageBatchAdd(batch, address_buffer, "i", 0);
        }
    }

    return true;
}

OSC_REGISTER_MACRO_RESPONSE_FACTORY("wing_talk_to_single_bus", response_factory__wing_talk_to_single_bus);