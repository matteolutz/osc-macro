#include "osc_macro_main_loop.h"

#include <stdio.h>
#include <string.h>

#include "vector.h"

static VECTOR(osc_main_loop_hook, registered_main_loop_hooks) = {0};

void register_main_loop_hook(osc_main_loop_ctx *collection, const char *name, bool (*callback)(osc_main_loop_event_type event_type, osc_main_loop_hook_ctx *context))
{
    osc_main_loop_hook hook = {.name = name, .callback = callback};
    vec_push(&collection->hooks, hook);
}

void register_main_loop_hook_globally(const char *name, bool (*callback)(osc_main_loop_event_type event_type, osc_main_loop_hook_ctx *context))
{
    osc_main_loop_hook hook = {.name = name, .callback = callback};
    vec_push(&registered_main_loop_hooks, hook);
}

void load_registered_main_loop_hooks(osc_main_loop_ctx *collection)
{
    for (size_t i = 0; i < registered_main_loop_hooks.count; ++i)
    {
        osc_main_loop_hook *candidate = &registered_main_loop_hooks.items[i];

        bool already_registered = false;
        for (size_t j = 0; j < collection->hooks.count; ++j)
        {
            osc_main_loop_hook *existing = &collection->hooks.items[j];
            if (strcmp(existing->name, candidate->name) == 0)
            {
                already_registered = true;
                break;
            }
        }

        if (!already_registered)
        {
            vec_push(&collection->hooks, *candidate);
        }
    }
}

void dispatch_main_loop_hooks(osc_main_loop_ctx *collection, osc_main_loop_event_type event_type, osc_main_loop_hook_ctx *context)
{
    for (size_t i = 0; i < collection->hooks.count; ++i)
    {
        osc_main_loop_hook *hook = &collection->hooks.items[i];
        if (!hook->callback(event_type, context))
        {
            printf("main loop hook '%s' returned false\n", hook->name);
        }
    }
}

osc_udp_transport main_loop_ctx_get_transport(osc_main_loop_hook_ctx *context)
{
    return (osc_udp_transport){
        .socket_fd = context->socket_fd,
        .destination_address = &context->client_address,
        .send_buffer = context->send_buffer,
        .send_buffer_size = context->send_buffer_size};
}

void free_main_loop_ctx(osc_main_loop_ctx *collection)
{
    vec_free(collection->hooks);
}