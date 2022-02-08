#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "format.h"
#include "str_builder.h"

/* TODO: not actually certain if vscode distinguishes between no match and
 * capture of length 0 (on 2021-08-31) */
#define CAPTURE(cap, i) (cap)[2*i]
#define CAPTURE_END(cap, i) (cap)[2*i+1]
#define CAPTURE_LEN(cap, i) (cap)[2*i+1] - (cap)[2*i]

struct Trafo {
	struct Format **fmts;
	size_t size;
	bool has_else;
};

typedef void (*apply_fun)(const struct Format*, str_builder_t*, char**, int);

struct Format {
	int idx;
	wchar_t *arg1;
	wchar_t *arg2;
	apply_fun apply;
};


struct Format *format_create(int idx, const wchar_t *arg1, const wchar_t *arg2, apply_fun apply)
{
	struct Format *f = malloc(sizeof(*f));
	f->idx = idx;
	f->arg1 = arg1 ? wcsdup(arg1) : NULL;
	f->arg2 = arg2 ? wcsdup(arg2) : NULL;
	f->apply = apply;
	return f;
}


static void format_destroy(struct Format *f)
{
	if (!f)
		return;
	free(f->arg1);
	free(f->arg2);
	free(f);
}


static inline void format_apply(struct Format *f, str_builder_t *sb, char **captures, int capture_count)
{
	f->apply(f, sb, captures, capture_count);
}


static int haswprefix(const wchar_t *string, const wchar_t *prefix)
{
	while (*prefix) {
		if (*prefix++ != *string++) {
			return 0;
		}
	}
	return 1;
}


// normal text
static void apply_const(const struct Format *t, str_builder_t *sb, char **captures, int capture_count)
{
	str_builder_add_wstr(sb, t->arg1, 0);
}


// ${idx}
// $idx
static void apply_group(const struct Format *t, str_builder_t *sb, char **captures, int capture_count)
{
	if (t->idx < capture_count
			&& CAPTURE(captures, t->idx)
			&& CAPTURE_LEN(captures, t->idx) > 0) {
		str_builder_add_str(sb, CAPTURE(captures, t->idx), CAPTURE_LEN(captures, t->idx));
	}
}


// ${idx:/upcase}
static void apply_upcase(const struct Format *t, str_builder_t *sb, char **captures, int capture_count)
{
	char *s;
	wchar_t wc;
	if (t->idx < capture_count && (s = CAPTURE(captures, t->idx))) {
		char *end = CAPTURE_END(captures, t->idx);
		while (s < end) {
			int l = mbtowc(&wc, s, end-s);
			if (l > 0) {
				s += l;
				str_builder_add_wchar(sb, towupper(wc));
			} else {
				// conversion error
				s = end;
			}
		}
	}
}


// ${idx:/downcase}
static void apply_downcase(const struct Format *t, str_builder_t *sb, char **captures, int capture_count)
{
	char *s;
	wchar_t wc;
	if (t->idx < capture_count && (s = CAPTURE(captures, t->idx))) {
		char *end = CAPTURE_END(captures, t->idx);
		while (s < end) {
			int l = mbtowc(&wc, s, end-s);
			if (l > 0) {
				s += l;
				str_builder_add_wchar(sb, towlower(wc));
			} else {
				// conversion error
				s = end;
			}
		}
	}
}


// ${idx:/capitalize}
static void apply_capitalize(const struct Format *t, str_builder_t *sb, char **captures, int capture_count)
{
	char *s;
	wchar_t wc;
	if (t->idx < capture_count && (s = CAPTURE(captures, t->idx))) {
		char *end = CAPTURE_END(captures, t->idx);
		bool capitalize = true;
		while (s < end) {
			int l = mbtowc(&wc, s, end-s);
			if (l > 0) {
				s += l;
				if (capitalize) {
					str_builder_add_wchar(sb, towupper(wc));
					if (iswalpha(wc)) {
						capitalize = false;
					}
				} else {
					str_builder_add_wchar(sb, towlower(wc));
					if (!iswalpha(wc)) {
						capitalize = true;
					}
				}
			} else {
				// conversion error
				s = end;
			}
		}
	}
}


