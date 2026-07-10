#include "osc_macro_factory_registration.h"

#include <string.h>
#include <stdio.h>

static const char *wing_get_talk_prefix(const char *arg)
{
    if (strcmp(arg, "A") == 0)
    {
        return "/cfg/talk/A";
    }

    if (strcmp(arg, "B") == 0)
    {
        return "/cfg/talk/B";
    }

    return NULL;
}

/**
 * Usage: wing_talk_to("A"|"B" ...)
 *
 * Variadic arguments can be in the shape of: "B1", "MX1", "M3", ...
 */
static bool response_factory__wing_talk_to(tosc_message_batch *batch, tosc_message_argument args[], size_t arg_count)
{
    if (arg_count < 2)
        return false;

    // make sure all args are strings
    for (size_t i = 0; i < arg_count; ++i)
    {
        if (args[i].argType != TOSC_ARGUMENT_STRING)
            return false;
    }

    const char *address_prefix = wing_get_talk_prefix(args[0].argValue.asString);
    if (address_prefix == NULL)
        return false;

    bool busses[16] = {0};
    bool matrices[8] = {0};
    bool mains[4] = {0};

    for (size_t i = 1; i < arg_count; ++i)
    {
        const char *arg = args[i].argValue.asString;
        if (arg[0] == 'B')
        {
            int bus_id = atoi(arg + 1);
            if (bus_id < 1 || bus_id > 16)
                return false;
            busses[bus_id - 1] = true;
        }

        if (arg[0] == 'M')
        {
            if (arg[1] == 'X')
            {
                int matrix_id = atoi(arg + 2);
                if (matrix_id < 1 || matrix_id > 8)
                    return false;
                matrices[matrix_id - 1] = true;
            }
            else
            {
                int main_id = atoi(arg + 1);
                if (main_id < 1 || main_id > 4)
                    return false;
                mains[main_id - 1] = true;
            }
        }
    }

    printf("came here\n");

    char address_buffer[20];

    for (uint8_t i = 1; i <= 16; ++i)
    {
        // Busses
        snprintf(address_buffer, sizeof(address_buffer), "%s/B%d", address_prefix, i);
        tosc_messageBatchAdd(batch, address_buffer, "i", busses[i - 1]);

        // Matrices
        if (i <= 8)
        {
            snprintf(address_buffer, sizeof(address_buffer), "%s/MX%d", address_prefix, i);
            tosc_messageBatchAdd(batch, address_buffer, "i", matrices[i - 1]);
        }

        // Mains
        if (i <= 4)
        {
            snprintf(address_buffer, sizeof(address_buffer), "%s/M%d", address_prefix, i);
            tosc_messageBatchAdd(batch, address_buffer, "i", mains[i - 1]);
        }
    }

    return true;
}

OSC_REGISTER_MACRO_RESPONSE_FACTORY("wing_talk_to", response_factory__wing_talk_to);