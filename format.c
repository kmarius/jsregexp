#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#include "format.h"
#include "str_builder.h"

/* TODO: not actually certain if vscode distinguishes between no match and
 * capture of length 0 (on 2021-08-31) */
#define CAPTURE(cap, i) (cap)[2*i]
#define CAPTURE_END(cap, i) (cap)[2*i+1]
#define CAPTURE_LEN(cap, i) (cap)[2*i+1] - (cap)[2*i]

struct format_t {
	struct trafo_t **trafos;
	int size;
};

typedef void (*apply_fun)(const struct trafo_t*, str_builder_t*, char**, int);

struct trafo_t {
	int idx;
	wchar_t *arg1;
	wchar_t *arg2;
	apply_fun apply;
};

struct trafo_t *trafo_create(int idx, const wchar_t *arg1, const wchar_t *arg2, apply_fun apply)
{
	struct trafo_t *t = malloc(sizeof(*t));
	if (!t)
		return NULL;
	t->idx = idx;
	t->arg1 = arg1 ? wcsdup(arg1) : NULL;
	t->arg2 = arg2 ? wcsdup(arg2) : NULL;
	t->apply = apply;
	return t;
}

static void trafo_destroy(struct trafo_t *t)
{
	if (!t)
		return;
	free(t->arg1);
	free(t->arg2);
	free(t);
}

