#ifndef OSC_SNIPPET_H
#define OSC_SNIPPET_H

#include "tinyosc.h"

typedef struct osc_snippet {
  tosc_message_builder message_builder;
} osc_snippet;

/**
 * Parse an OSC snippet in the form of
 *
 * /channel/1/test("hello"   1      1.2f     4.5d)
 *                 ^ string  ^ int  ^ float  ^ double
 *
 * Arguments are delimited by 1-n whitespaces.
 *
 * This function derives substrings from the given snippet so the char*
 * passed to it has to be mutable in order to 0-terminate the substrings
 */
bool parse_osc_snippet(char *snippet, osc_snippet *out_snippet);

#endif // OSC_SNIPPET_H
