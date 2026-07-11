#ifndef OSC_MACRO_TRANSPORT_H
#define OSC_MACRO_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>

#include <netinet/in.h>

#include "tinyosc.h"

typedef struct osc_udp_transport
{
    int socket_fd;
    struct sockaddr_in *destination_address;

    char *send_buffer;
    size_t send_buffer_size;
} osc_udp_transport;

bool osc_send_message_builder(tosc_message_builder *builder, osc_udp_transport *transport);

#endif // OSC_MACRO_TRANSPORT_H