static inline void trafo_apply(struct trafo_t *t, str_builder_t *sb, char **captures, int capture_count)
{
	t->apply(t, sb, captures, capture_count);
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
static void apply_const(const struct trafo_t *t, str_builder_t *sb, char **captures, int capture_count)
{
	str_builder_add_wstr(sb, t->arg1, 0);
}

// ${idx}
// $idx
static void apply_group(const struct trafo_t *t, str_builder_t *sb, char **captures, int capture_count)
{
	if (t->idx < capture_count) {
		if (CAPTURE(captures, t->idx) && CAPTURE_LEN(captures, t->idx) > 0) {
			str_builder_add_str(sb, CAPTURE(captures, t->idx), CAPTURE_LEN(captures, t->idx));
		}
	}
}

// ${idx:/upcase}
static void apply_upcase(const struct trafo_t *t, str_builder_t *sb, char **captures, int capture_count)
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
static void apply_downcase(const struct trafo_t *t, str_builder_t *sb, char **captures, int capture_count)
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
static void apply_capitalize(const struct trafo_t *t, str_builder_t *sb, char **captures, int capture_count)
{
	char *s;
	wchar_t wc;
	if (t->idx < capture_count && (s = CAPTURE(captures, t->idx))) {
		char *end = CAPTURE_END(captures, t->idx);
		int cap = 1; /* capitalize next char */
		while (s < end) {
			int l = mbtowc(&wc, s, end-s);
			if (l > 0) {
				s += l;
				if (cap) {
					str_builder_add_wchar(sb, towupper(wc));
					if (iswalpha(wc)) {
						cap = 0;
					}
				} else {
					str_builder_add_wchar(sb, towlower(wc));
					if (!iswalpha(wc)) {
						cap = 1;
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
static void apply_if(const struct trafo_t *t, str_builder_t *sb, char **captures, int capture_count)
{
	if (t->idx < capture_count
			&& CAPTURE(captures, t->idx)
			&& CAPTURE_LEN(captures, t->idx) > 0) {
		str_builder_add_wstr(sb, t->arg1, 0);
	}
}

// ${idx:-t->arg1}
// ${idx:t->arg1}
static void apply_else(const struct trafo_t *t, str_builder_t *sb, char **captures, int capture_count)
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
static void apply_ifelse(const struct trafo_t *t, str_builder_t *sb, char **captures, int capture_count)
{
	if (t->idx < capture_count
			&& CAPTURE(captures, t->idx)
			&& CAPTURE_LEN(captures, t->idx) > 0) {
		str_builder_add_wstr(sb, t->arg1, 0);
	} else {
		str_builder_add_wstr(sb, t->arg2, 0);
	}
}

typedef struct scanner_t {
	const wchar_t *str;
	const wchar_t *pos;
	wchar_t *buf;
	int buf_ind;
} scanner_t;

static wchar_t scanner_peek(scanner_t *s)
{
	return *s->pos;
}

static wchar_t scanner_pop(scanner_t *s)
{
	return *s->pos++;
}

// If scan_to_end is not zero, then reaching L'\0' is not an error, otherwise
// NULL is returned.
static wchar_t *scanner_scan(scanner_t *s, wchar_t c, int scan_to_end)
{
	wchar_t * const res = s->buf+s->buf_ind;
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

static struct trafo_t *parse_transform(scanner_t *s, char *err, int err_len)
{
	/* printf("parse_transform %S\n", w); */
	wchar_t w, *arg1, *arg2;
	int idx = 0, simple = 1;
	apply_fun f;
	struct trafo_t *res;

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
		res = trafo_create(idx, NULL, NULL, apply_group);
		goto ret;
	}

	switch (w = scanner_peek(s)) {
		case L'}':
			scanner_pop(s);
			res = trafo_create(idx, NULL, NULL, apply_group);
			goto ret;
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
				s->pos += wcslen(L"/upcase}");
				f = apply_upcase;
			} else if (haswprefix(s->pos, L"/downcase}")) {
				s->pos += wcslen(L"/downcase}");
				f = apply_downcase;
			} else if (haswprefix(s->pos, L"/capitalize}")) {
				s->pos += wcslen(L"/capitalize}");
				f = apply_capitalize;
			} else if (haswprefix(s->pos, L"/pascalcase}")) {
				/* TODO:  (on 2021-08-29) */
				s->pos += wcslen(L"/pascalcase}");
				f = apply_group;
			} else if (haswprefix(s->pos, L"/camelcase}")) {
				/* TODO:  (on 2021-08-29) */
				s->pos += wcslen(L"/camelcase}");
				f = apply_group;
			} else {
				snprintf(err, err_len, "malformed placeholder: unexpected transform %S", s->pos);
				return NULL;
			}
			res = trafo_create(idx, NULL, NULL, f);
			break;
		case L'+':
			scanner_pop(s);
			if (!(arg1 = scanner_scan(s, L'}', 0))) {
				snprintf(err, err_len, "malformed placeholder: missing '}'");
				return NULL;
			}
			scanner_pop(s);
			res = trafo_create(idx, arg1, NULL, apply_if);
			break;
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
			res = trafo_create(idx, arg1, arg2, apply_ifelse);
			break;
		case L'-':
			scanner_pop(s);
		default:
			if (!(arg1 = scanner_scan(s, L'}', 0))) {
				snprintf(err, err_len, "malformed placeholder: missing '}'");
				return NULL;
			}
			scanner_pop(s);
			res = trafo_create(idx, arg1, NULL, apply_else);
	}

ret:
	if (!res) {
		snprintf(err, err_len, "format_create: malloc failed");
		return NULL;
	}
	return res;
}

void format_destroy(struct format_t *fmt)
{
	int i;
	if (!fmt)
		return;
	for (i = 0; i < fmt->size; i++) {
		trafo_destroy(fmt->trafos[i]);
	}
	free(fmt->trafos);
	free(fmt);
}

void format_apply(struct format_t *fmt, str_builder_t *sb, char **captures, int capture_count)
{
	int i;
	for (i = 0; i < fmt->size; i++) {
		trafo_apply(fmt->trafos[i], sb, captures, capture_count);
	}
}

format_t *format_create(const char *format, char *err, int err_len)
{
	/* printf("format_create %s\n", format); */
	struct trafo_t *t;
	mbstate_t state;
	wchar_t *str;

	memset(&state, 0, sizeof state);
	const int l = mbsrtowcs(NULL, &format, 0, &state);
	wchar_t wformat[l+1], buf[l+1];
	mbsrtowcs(wformat, &format, l + 1, &state);

	scanner_t s = {
		.str = wformat,
		.pos = wformat,
		.buf = buf,
		.buf_ind = 0,
	};

	struct format_t *fmt = malloc(sizeof(*fmt));
	if (!fmt) {
		snprintf(err, err_len, "format_create: malloc failed");
		return NULL;
	}

	fmt->trafos = malloc(sizeof(*fmt->trafos) * 4);
	if (!fmt->trafos) {
		snprintf(err, err_len, "format_create: malloc failed");
		free(fmt);
		return NULL;
	}
	fmt->size = 0;
	int capacity = 4;

	while (scanner_peek(&s)) {
		if ((str = scanner_scan(&s, L'$', 1)) && str[0] != L'\0') {
			if (fmt->size + 1 >= capacity) {
				capacity *= 2;
				struct trafo_t **tmp = realloc(fmt->trafos, sizeof(*tmp)*capacity);
				if (!tmp) {
					snprintf(err, err_len, "format_create: realloc failed");
					format_destroy(fmt);
					return NULL;
				}
				fmt->trafos = tmp;
			}
			if (!(t = trafo_create(0, str, NULL, apply_const))) {
				format_destroy(fmt);
				snprintf(err, err_len, "format_create: malloc failed");
				return NULL;
			}
			fmt->trafos[fmt->size++] = t;
		}
		if (scanner_peek(&s)) {
			if (!(t = parse_transform(&s, err, err_len))) {
				format_destroy(fmt);
				return NULL;
			}
			fmt->trafos[fmt->size++] = t;
		}
	}
	return fmt;
}
