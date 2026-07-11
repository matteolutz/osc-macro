#include "osc_macro_transport.h"

#include <stdio.h>

#include <sys/socket.h>

bool osc_send_message_builder(tosc_message_builder *builder, osc_udp_transport *transport)
{
    uint32_t bytes_written = tosc_messageBuilderBuild(builder, transport->send_buffer, transport->send_buffer_size);

    if (bytes_written == 0)
    {
        printf("Failed to build response\n");
        return false;
    }

    int send_result = sendto(transport->socket_fd, transport->send_buffer, bytes_written, 0, (struct sockaddr *)transport->destination_address, sizeof(struct sockaddr_in));
    if (send_result < 0)
    {
        printf("failed to send response\n");
        return false;
    }

    printf("Response sent: %d bytes\n", send_result);
    return true;
}