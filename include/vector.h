/**
 * Vector implementation in C.
 *
 * Code based on (nob.h)[https://github.com/tsoding/nob.h]
 * Copyright (c) 2024 Alexey Kutepov
   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
 */

#ifndef _VECTOR_H
#define _VECTOR_H

#ifndef VEC_REALLOC
#include <stdlib.h>
#define VEC_REALLOC realloc
#endif

#ifndef VEC_FREE
#include <stdlib.h>
#define VEC_FREE free
#endif

#ifndef VEC_ASSERT
#include <assert.h>
#define VEC_ASSERT assert
#endif

#include <stddef.h>

#define VECTOR(type, name) \
    struct                 \
    {                      \
        type *items;       \
        size_t size;       \
        size_t capacity;   \
    } name

#define vector_reserve(vec, expected_capacity)                                                 \
    do                                                                                         \
    {                                                                                          \
        if ((expected_capacity) > (vec)->capacity)                                             \
        {                                                                                      \
            if ((vec)->capacity == 0)                                                          \
            {                                                                                  \
                (vec)->capacity = 4;                                                           \
            }                                                                                  \
            while ((expected_capacity) > (vec)->capacity)                                      \
            {                                                                                  \
                (vec)->capacity *= 2;                                                          \
            }                                                                                  \
            (vec)->items = VEC_REALLOC((vec)->items, (vec)->capacity * sizeof(*(vec)->items)); \
            VEC_ASSERT((vec)->items != NULL && "[Vec] Failed to realloc items");               \
        }                                                                                      \
    } while (0)

/** Append item to vector */
#define vec_push(vec, item)                    \
    do                                         \
    {                                          \
        vec_reserve((vec), (vec)->count + 1);  \
        (vec)->items[(vec)->count++] = (item); \
    } while (0)

/** Free a vector. Require vector being moved by value */
#define vec_free(vec) VEC_FREE((vec).items)

// Append several items to a dynamic array
#define vec_extend(vec, new_items, new_items_count)                                                  \
    do                                                                                               \
    {                                                                                                \
        vec_reserve((vec), (vec)->count + (new_items_count));                                        \
        memcpy((vec)->items + (vec)->count, (new_items), (new_items_count) * sizeof(*(vec)->items)); \
        (vec)->count += (new_items_count);                                                           \
    } while (0)

#define vec_resize(da, new_size)      \
    do                                \
    {                                 \
        vec_reserve((vec), new_size); \
        (vec)->count = (new_size);    \
    } while (0)

#define vec_pop(vec) (vec)->items[(VEC_ASSERT((vec)->count > 0), --(vec)->count)]
#define vec_first(vec) (vec)->items[(VEC_ASSERT((vec)->count > 0), 0)]
#define vec_last(vec) (vec)->items[(VEC_ASSERT((vec)->count > 0), (vec)->count - 1)]

#endif // _VECTOR_H
