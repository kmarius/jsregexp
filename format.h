#ifndef TRANSFORMATION_H
#define TRANSFORMATION_H

#include <stdbool.h>

#include "str_builder.h"

struct Trafo;
typedef struct Trafo Trafo;

Trafo *trafo_create(const char *format, char *err, int err_len);
void trafo_destroy(Trafo *fmt);
void trafo_apply(Trafo *fmt, str_builder_t *sb, char **captures, int capture_count);
bool trafo_has_else(struct Trafo *fmt);

#endif
