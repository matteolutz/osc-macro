#include "osc_snippet.h"
#include <ctype.h>
#include <string.h>

char *parse_osc_snippet(char *snippet, osc_snippet *out_snippet)
{
  if (*snippet != '/')
    return NULL; // snippet has to start with osc adress

  char *eoa = strchr(snippet, '(');
  if (eoa == NULL)
    return NULL; // snippet has to have "("

  char *cursor = eoa + 1;
  while (*cursor != ')')
  {
    if (*cursor == 0)
    {
      return NULL; // unexpected EOF
    }

    if (*cursor == ' ')
    {
      cursor++; // skip whitespace
      continue;
    }

    if (*cursor == '"')
    {
      // we have the start of a string
      cursor++; // skip over the "

      char *endOfString = strchr(cursor, '"');
      if (endOfString == NULL)
        return NULL; // string must be terminated

      *endOfString = 0; // null terminate the string
      tosc_messageBuilderAppendString(&out_snippet->message_builder, cursor);

      cursor = endOfString + 1;
    }

    if (isdigit(*cursor))
    {
      // we have the start of a numeric value
      int i_val = *cursor++ - '0';
      while (*cursor != 0 && isdigit(*cursor))
      {
        int digit = *cursor - '0';
        i_val = i_val * 10 + digit;
        cursor++;
      }

      if (*cursor != '.')
      {
        // it isn't a float or double so let's jsut append it
        tosc_messageBuilderAppendInt(&out_snippet->message_builder, i_val);
        continue;
      }

      double float_or_double = (double)i_val;
      double divisor = 1.0;
      cursor++; // skip the "."

      // parse fractional part
      while (*cursor != 0 && isdigit(*cursor))
      {
        int digit = *cursor - '0';
        float_or_double = float_or_double * 10.0 + digit;
        divisor *= 10;
        cursor++;
      }

      float_or_double /= divisor;

      if (*cursor == 'f')
      {
        float f_val = (float)float_or_double;
        tosc_messageBuilderAppendFloat(&out_snippet->message_builder, f_val);
      }
      else if (*cursor == 'd')
      {
        tosc_messageBuilderAppendDouble(&out_snippet->message_builder, float_or_double);
      }
      else
      {
        return NULL; // decimal numbers have to end in 'f' or 'd'
      }

      cursor++; // skip over 'f' or 'd'
    }
  }

  *eoa = 0; // null terminate the end of address
  out_snippet->message_builder.address = snippet;

  return cursor + 1;
}

char *parse_osc_macro(char *macro, osc_macro *out_macro)
{
  osc_snippet trigger = {0};
  if (!(macro = parse_osc_snippet(macro, &trigger)))
  {
    return NULL; // failed to parse trigger
  }
  out_macro->trigger = trigger;

  // find the next newline
  if (!(macro = strchr(macro, '\n')))
  {
    return NULL; // no newline found after trigger, so we have no responses
  }

  macro++; // skip over the newline

  while (1)
  {
    if (*macro == ' ')
    {
      macro++; // skip whitespace
      continue;
    }

    if (*macro != '>')
    {
      break;
    }

    macro++; // skip over the ">"
    while (macro != 0 && *macro == ' ')
    {
      macro++; // skip whitespace
    }

    osc_snippet response = {0};
    if (!(macro = parse_osc_snippet(macro, &response)))
    {
      return NULL; // failed to parse response
    }

    out_macro->responses[out_macro->responses_count].type = OSC_MACRO_RESPONSE_TYPE_OSC;
    out_macro->responses[out_macro->responses_count].response.as_osc = response;
    out_macro->responses_count++;

    // look for the next newline
    char *next_newline = strchr(macro, '\n');
    if (!next_newline)
    {
      break; // no more responses
    }

    macro = next_newline + 1; // skip over the newline
  }

  return macro;
}

char *parse_osc_macro_collection(char *macro_collection, osc_macro_collection *out_macro_collection)
{
  char *cursor = macro_collection;

  while (*cursor != 0 && out_macro_collection->macro_count < OSC_MAX_MACROS)
  {
    // skip any whitespaces or newlines
    if (*cursor == ' ' || *cursor == '\n')
    {
      cursor++;
      continue;
    }

    osc_macro macro = {0};
    if (!(cursor = parse_osc_macro(cursor, &macro)))
    {
      return NULL;
    }

    out_macro_collection->macros[out_macro_collection->macro_count++] = macro;
  }

  return cursor;
}

osc_macro *find_macro_by_trigger_message(osc_macro_collection *collection, tosc_message *trigger_message)
{
  for (int i = 0; i < collection->macro_count; ++i)
  {
    osc_macro *macro = &collection->macros[i];
    if (tosc_messageBuilderEqualsMessage(&macro->trigger.message_builder, trigger_message))
    {
      return macro;
    }
  }
  return NULL;
}