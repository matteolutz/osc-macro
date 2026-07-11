#include "osc_macro_main_loop_registration.h"
#include "osc_macro_transport.h"

#include <stdio.h>
#include <time.h>

#include <arpa/inet.h>

#define WING_KEEPALIVE_INTERVAL_SECONDS 8

static time_t wing_keepalive_next_send_at = 0;

static bool wing_keepalive_send(osc_main_loop_context *context)
{
    if (!context->has_client_address)
        return true;

    tosc_message_builder builder = {0};
    tosc_messageBuilderSetAddress(&builder, "/*s");

    osc_udp_transport transport = {
        .socket_fd = context->socket_fd,
        .destination_address = &context->client_address,
        .send_buffer = context->send_buffer,
        .send_buffer_size = context->send_buffer_size,
    };

    printf("sending wing keepalive to %s:%d\n", inet_ntoa(context->client_address.sin_addr), ntohs(context->client_address.sin_port));
    return osc_send_message_builder(&builder, &transport);
}

static bool wing_keepalive_on_event(osc_main_loop_event_type event_type, osc_main_loop_context *context)
{
    time_t now = time(NULL);

    switch (event_type)
    {
    case OSC_MAIN_LOOP_EVENT_START:
        wing_keepalive_next_send_at = 0;
        return true;
    case OSC_MAIN_LOOP_EVENT_TICK:
        if (!context->has_client_address)
            return true;

        if (wing_keepalive_next_send_at == 0)
        {
            wing_keepalive_next_send_at = now;
        }

        if (now < wing_keepalive_next_send_at)
            return true;

        if (!wing_keepalive_send(context))
            return false;

        wing_keepalive_next_send_at = now + WING_KEEPALIVE_INTERVAL_SECONDS;
        return true;
    case OSC_MAIN_LOOP_EVENT_MESSAGE:
        return true;
    case OSC_MAIN_LOOP_EVENT_SHUTDOWN:
        wing_keepalive_next_send_at = 0;
        return true;
    default:
        return true;
    }
}

OSC_REGISTER_MAIN_LOOP_HOOK("wing_keepalive", wing_keepalive_on_event);