// ${idx:+t->arg1}
static void apply_if(const struct Format *t, str_builder_t *sb, char **captures, int capture_count)
{
	if (t->idx < capture_count
			&& CAPTURE(captures, t->idx)
			&& CAPTURE_LEN(captures, t->idx) > 0) {
		str_builder_add_wstr(sb, t->arg1, 0);
	}
}


// ${idx:-t->arg1}
// ${idx:t->arg1}
static void apply_else(const struct Format *t, str_builder_t *sb, char **captures, int capture_count)
{
	if (t->idx < capture_count
			&& CAPTURE(captures, t->idx)
			&& CAPTURE_LEN(captures, t->idx) > 0) {
		str_builder_add_str(sb, CAPTURE(captures, t->idx), CAPTURE_LEN(captures, t->idx));
	} else {
		str_builder_add_wstr(sb, t->arg1, 0);
	}
}


// ${idx:?t->arg1:t->arg2}
static void apply_ifelse(const struct Format *t, str_builder_t *sb, char **captures, int capture_count)
{
	if (t->idx < capture_count
			&& CAPTURE(captures, t->idx)
			&& CAPTURE_LEN(captures, t->idx) > 0) {
		str_builder_add_wstr(sb, t->arg1, 0);
	} else {
		str_builder_add_wstr(sb, t->arg2, 0);
	}
}


struct scanner {
	const wchar_t *str;
	const wchar_t *pos;
	wchar_t *buf;
	int buf_ind;
};


static wchar_t scanner_peek(struct scanner *s)
{
	return *s->pos;
}


static wchar_t scanner_pop(struct scanner *s)
{
	return *s->pos++;
}


// behaves only if we know c exists
static void scanner_seek(struct scanner *s, wchar_t c)
{
	while (scanner_pop(s) != c);
}


// If scan_to_end is not zero, then reaching L'\0' is not an error, otherwise
// NULL is returned.
static wchar_t *scanner_scan(struct scanner *s, wchar_t c, int scan_to_end)
{
	wchar_t *const res = s->buf + s->buf_ind;
	wchar_t w;
	while ((w = scanner_peek(s))) {
		if (w == c) {
			s->buf[s->buf_ind++] = L'\0';
			return res;
		}
		if (w == L'\\')
			scanner_pop(s);
		s->buf[s->buf_ind++] = scanner_pop(s);
	}
	if (scan_to_end) {
		s->buf[s->buf_ind++] = L'\0';
		return res;
	}
	return NULL;
}


static struct Format *parse_transform(struct scanner *s, char *err, int err_len)
{
	/* printf("parse_transform %S\n", w); */
	wchar_t w, *arg1, *arg2;
	int idx = 0, simple = 1;
	apply_fun f;

	if ((w = scanner_pop(s)) != L'$') {
		snprintf(err, err_len, "malformed placeholder: expected '$', found %c", w);
		return NULL;
	}
	if ((w = scanner_peek(s)) == L'{') {
		scanner_pop(s);
		simple = 0;
		w = scanner_peek(s);
	}
	if (!iswdigit(w)) {
		snprintf(err, err_len, "malformed placeholder: expected number or '{', found %c", w);
		return NULL;
	}

	while (iswdigit(scanner_peek(s))) {
		idx = idx * 10 + (scanner_pop(s) - '0');
	}
	if (simple) {
		return format_create(idx, NULL, NULL, apply_group);
	}

	switch (w = scanner_peek(s)) {
		case L'}':
			scanner_pop(s);
			return format_create(idx, NULL, NULL, apply_group);
			break;
		case L':':
			scanner_pop(s);
			break;
		default:
			snprintf(err, err_len, "malformed placeholder: expected ':' or '}', found %c", w);
			return NULL;
	}

