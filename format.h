#ifndef TRANSFORMATION_H
#define TRANSFORMATION_H

#include "str_builder.h"

struct format_t;
typedef struct format_t format_t;

format_t *format_create(const char *format, char *err, int err_len);
void format_destroy(format_t *fmt);
void format_apply(format_t *fmt, str_builder_t *sb, char **captures, int capture_count);
int format_has_else(struct format_t *fmt);

#endif
