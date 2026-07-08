#include "tinyosc.h"

#include <netinet/in.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

int main() {
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