	switch (w = scanner_peek(s)) {
		case L'/':
			if (haswprefix(s->pos, L"/upcase}")) {
				f = apply_upcase;
			} else if (haswprefix(s->pos, L"/downcase}")) {
				f = apply_downcase;
			} else if (haswprefix(s->pos, L"/capitalize}")) {
				f = apply_capitalize;
			} else if (haswprefix(s->pos, L"/pascalcase}")) {
				/* TODO:  (on 2021-08-29) */
				f = apply_group;
			} else if (haswprefix(s->pos, L"/camelcase}")) {
				/* TODO:  (on 2021-08-29) */
				f = apply_group;
			} else {
				snprintf(err, err_len, "malformed placeholder: unexpected transform %S", s->pos);
				return NULL;
			}
			scanner_seek(s, L'}');
			return format_create(idx, NULL, NULL, f);
		case L'+':
			scanner_pop(s);
			if (!(arg1 = scanner_scan(s, L'}', 0))) {
				snprintf(err, err_len, "malformed placeholder: missing '}'");
				return NULL;
			}
			scanner_pop(s);
			return format_create(idx, arg1, NULL, apply_if);
		case L'?':
			scanner_pop(s);
			if (!(arg1 = scanner_scan(s, L':', 0))) {
				snprintf(err, err_len, "malformed placeholder: missing ':'");
				return NULL;
			}
			scanner_pop(s);
			if (!(arg2 = scanner_scan(s, L'}', 0))) {
				snprintf(err, err_len, "malformed placeholder: missing '}'");
				return NULL;
			}
			scanner_pop(s);
			return format_create(idx, arg1, arg2, apply_ifelse);
		case L'-':
			scanner_pop(s);
		default:
			if (!(arg1 = scanner_scan(s, L'}', 0))) {
				snprintf(err, err_len, "malformed placeholder: missing '}'");
				return NULL;
			}
			scanner_pop(s);
			return format_create(idx, arg1, NULL, apply_else);
	}
}


void trafo_destroy(struct Trafo *trafo)
{
	if (!trafo)
		return;

	for (size_t i = 0; i < trafo->size; i++) {
		format_destroy(trafo->fmts[i]);
	}
	free(trafo->fmts);
	free(trafo);
}


bool trafo_has_else(struct Trafo *trafo)
{
	return trafo->has_else;
}


void trafo_apply(struct Trafo *trafo, str_builder_t *sb, char **captures, int capture_count)
{
	for (size_t i = 0; i < trafo->size; i++) {
		format_apply(trafo->fmts[i], sb, captures, capture_count);
	}
}


Trafo *trafo_create(const char *formats, char *err, int err_len)
{
	/* printf("format_create %s\n", format); */

	struct Trafo *trafo = malloc(sizeof(*trafo));
	trafo->fmts = malloc(sizeof(*trafo->fmts) * 4);
	trafo->size = 0;
	trafo->has_else = false;
	size_t capacity = 4;

	mbstate_t state;
	memset(&state, 0, sizeof(state));
	const size_t l = mbsrtowcs(NULL, &formats, 0, &state);
	wchar_t *wformat = calloc(l + 1, sizeof(wchar_t)) ;
	wchar_t *buf = calloc(l + 1, sizeof(wchar_t));
	mbsrtowcs(wformat, &formats, l + 1, &state);

	struct scanner s = {
		.str = wformat,
		.pos = wformat,
		.buf = buf,
	};

	struct Format *t;
	wchar_t *str;
	while (scanner_peek(&s)) {
		if ((str = scanner_scan(&s, L'$', 1)) && str[0] != L'\0') {
			if (trafo->size + 1 >= capacity) {
				capacity *= 2;
				trafo->fmts = realloc(trafo->fmts, capacity * sizeof(*trafo->fmts));
			}
			trafo->fmts[trafo->size++] = format_create(0, str, NULL, apply_const);
		}
		if (scanner_peek(&s)) {
			if (!(t = parse_transform(&s, err, err_len))) {
				trafo_destroy(trafo);
				trafo = NULL;
				break;
			}
			if (t->apply == apply_else || t->apply == apply_ifelse) {
				trafo->has_else = true;
			}
			trafo->fmts[trafo->size++] = t;
		}
	}
	free(buf);
	free(wformat);
	return trafo;
}
