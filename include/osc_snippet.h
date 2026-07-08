#ifndef OSC_SNIPPET_H
#define OSC_SNIPPET_H

#ifndef OSC_MACRO_RESPONSES_CAPACITY
#define OSC_MACRO_RESPONSES_CAPACITY 8
#endif

#include "tinyosc.h"

typedef struct osc_snippet
{
  tosc_message_builder message_builder;
} osc_snippet;

typedef struct osc_macro
{
  osc_snippet trigger;

  osc_snippet responses[OSC_MACRO_RESPONSES_CAPACITY];
  uint32_t responses_count;
} osc_macro;

/**
 * Parse an OSC snippet in the form of
 *
 * /channel/1/test("hello"   1      1.2f     4.5d)
 *                 ^ string  ^ int  ^ float  ^ double
 *
 * Arguments are delimited by 1-n whitespaces.
 *
 * This function derives substrings from the given snippet so the char*
 * passed to it has to be mutable in order to 0-terminate the substrings.
 *
 * Returns a pointer to the first character after the closing ')' of the snippet or NULL if parsing failed.
 */
char *parse_osc_snippet(char *snippet, osc_snippet *out_snippet);

/**
 * Parse an OSC macro in the form of
 *
 * /channel/1/test("hello"   1   3.2f   4.5d)   > trigger
 * > /channel/1/another()                       > response 1
 * > /channel/1/third(1 2 3.14f "test)          > response 2
 *
 * A macro must have a trigger and can have 0-n responses. Each response must be on a new line and start with a '>'.
 *
 * This function derives substrings from the given snippet so the char*
 * passed to it has to be mutable in order to 0-terminate the substrings.
 *
 * Returns a pointer to the first character after the last response of the macro or NULL if parsing failed.
 */
char *parse_osc_macro(char *macro, osc_macro *out_macro);

#endif // OSC_SNIPPET_H
