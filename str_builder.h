// Copyright John Schember <john@nachtimwald.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __STR_BUILDER_H__
#define __STR_BUILDER_H__

#include <stddef.h>
#include <wchar.h>

/*! addtogroup str_builder String Builder
 *
 * A mutable string of characters used to dynamically build a string.
 *
 * @{
 */

struct str_builder;
typedef struct str_builder str_builder_t;

/* - - - - */

/*! Create a str builder.
 *
 * return str builder.
 */
str_builder_t *str_builder_create(void);

/*! Destroy a str builder.
 *
 * param[in,out] sb Builder.
 */
void str_builder_destroy(str_builder_t *sb);

/* - - - - */

/*! Add a string to the builder.
 *
 * param[in,out] sb  Builder.
 * param[in]     str String to add.
 * param[in]     len Length of string to add. If 0, strlen will be called
 *                internally to determine length.
 */
void str_builder_add_str(str_builder_t *sb, const char *str, size_t len);
void str_builder_add_wstr(str_builder_t *sb, const wchar_t *str, size_t len);

/*! Add a character to the builder.
 *
 * param[in,out] sb Builder.
 * param[in]     c  Character.
 */
void str_builder_add_char(str_builder_t *sb, char c);
void str_builder_add_wchar(str_builder_t *sb, wchar_t c);

/*! Add an integer as to the builder.
 *
 * param[in,out] sb  Builder.
 * param[in]     val Int to add.
 */
void str_builder_add_int(str_builder_t *sb, int val);

/* - - - - */

/*! Clear the builder.
 *
 * param[in,out] sb  Builder.
 */
void str_builder_clear(str_builder_t *sb);

/*! Remove data from the end of the builder.
 *
 * param[in,out] sb  Builder.
 * param[in]     len The new length of the string.
 *                    Anything after this length is removed.
 */
void str_builder_truncate(str_builder_t *sb, size_t len);

/*! Remove data from the beginning of the builder.
 *
 * param[in,out] sb  Builder.
 * param[in]     len The length to remove.
 */
void str_builder_drop(str_builder_t *sb, size_t len);

/* - - - - */

/*! The length of the string contained in the builder.
 *
 * param[in] sb Builder.
 *
 * return Length.
 */
size_t str_builder_len(const str_builder_t *sb);

/*! A pointer to the internal buffer with the builder's string data.
 *
 * The data is guaranteed to be NULL terminated.
 *
 * param[in] sb Builder.
 *
 * return Pointer to internal string data.
 */
const char *str_builder_peek(const str_builder_t *sb);

/*! Return a copy of the string data.
 *
 * param[in]  sb  Builder.
 * param[out] len Length of returned data. Can be NULL if not needed.
 *
 * return Copy of the internal string data.
 */
char *str_builder_dump(const str_builder_t *sb, size_t *len);

/*! @}
 */

#endif /* __STR_BUILDER_H__ */
