#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#include "format.h"
#include "str_builder.h"

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
	t->idx = idx;
	t->arg1 = arg1 ? wcsdup(arg1) : NULL;
	t->arg2 = arg2 ? wcsdup(arg2) : NULL;
	t->apply = apply;
	return t;
}

static void trafo_destroy(struct trafo_t *t)
{
	if (t) {
		free(t->arg1);
		free(t->arg2);
		free(t);
	}
}

static inline void trafo_apply(struct trafo_t *t, str_builder_t *sb, char **captures, int capture_count)
{
	t->apply(t, sb, captures, capture_count);
}

static int wcshasprefix(const wchar_t *string, const wchar_t *prefix)
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
					str_builder_add_char(sb, towupper(wc));
					if (iswalpha(wc)) {
						cap = 0;
					}
				} else {
					str_builder_add_char(sb, towlower(wc));
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

// on success, ww should point at the last consumed character
static struct trafo_t *parse_transform(const wchar_t **ww,
		wchar_t *buf, char *err, int err_len)
{
	/* printf("parse_transform %S\n", w); */
	int idx = 0, done;
	const wchar_t *w = *ww;
	int buf_ind = 0;

	int simple = 0;
	if (isdigit(*w)) {
		simple = 1;
	} else if (*w == L'{') {
		w++;
	}

	if (!isdigit(*w)) {
		snprintf(err, err_len, "malformed placeholder: expected number, found %c", *w);
		return NULL;
	}
	while (isdigit(*w)) {
		idx = idx * 10 + (*w++ - '0');
	}
	if (simple) {
		*ww = w;
		return trafo_create(idx, NULL, NULL, apply_group);
	}
	if (*w == L'}') {
		*ww = w;
		return trafo_create(idx, NULL, NULL, apply_group);
	}
	if (*w != L':') {
		snprintf(err, err_len, "malformed placeholder: expected ':', found %c", *w);
		return NULL;
	}
	switch (*(++w)) {
		case L'/':
			{
				apply_fun f;
				if (wcshasprefix(w, L"/upcase}")) {
					*ww = w + wcslen(L"/upcase");
					f = apply_upcase;
				} else if (wcshasprefix(w, L"/downcase}")) {
					*ww = w + wcslen(L"/downcase");
					f = apply_downcase;
				} else if (wcshasprefix(w, L"/capitalize}")) {
					*ww = w + wcslen(L"/capitalize");
					f = apply_capitalize;
				} else if (wcshasprefix(w, L"/pascalcase}")) {
					/* TODO:  (on 2021-08-29) */
					*ww = w + wcslen(L"/pascalcase");
					f = apply_group;
				} else if (wcshasprefix(w, L"/camelcase}")) {
					/* TODO:  (on 2021-08-29) */
					*ww = w + wcslen(L"/camelcase");
					f = apply_group;
				} else {
					snprintf(err, err_len, "malformed placeholder: unexpected transform %S", w+1);
					return NULL;
				}
				return trafo_create(idx, NULL, NULL, f);
			}
		case L'+':
			{
				w++;
				for (done = 0; !done && *w; ) {
					switch (*w) {
						case L'}':
							done = 1;
							break;
						case L'\\':
							if (!*(++w))
								break;
						default:
							buf[buf_ind++] = *w++;
					}
				}
				if (!done) {
					snprintf(err, err_len, "malformed placeholder: missing '}'");
					return NULL;
				}
				buf[buf_ind] = L'\0';
				*ww = w;
				return trafo_create(idx, wcsdup(buf), NULL, apply_if);
			}
		case L'?':
			{
				w++;
				for (done = 0; !done && *w; ) {
					switch (*w) {
						case L':':
							done = 1;
							break;
						case L'\\':
							if (!*(++w))
								break;
						default:
							buf[buf_ind++] = *w++;
					}
				}
				if (!done) {
					snprintf(err, err_len, "malformed placeholder: missing ':'");
					return NULL;
				}

				buf[buf_ind++] = L'\0';
				wchar_t *arg2 = buf + buf_ind;

				w++;
				for (done = 0; !done && *w; ) {
					switch (*w) {
						case L'}':
							done = 1;
							break;
						case L'\\':
							if (!*(++w))
								break;
						default:
							buf[buf_ind++] = *w++;
					}
				}

				if (!done) {
					snprintf(err, err_len, "malformed placeholder: missing '}'");
					return NULL;
				}

				buf[buf_ind] = L'\0';

				*ww = w;
				return trafo_create(idx, wcsdup(buf), wcsdup(arg2), apply_ifelse);
			}
		default:
			if (*w == L'-') {
				w++;
			}
			for (done = 0; !done && *w; ) {
				switch (*w) {
					case L'}':
						done = 1;
						break;
					case L'\\':
						if (!*(++w))
							break;
					default:
						buf[buf_ind++] = *w++;
				}
			}
			if (!done) {
				snprintf(err, err_len, "malformed placeholder: missing '}'");
				return NULL;
			}
			buf[buf_ind] = L'\0';
			*ww = w;
			return trafo_create(idx, wcsdup(buf), NULL, apply_else);
	}
}

void format_destroy(struct format_t *fmt)
{
	int i;
	if (fmt) {
		for (i = 0; i < fmt->size; i++) {
			trafo_destroy(fmt->trafos[i]);
		}
		free(fmt->trafos);
		free(fmt);
	}
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
	const wchar_t *w;
	struct trafo_t *t;
	mbstate_t state;

	memset(&state, 0, sizeof state);
	const int l = mbsrtowcs(NULL, &format, 0, &state);
	wchar_t wformat[l+1], buf[l+1];
	mbsrtowcs(wformat, &format, l+1, &state);

	int buf_ind = 0;

	struct format_t *fmt = malloc(sizeof(*fmt));
	fmt->trafos = malloc(sizeof(struct trafo_t*) * 4);
	fmt->size = 0;
	int capacity = 4;

	for (w = wformat; *w; w++) {
		switch (*w) {
			case L'$':
				// make space for (at least) two more
				if (fmt->size + 1 >= capacity) {
					capacity *= 2;
					fmt->trafos = realloc(fmt->trafos, sizeof(*fmt->trafos)*capacity);
				}

				if (buf_ind > 0) {
					buf[buf_ind] = L'\0';
					fmt->trafos[fmt->size++] = trafo_create(0, buf, NULL, apply_const);
					buf[0] = L'\0';
					buf_ind = 0;
				}

				w++;
				if (NULL == (t = parse_transform(&w, buf, err, err_len))) {
					format_destroy(fmt);
					return NULL;
				}
				fmt->trafos[fmt->size++] = t;
				break;
			case L'\\':
				if (!*(++w))
					break; /* ignore \ at the very end? */
			default:
				buf[buf_ind++] = *w;
		}
	}

	if (buf_ind > 0) {
		if (fmt->size >= capacity) {
			capacity++;
			fmt->trafos = realloc(fmt->trafos, sizeof(*fmt->trafos)*capacity);
		}

		buf[buf_ind] = L'\0';
		fmt->trafos[fmt->size++] = trafo_create(0, buf, NULL, apply_const);
	}
	return fmt;
}
