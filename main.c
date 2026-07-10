#include "osc_macro.h"
#include "tinyosc.h"

#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

typedef struct socket_response_ctx
{
  int socket_fd;
  struct sockaddr_in *response_address;

  char *send_buffer;
  size_t send_buffer_size;
} socket_response_ctx;

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
bool send_message_builder(tosc_message_builder *builder, socket_response_ctx *ctx)
{
  tosc_messageBuilderPrint(builder);

  uint32_t bytes_written = tosc_messageBuilderBuild(builder, ctx->send_buffer, ctx->send_buffer_size);

  if (bytes_written == 0)
  {
    printf("Failed to build response\n");
    return false;
  }

  printf("Response built: %d bytes\n", bytes_written);

  int send_result = sendto(ctx->socket_fd, ctx->send_buffer, bytes_written, 0, (struct sockaddr *)ctx->response_address, sizeof(struct sockaddr_in));
  if (send_result < 0)
  {
    printf("failed to send response\n");
    return false;
  }

  printf("Response sent: %d bytes\n", send_result);
  return true;
}

/**
 * Handle the given macro by sending all of its responses to the given socket client context.
 */
void handle_trigger(osc_macro_ctx *macro_ctx, osc_macro *macro, socket_response_ctx *ctx)
{
  printf("Sending %zu responses to: %s:%d\n", macro->responses.count, inet_ntoa(ctx->response_address->sin_addr), ntohs(ctx->response_address->sin_port));

  for (size_t i = 0; i < macro->responses.count; ++i)
  {
    printf("\nSending response %zu:\n", i + 1);

    switch (macro->responses.items[i].type)
    {
    case OSC_MACRO_RESPONSE_TYPE_OSC:
    {
      tosc_message_builder *builder_ref = &macro->responses.items[i].response.as_osc.message_builder;
      if (!send_message_builder(builder_ref, ctx))
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

      for (size_t i = 0; i < message_batch.messages.count; ++i)
      {
        printf("sending response %zu of %zu from factory '%s'\n", i + 1, message_batch.messages.count, invocation->name);

        tosc_message_builder *builder_ref = &message_batch.messages.items[i];
        if (!send_message_builder(builder_ref, ctx))
        {
          printf("Failed to send response %zu of %zu from factory '%s'\n", i + 1, message_batch.messages.count, invocation->name);
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
  if (argc < 2)
  {
    printf("Usage: %s <osc-macro-file>\n", argv[0]);
    return 1;
  }

  char *osc_macro_file = read_entire_file(argv[1]);
  if (osc_macro_file == NULL)
  {
    printf("could not read from osc macro file %s\n", argv[1]);
    return 1; // don't go to panic, this would mean calling free on a NULL pointer
  }

  osc_macro_ctx macro_ctx = {0};
  char *parse_success = parse_osc_macro_collection(osc_macro_file, &macro_ctx);

  if (parse_success == NULL)
  {
    printf("osc macro collection parsing failed\n");
    goto panic;
  }

  load_registered_macro_response_factories(&macro_ctx);

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

  struct sockaddr_in client_address;
  socklen_t client_address_size = sizeof(client_address);

  while (1)
  {
    int len = recvfrom(fd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&client_address, &client_address_size);

    if (tosc_isBundle(recv_buffer))
      continue; // do nothing we don't support bundles

    tosc_message osc;
    tosc_parseMessage(&osc, recv_buffer, len);

    printf("Received OSC message:\n");
    tosc_printMessage(&osc);
    tosc_reset(&osc); // tosc_printMessage doesn't reset the marker so we have to do it manually

    osc_macro *triggered_macro = find_macro_by_trigger_message(&macro_ctx, &osc);
    if (triggered_macro == NULL)
      continue;

    client_address.sin_port = htons(2223); // send to port 2223
    socket_response_ctx ctx = {
        .socket_fd = fd,
        .response_address = &client_address,
        .send_buffer = send_buffer,
        .send_buffer_size = sizeof(send_buffer),
    };

    printf("------------------------- found macro for trigger ------------------------\n");
    handle_trigger(&macro_ctx, triggered_macro, &ctx);
    printf("-------------------------- done handling trigger -------------------------\n");
  }

  free(osc_macro_file);
  return 0;

panic:
  free_osc_macro_ctx(&macro_ctx);
  free(osc_macro_file);
  return 1;
}
