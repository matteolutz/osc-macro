#include "osc_macro.h"
#include "osc_macro_main_loop.h"
#include "osc_macro_transport.h"
#include "tinyosc.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

/**
 * Read the entire file into a heap allocated buffer and return a pointer to it.
 * The caller is responsible for freeing the buffer.
 */
char *read_entire_file(const char *filename)
{
  FILE *fp = fopen(filename, "r");

  if (fp == NULL)
    return NULL;

  if (fseek(fp, 0, SEEK_END) != 0)
    goto failed;

  long file_size = ftell(fp);
  if (file_size == -1)
    goto failed;

  char *buffer = malloc(file_size + 1);
  if (buffer == NULL)
    goto failed;

  if (fseek(fp, 0, SEEK_SET) != 0)
  {
    free(buffer);
    goto failed;
  }

  size_t actual_size = fread(buffer, sizeof(char), file_size, fp);
  buffer[actual_size++] = '\0'; // null terminate the buffer

  if (ferror(fp) != 0)
  {
    free(buffer);
    goto failed;
  }

  fclose(fp);
  return buffer;

failed:
  fclose(fp);
  return NULL;
}

/**
 * Build and send an OSC message from the given message builder to the given socket client context.
 * Returns true if the message was built and sent successfully, false otherwise.
 */
bool send_message_builder(tosc_message_builder *builder, osc_udp_transport *transport)
{
  tosc_messageBuilderPrint(builder);
  return osc_send_message_builder(builder, transport);
}

static bool parse_ipv4_address(const char *address_text, struct sockaddr_in *out_address)
{
  out_address->sin_family = AF_INET;
  out_address->sin_port = htons(2223);

  return inet_pton(AF_INET, address_text, &out_address->sin_addr) == 1;
}

/**
 * Handle the given macro by sending all of its responses to the given socket client context.
 */
void handle_trigger(osc_macro_ctx *macro_ctx, osc_macro *macro, osc_udp_transport *transport)
{
  printf("Sending %zu responses to: %s:%d\n", macro->responses.count, inet_ntoa(transport->destination_address->sin_addr), ntohs(transport->destination_address->sin_port));

  for (size_t i = 0; i < macro->responses.count; ++i)
  {
    printf("\nSending response %zu:\n", i + 1);

    switch (macro->responses.items[i].type)
    {
    case OSC_MACRO_RESPONSE_TYPE_OSC:
    {
      tosc_message_builder *builder_ref = &macro->responses.items[i].response.as_osc.message_builder;
      if (!send_message_builder(builder_ref, transport))
      {
        printf("Failed to send response %zu\n", i + 1);
      }

      break;
    }
    case OSC_MACRO_RESPONSE_TYPE_FACTORY:
    {
      tosc_message_batch message_batch = {0};

      osc_macro_factory_invocation *invocation = &macro->responses.items[i].response.as_factory;
      osc_response_factory *factory = find_macro_response_factory(macro_ctx, invocation->name);

      if (factory == NULL)
      {
        printf("Response factory '%s' not found\n", invocation->name);
        continue;
      }

      bool factory_result = factory->callback(&message_batch, invocation->args.items, invocation->args.count);
      if (!factory_result)
      {
        printf("Response factory '%s' failed to build response\n", invocation->name);
        continue;
      }

      for (size_t j = 0; j < message_batch.messages.count; ++j)
      {
        printf("sending response %zu of %zu from factory '%s'\n", j + 1, message_batch.messages.count, invocation->name);

        tosc_message_builder *builder_ref = &message_batch.messages.items[j];
        if (!send_message_builder(builder_ref, transport))
        {
          printf("Failed to send response %zu of %zu from factory '%s'\n", j + 1, message_batch.messages.count, invocation->name);
        }
      }

      tosc_messageBatchFree(&message_batch); // free the batch after building the message
      break;
    }
    default:
      printf("Unknown response type for response %zu\n", i + 1);
      continue;
    }
  }

  printf("\n");
}

