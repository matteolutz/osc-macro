#ifndef OSC_MACRO_MAIN_LOOP_H
#define OSC_MACRO_MAIN_LOOP_H

#include <stdbool.h>
#include <stddef.h>

#include <netinet/in.h>

#include "osc_macro.h"
#include "osc_macro_transport.h"

typedef enum osc_main_loop_event_type
{
    OSC_MAIN_LOOP_EVENT_START,
    OSC_MAIN_LOOP_EVENT_TICK,
    OSC_MAIN_LOOP_EVENT_MESSAGE,
    OSC_MAIN_LOOP_EVENT_SHUTDOWN
} osc_main_loop_event_type;

typedef struct osc_main_loop_hook_ctx
{
    int socket_fd;

    struct sockaddr_in client_address;
    bool has_client_address;

    char *recv_buffer;
    size_t recv_buffer_size;

    char *send_buffer;
    size_t send_buffer_size;

    tosc_message *received_message;
    int received_message_length;

    osc_macro_ctx *macro_ctx;
} osc_main_loop_hook_ctx;

typedef struct osc_main_loop_hook
{
    const char *name;
    bool (*callback)(osc_main_loop_event_type event_type, osc_main_loop_hook_ctx *context);
} osc_main_loop_hook;

typedef struct osc_main_loop_ctx
{
    VECTOR(osc_main_loop_hook, hooks);
} osc_main_loop_ctx;

void register_main_loop_hook(osc_main_loop_ctx *collection, const char *name, bool (*callback)(osc_main_loop_event_type event_type, osc_main_loop_hook_ctx *context));
void register_main_loop_hook_globally(const char *name, bool (*callback)(osc_main_loop_event_type event_type, osc_main_loop_hook_ctx *context));
void load_registered_main_loop_hooks(osc_main_loop_ctx *collection);
void dispatch_main_loop_hooks(osc_main_loop_ctx *collection, osc_main_loop_event_type event_type, osc_main_loop_hook_ctx *context);

osc_udp_transport main_loop_ctx_get_transport(osc_main_loop_hook_ctx *context);

void free_main_loop_ctx(osc_main_loop_ctx *collection);

#endif // OSC_MACRO_MAIN_LOOP_H