#include "osc_snippet.h"
#include "tinyosc.h"

#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
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

int main()
{
  char *osc_macro_file = read_entire_file("./maros.txt");

  osc_macro_collection macro_collection = {0};
  char *parse_success = parse_osc_macro_collection(osc_macro_file, &macro_collection);

  if (parse_success == NULL)
  {
    printf("osc macro collection parsing failed\n");
    goto panic;
  }

  char recv_buffer[2048];
  char send_buffer[2048];

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
  {
    printf("failed to create socket\n");
    goto panic;
  }

  // int resuse_result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
  // if (resuse_result < 0)
  // {
  //   printf("failed to set SO_REUSEADDR\n");
  //   goto panic;
  // }

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

  while (1)
  {
    struct sockaddr client_sa;
    socklen_t client_sa_len = sizeof(struct sockaddr_in);

    int len = 0;

    while ((len = recvfrom(fd, recv_buffer, sizeof(recv_buffer), 0, &client_sa,
                           &client_sa_len)) > 0)
    {
      if (tosc_isBundle(recv_buffer))
        continue; // do nothing we don't support bundles

      tosc_message osc;
      tosc_parseMessage(&osc, recv_buffer, len);

      printf("Received OSC message:\n");
      tosc_printMessage(&osc);
      tosc_reset(&osc); // tosc_printMessage doesn't reset the marker so we have to do it manually

      osc_macro *triggered_macro = find_macro_by_trigger_message(&macro_collection, &osc);
      if (triggered_macro == NULL)
        continue;

      printf("found macro for trigger!\n");

      for (int i = 0; i < triggered_macro->responses_count; ++i)
      {
        printf("Sending response %d:\n", i + 1);
        tosc_messageBuilderPrint(&triggered_macro->responses[i].message_builder);

        uint32_t bytes_written = tosc_messageBuilderBuild(&triggered_macro->responses[i].message_builder, send_buffer, sizeof(send_buffer));
        if (bytes_written == 0)
        {
          printf("failed to build response!\n");
          continue;
        }

        int send_result = sendto(fd, send_buffer, bytes_written, 0, &client_sa,
                                 client_sa_len);
        printf("response %d sent: %d bytes\n", i + 1, send_result);
      }
    }
  }

  free(osc_macro_file);
  return 0;

panic:
  free(osc_macro_file);
  return 1;
}
