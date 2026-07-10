#include "osc_snippet.h"
#include <ctype.h>
#include <string.h>

#include "vector.h"

static VECTOR(osc_response_factory, registered_response_factories) = {0};

static char *parse_message_builder_args(char *cursor, tosc_message_builder *builder)
{
  while (*cursor != ')')
  {
    if (*cursor == 0)
      return NULL; // unexpected EOF

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
      tosc_messageBuilderAppendString(builder, cursor);

      cursor = endOfString + 1;
    }

    if (isdigit(*cursor))
    {
      // we have the start of a numeric value
      int32_t i_val = *cursor++ - '0';
      while (*cursor != 0 && isdigit(*cursor))
      {
        int32_t digit = *cursor - '0';
        i_val = i_val * 10 + digit;
        cursor++;
      }

      if (*cursor != '.')
      {
        // it isn't a float or double so let's jsut append it
        tosc_messageBuilderAppendInt(builder, i_val);
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
        tosc_messageBuilderAppendFloat(builder, f_val);
      }
      else if (*cursor == 'd')
      {
        tosc_messageBuilderAppendDouble(builder, float_or_double);
      }
      else
      {
        return NULL; // decimal numbers have to end in 'f' or 'd'
      }

      cursor++; // skip over 'f' or 'd'
    }
  }

  return cursor;
}

char *parse_osc_snippet(char *snippet, osc_snippet *out_snippet)
{
  if (*snippet != '/')
    return NULL; // snippet has to start with osc adress

  char *eoa = strchr(snippet, '(');
  if (eoa == NULL)
    return NULL; // snippet has to have "("

  char *cursor = eoa + 1;
  cursor = parse_message_builder_args(cursor, &out_snippet->message_builder);
  if (cursor == NULL)
    return NULL;

  *eoa = 0; // null terminate the end of address
  tosc_messageBuilderSetAddress(&out_snippet->message_builder, snippet);

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

    if (*macro == '/')
    {
      osc_snippet osc_response = {0};
      if (!(macro = parse_osc_snippet(macro, &osc_response)))
      {
        return NULL; // failed to parse response
      }

      osc_macro_response response = {.type = OSC_MACRO_RESPONSE_TYPE_OSC, .response.as_osc = osc_response};
      vec_push(&out_macro->responses, response);
    }
    else
    {
      // parse factory invocation
      char *start_of_name = macro;
      char *end_of_name = strchr(macro, '(');
      if (end_of_name == NULL)
        return NULL; // factory invocation has to have "("

      *end_of_name = '\0';

      macro = end_of_name + 1;
      tosc_message_builder builder = {0};
      macro = parse_message_builder_args(macro, &builder);
      if (macro == NULL)
        return NULL;

      macro++; // skip the ')'

      osc_macro_response response = {
          .type = OSC_MACRO_RESPONSE_TYPE_FACTORY,
          .response.as_factory.name = start_of_name,
          .response.as_factory.args = vec_move(builder.args)};
      vec_push(&out_macro->responses, response);
    }

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

  while (*cursor != 0)
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

    vec_push(&out_macro_collection->macros, macro);
  }

  return cursor;
}

osc_macro *find_macro_by_trigger_message(osc_macro_collection *collection, tosc_message *trigger_message)
{
  for (size_t i = 0; i < collection->macros.count; ++i)
  {
    osc_macro *macro = &collection->macros.items[i];
    if (tosc_messageBuilderEqualsMessage(&macro->trigger.message_builder, trigger_message))
    {
      return macro;
    }
  }
  return NULL;
}

void register_macro_response_factory(osc_macro_collection *collection, const char *name, bool (*callback)(tosc_message_batch *out_message_batch, tosc_message_argument args[], size_t arg_count))
{
  osc_response_factory factory = {.name = name, .callback = callback};
  vec_push(&collection->response_factories, factory);
}

void register_macro_response_factory_globally(const char *name, bool (*callback)(tosc_message_batch *out_message_batch, tosc_message_argument args[], size_t arg_count))
{
  osc_response_factory factory = {.name = name, .callback = callback};
  vec_push(&registered_response_factories, factory);
}

void load_registered_macro_response_factories(osc_macro_collection *collection)
{
  for (size_t i = 0; i < registered_response_factories.count; ++i)
  {
    osc_response_factory *candidate = &registered_response_factories.items[i];

    bool already_registered = false;
    for (size_t j = 0; j < collection->response_factories.count; ++j)
    {
      osc_response_factory *existing = &collection->response_factories.items[j];
      if (strcmp(existing->name, candidate->name) == 0)
      {
        already_registered = true;
        break;
      }
    }

    if (!already_registered)
    {
      vec_push(&collection->response_factories, *candidate);
    }
  }
}

osc_response_factory *find_macro_response_factory(osc_macro_collection *collection, const char *name)
{
  for (size_t i = 0; i < collection->response_factories.count; ++i)
  {
    osc_response_factory *factory = &collection->response_factories.items[i];
    if (strcmp(factory->name, name) == 0)
    {
      return factory;
    }
  }

  return NULL;
}

void free_osc_macro_response(osc_macro_response *response)
{
  switch (response->type)
  {
  case OSC_MACRO_RESPONSE_TYPE_OSC:
    tosc_messageBuilderFree(&response->response.as_osc.message_builder);
    break;
  case OSC_MACRO_RESPONSE_TYPE_FACTORY:
    vec_free(response->response.as_factory.args);
    break;
  default:
    break;
  }
}

void free_osc_macro(osc_macro *macro)
{
  for (size_t i = 0; i < macro->responses.count; ++i)
  {
    free_osc_macro_response(&macro->responses.items[i]);
  }

  vec_free(macro->responses);
}

void free_osc_macro_collection(osc_macro_collection *collection)
{
  for (size_t i = 0; i < collection->macros.count; ++i)
  {
    free_osc_macro(&collection->macros.items[i]);
  }

  vec_free(collection->macros);
  vec_free(collection->response_factories);
}