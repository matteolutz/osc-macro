#ifndef OSC_SNIPPET_H
#define OSC_SNIPPET_H

#ifndef OSC_MACRO_RESPONSES_CAPACITY
#define OSC_MACRO_RESPONSES_CAPACITY 8
#endif

#ifndef OSC_MAX_MACROS
#define OSC_MAX_MACROS 16
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

typedef struct osc_macro_collection
{
  osc_macro macros[OSC_MAX_MACROS];
  uint32_t macro_count;
} osc_macro_collection;

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
 * The maximum number of responses is defined by OSC_MACRO_RESPONSES_CAPACITY. To change this value,
 * define OSC_MACRO_RESPONSES_CAPACITY before including osc_snippet.h.
 *
 * This function derives substrings from the given snippet so the char*
 * passed to it has to be mutable in order to 0-terminate the substrings.
 *
 * Returns a pointer to the first character after the last response of the macro or NULL if parsing failed.
 */
char *parse_osc_macro(char *macro, osc_macro *out_macro);

/**
 * Parse a collection of OSC macros.
 * An OSC macro collection is a list of macros separated by newlines.
 * Any whitespace or newlines before the first macro, in between macros, or after the last macro is ignored.
 *
 * The maximum number of macros is defined by OSC_MAX_MACROS. To change this value,
 * define OSC_MAX_MACROS before including osc_snippet.h.
 *
 * This function derives substrings from the given snippet so the char*
 * passed to it has to be mutable in order to 0-terminate the substrings.
 *
 * Returns a pointer to the first character after the last macro of the collection or NULL if parsing failed.
 */
char *parse_osc_macro_collection(char *macro_collection, osc_macro_collection *out_macro_collection);

/**
 * Find a macro in the given collection that has a trigger message equal to the given trigger_message.
 *
 * This will not consume the `tosc_message`, because it internally calls `tosc_reset`.
 *
 * Returns a pointer to the found macro or NULL if no matching macro was found.
 */
osc_macro *find_macro_by_trigger_message(osc_macro_collection *collection, tosc_message *trigger_message);

#endif // OSC_SNIPPET_H
