#include "osc_macro_factory_registration.h"

/** Usage: echo("/<osc-address>" ...args) */
static bool response_factory__echo(tosc_message_batch *batch, tosc_message_argument args[], size_t arg_count)
{
    if (arg_count < 1)
        return false;

    tosc_message_builder builder = {0};
    tosc_message_argument address_arg = args[0];
    if (address_arg.argType != TOSC_ARGUMENT_STRING)
        return false;

    tosc_messageBuilderSetAddress(&builder, address_arg.argValue.asString);
    for (size_t i = 1; i < arg_count; ++i)
    {
        tosc_messageBuilderAppend(&builder, args[i]);
    }

    tosc_messageBatchAddBuilder(batch, builder);

    return true;
}

OSC_REGISTER_MACRO_RESPONSE_FACTORY("echo", response_factory__echo);