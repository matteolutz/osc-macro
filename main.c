#include "osc_snippet.h"
#include "tinyosc.h"

#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

void print_message_buffer(const char *buffer, const int bufferLen)
{
  printf("Buffer: ");
  for (int i = 0; i < bufferLen; ++i)
  {
    unsigned char val = buffer[i];
    if (isprint(val))
    {
      putchar(val);
    }
    else
    {
      printf("[0x%1x]", val);
    }
  }

  printf("\n");
}

void test_message_builder()
{
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

void test_message_builder_eq()
{
  tosc_message_builder builder_a = {0};
  tosc_messageBuilderInit(&builder_a, "/channel/1/123");

  tosc_messageBuilderAppendInt(&builder_a, 2);
  tosc_messageBuilderAppendString(&builder_a, "hello");
  tosc_messageBuilderAppendFloat(&builder_a, 3.14);
  tosc_messageBuilderAppendDouble(&builder_a, 9.81);

  tosc_message_builder builder_b = {0};
  tosc_messageBuilderInit(&builder_b, "/channel/1/123");

  tosc_messageBuilderAppendInt(&builder_b, 2);
  tosc_messageBuilderAppendString(&builder_b, "hello");
  tosc_messageBuilderAppendFloat(&builder_b, 3.14);
  tosc_messageBuilderAppendDouble(&builder_b, 9.81);

  printf("builder_a == builder_b: %d\n", tosc_messageBuilderEquals(&builder_a, &builder_b));
}

void test_message()
{
  char buffer[2048];
  uint32_t bufferLen = tosc_writeMessage(
      buffer, sizeof(buffer), "/channel/1/123", "isfd", 2, "hello", 3.14, 9.81);

  print_message_buffer(buffer, bufferLen);
}

void log_message_builder_args(tosc_message_builder *builder)
{
  for (int i = 0; i < builder->argCount; i++)
  {
    tosc_message_argument *arg = &builder->args[i];
    printf("\targ %d: ", i + 1);
    switch (arg->argType)
    {
    case TOSC_ARGUMENT_STRING:
      printf("%s", arg->argValue.asString);
      break;
    case TOSC_ARGUMENT_INT32:
      printf("%d", arg->argValue.asInt);
      break;
    case TOSC_ARGUMENT_FLOAT:
      printf("%ff", arg->argValue.asFloat);
      break;
    case TOSC_ARGUMENT_DOUBLE:
      printf("%lff", arg->argValue.asDouble);
      break;
    default:
      break;
    }
    printf("\n");
  }
}

int main()
{
  test_message();
  test_message_builder();

  test_message_builder_eq();

  char snippet[128] = "/channel/1/test(\"hello\")\n"
                      "> /channel/1/another()\n"
                      "> /channel/1/third(1 2 3.14f \"test\")";

  osc_macro macro = {0};
  char *parse_success = parse_osc_macro(snippet, &macro);

  if (parse_success != NULL)
  {
    printf("trigger address: %s\n", macro.trigger.message_builder.address);
    log_message_builder_args(&macro.trigger.message_builder);

    printf("there are %d responses\n", macro.responses_count);
    for (int i = 0; i < macro.responses_count; i++)
    {
      osc_snippet *response = &macro.responses[i];
      printf("response %d address: %s\n", i + 1, response->message_builder.address);
      log_message_builder_args(&response->message_builder);
    }
  }
  else
  {
    printf("parsing failed\n");
  }

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

  while (1)
  {
    struct sockaddr client_sa;
    socklen_t client_sa_len = sizeof(struct sockaddr_in);

    int len = 0;

    while ((len = recvfrom(fd, recv_buffer, sizeof(recv_buffer), 0, &client_sa,
                           &client_sa_len)) > 0)
    {
      if (tosc_isBundle(recv_buffer))
      {
        // do nothing we don't support bundles
      }
      else
      {
        tosc_message osc;
        tosc_parseMessage(&osc, recv_buffer, len);
        tosc_printMessage(&osc);

        printf("Address: %s\n", tosc_getAddress(&osc));
      }
    }
  }

  return 0;
}
