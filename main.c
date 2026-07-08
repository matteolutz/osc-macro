#include "include/tinyosc.h"
#include "tinyosc.h"

#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

void print_message_buffer(const char *buffer, const int bufferLen) {
  printf("Buffer: ");
  for (int i = 0; i < bufferLen; ++i) {
    unsigned char val = buffer[i];
    if (isprint(val)) {
      putchar(val);
    } else {
      printf("[0x%1x]", val);
    }
  }

  printf("\n");
}

void test_message_builder() {
  tosc_message_builder builder = {0};
  tosc_messageBuilderInit(&builder, "/channel/1/123");

  tosc_messageBuilderAppendInt(&builder, 2);
  tosc_messageBuilderAppendString(&builder, "hello");
  tosc_messageBuilderAppendFloat(&builder, 3.14);
  tosc_messageBuilderAppendDouble(&builder, 9.81);

  char buffer[512];
  uint32_t bufferLen =
      tosc_messageBuilderBuild(&builder, buffer, sizeof(buffer));

  print_message_buffer(buffer, bufferLen);
}

void test_message() {
  char buffer[2048];
  uint32_t bufferLen = tosc_writeMessage(
      buffer, sizeof(buffer), "/channel/1/123", "isfd", 2, "hello", 3.14, 9.81);

  print_message_buffer(buffer, bufferLen);
}

int main() {
  test_message();
  test_message_builder();

  return 0;

  char recv_buffer[2048];

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
    return 1;

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(2223);
  sin.sin_addr.s_addr = INADDR_ANY;

  int bind_result = bind(fd, (struct sockaddr *)&sin, sizeof(struct sockaddr));
  if (bind_result < 0)
    return 1;

  printf("osc-macro is listening on port 2223\n");

  while (1) {
    struct sockaddr client_sa;
    socklen_t client_sa_len = sizeof(struct sockaddr_in);

    int len = 0;

    while ((len = recvfrom(fd, recv_buffer, sizeof(recv_buffer), 0, &client_sa,
                           &client_sa_len)) > 0) {
      if (tosc_isBundle(recv_buffer)) {
        // do nothing we don't support bundles
      } else {
        tosc_message osc;
        tosc_parseMessage(&osc, recv_buffer, len);
        tosc_printMessage(&osc);

        printf("Address: %s\n", tosc_getAddress(&osc));
      }
    }
  }

  return 0;
}
