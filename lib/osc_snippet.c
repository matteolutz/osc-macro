#include "osc_snippet.h"
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

bool parse_osc_snippet(char *snippet, osc_snippet *out_snippet) {
  if (snippet[0] != '/')
    return false; // snippet has to start with osc adress

  char *eoa = strchr(snippet, '(');
  if (eoa == NULL)
    return false; // snippet has to have "("

  char *cursor = eoa + 1;
  while (*cursor != ')') {
    if (*cursor == 0) {
      return false; // unexpected EOF
    }

    if (*cursor == ' ') {
      cursor++; // skip whitespace
      continue;
    }

    if (*cursor == '"') {
      // we have the start of a string
      cursor++; // skip over the "

      char *endOfString = strchr(cursor, '"');
      if (endOfString == NULL)
        return false; // string must be terminated

      *endOfString = 0; // null terminate the string
      tosc_messageBuilderAppendString(&out_snippet->message_builder, cursor);

      cursor = endOfString + 1;
    }

    if (isdigit(*cursor)) {
      // we have the start of a numeric value
      int i_val = *cursor++ - '0';
      while (*cursor != 0 && isdigit(*cursor)) {
        int digit = *cursor - '0';
        i_val = i_val * 10 + digit;
        cursor++;
      }

      if (*cursor != '.') {
        // it isn't a float or double so let's jsut append it
        tosc_messageBuilderAppendInt(&out_snippet->message_builder, i_val);
        continue;
      }

      double float_or_double = (double)i_val;
      double divisor = 1.0;
      cursor++; // skip the "."

      // parse fractional part
      while (*cursor != 0 && isdigit(*cursor)) {
        int digit = *cursor - '0';
        float_or_double = float_or_double * 10.0 + digit;
        divisor *= 10;
        cursor++;
      }

      float_or_double /= divisor;

      if (*cursor == 'f') {
        float f_val = (float)float_or_double;
        tosc_messageBuilderAppendFloat(&out_snippet->message_builder, f_val);
      } else if (*cursor == 'd') {
        tosc_messageBuilderAppendDouble(&out_snippet->message_builder,
                                        float_or_double);
      } else {
        return false; // decimal numbers have to end in 'f' or 'd'
      }

      cursor++; // skip over 'f' or 'd'
    }
  }

  *eoa = 0; // null terminate the end of address
  out_snippet->message_builder.address = snippet;

  return true;
}