int main(int argc, char *argv[])
{
  if (argc < 2 || argc > 3)
  {
    printf("Usage: %s <osc-macro-file> [remote-ip]\n", argv[0]);
    return 1;
  }

  osc_macro_ctx macro_ctx = {0};
  osc_main_loop_ctx main_loop_ctx = {0};
  osc_main_loop_context loop_context = {0};

  char *osc_macro_file = read_entire_file(argv[1]);
  if (osc_macro_file == NULL)
  {
    printf("could not read from osc macro file %s\n", argv[1]);
    return 1; // don't go to panic, this would mean calling free on a NULL pointer
  }

  char *parse_success = parse_osc_macro_collection(osc_macro_file, &macro_ctx);

  if (parse_success == NULL)
  {
    printf("osc macro collection parsing failed\n");
    goto panic;
  }

  load_registered_macro_response_factories(&macro_ctx);
  load_registered_main_loop_hooks(&main_loop_ctx);

  bool use_fixed_peer = false;
  struct sockaddr_in fixed_peer_address = {0};
  if (argc == 3)
  {
    if (!parse_ipv4_address(argv[2], &fixed_peer_address))
    {
      printf("invalid IPv4 address: %s\n", argv[2]);
      goto panic;
    }

    use_fixed_peer = true;
  }

  char recv_buffer[2048];
  char send_buffer[2048];

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
  {
    printf("failed to create socket\n");
    goto panic;
  }

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(2223);
  sin.sin_addr.s_addr = INADDR_ANY;

  int bind_result = bind(fd, (struct sockaddr *)&sin, sizeof(struct sockaddr));
  if (bind_result < 0)
  {
    printf("failed to bind socket\n");
    goto panic;
  }

  printf("osc-macro is listening on port 2223\n");

  loop_context.socket_fd = fd;
  loop_context.has_client_address = false;
  loop_context.recv_buffer = recv_buffer;
  loop_context.recv_buffer_size = sizeof(recv_buffer);
  loop_context.send_buffer = send_buffer;
  loop_context.send_buffer_size = sizeof(send_buffer);
  loop_context.received_message = NULL;
  loop_context.received_message_length = 0;
  loop_context.macro_ctx = &macro_ctx;

  if (use_fixed_peer)
  {
    printf("restricting traffic to %s\n", argv[2]);
    loop_context.client_address = fixed_peer_address;
    loop_context.has_client_address = true;
  }

  dispatch_main_loop_hooks(&main_loop_ctx, OSC_MAIN_LOOP_EVENT_START, &loop_context);

  while (1)
  {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    struct timeval timeout = {
        .tv_sec = 1,
        .tv_usec = 0,
    };

    int select_result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
    if (select_result < 0)
    {
      if (errno == EINTR)
      {
        dispatch_main_loop_hooks(&main_loop_ctx, OSC_MAIN_LOOP_EVENT_TICK, &loop_context);
        continue;
      }

      printf("select failed\n");
      goto panic;
    }

    if (select_result == 0)
    {
      dispatch_main_loop_hooks(&main_loop_ctx, OSC_MAIN_LOOP_EVENT_TICK, &loop_context);
      continue;
    }

    if (!FD_ISSET(fd, &read_fds))
      continue;

    struct sockaddr_in client_address;
    socklen_t client_address_size = sizeof(client_address);
    int len = recvfrom(fd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&client_address, &client_address_size);

    if (len < 0)
    {
      printf("recvfrom failed\n");
      continue;
    }

    if (use_fixed_peer && client_address.sin_addr.s_addr != fixed_peer_address.sin_addr.s_addr)
    {
      continue;
    }

    if (!use_fixed_peer)
    {
      loop_context.client_address = client_address;
      loop_context.client_address.sin_port = htons(2223);
      loop_context.has_client_address = true;
    }

    if (tosc_isBundle(recv_buffer))
      continue; // do nothing we don't support bundles

    tosc_message osc;
    tosc_parseMessage(&osc, recv_buffer, len);

    loop_context.received_message = &osc;
    loop_context.received_message_length = len;

    dispatch_main_loop_hooks(&main_loop_ctx, OSC_MAIN_LOOP_EVENT_MESSAGE, &loop_context);

    printf("Received OSC message:\n");
    tosc_printMessage(&osc);
    tosc_reset(&osc); // tosc_printMessage doesn't reset the marker so we have to do it manually

    osc_macro *triggered_macro = find_macro_by_trigger_message(&macro_ctx, &osc);
    if (triggered_macro == NULL)
    {
      loop_context.received_message = NULL;
      loop_context.received_message_length = 0;
      continue;
    }

    osc_udp_transport transport = {
        .socket_fd = fd,
        .destination_address = &loop_context.client_address,
        .send_buffer = send_buffer,
        .send_buffer_size = sizeof(send_buffer),
    };

    printf("------------------------- found macro for trigger ------------------------\n");
    handle_trigger(&macro_ctx, triggered_macro, &transport);
    printf("-------------------------- done handling trigger -------------------------\n");

    loop_context.received_message = NULL;
    loop_context.received_message_length = 0;
  }

  dispatch_main_loop_hooks(&main_loop_ctx, OSC_MAIN_LOOP_EVENT_SHUTDOWN, &loop_context);
  free_main_loop_ctx(&main_loop_ctx);
  free_osc_macro_ctx(&macro_ctx);
  free(osc_macro_file);
  return 0;

panic:
  free_main_loop_ctx(&main_loop_ctx);
  free_osc_macro_ctx(&macro_ctx);
  free(osc_macro_file);
  return 1;
}
