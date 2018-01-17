

#include "proto.h"

#include <ctype.h>


#ifdef ENABLE_UTF8
#include <wchar.h>
#include <wctype.h>

static bool use_utf8 = FALSE;
		


void utf8_init(void)
{
	use_utf8 = TRUE;
}


bool using_utf8(void)
{
	return use_utf8;
}
#endif 


#include <string.h>
char *addstrings(char* str1, size_t len1, char* str2, size_t len2)
{
	str1 = charealloc(str1, len1 + len2 + 1);
	str1[len1] = '\0';

	strncat(&str1[len1], str2, len2);
	free(str2);

	return str1;
}


bool is_byte(int c)
{
	return ((unsigned int)c == (unsigned char)c);
}

void mbtowc_reset(void)
{
	IGNORE_CALL_RESULT(mbtowc(NULL, NULL, 0));
}


bool is_alpha_mbchar(const char *c)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc;

		if (mbtowc(&wc, c, MAXCHARLEN) < 0) {
			mbtowc_reset();
			return 0;
		}

		return iswalpha(wc);
	} else
#endif
		return isalpha((unsigned char)*c);
}


bool is_alnum_mbchar(const char *c)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc;

		if (mbtowc(&wc, c, MAXCHARLEN) < 0) {
			mbtowc_reset();
			return 0;
		}

		return iswalnum(wc);
	} else
#endif
		return isalnum((unsigned char)*c);
}


bool is_blank_mbchar(const char *c)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc;

		if (mbtowc(&wc, c, MAXCHARLEN) < 0) {
			mbtowc_reset();
			return 0;
		}

		return iswblank(wc);
	} else
#endif
		return isblank((unsigned char)*c);
}


bool is_ascii_cntrl_char(int c)
{
	return (0 <= c && c < 32);
}


bool is_cntrl_char(int c)
{
	return ((c & 0x60) == 0 || c == 127);
}


bool is_cntrl_mbchar(const char *c)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		return ((c[0] & 0xE0) == 0 || c[0] == 127 ||
				((signed char)c[0] == -62 && (signed char)c[1] < -96));
	} else
#endif
		return is_cntrl_char((unsigned char)*c);
}


bool is_punct_mbchar(const char *c)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc;

		if (mbtowc(&wc, c, MAXCHARLEN) < 0) {
			mbtowc_reset();
			return 0;
		}

		return iswpunct(wc);
	} else
#endif
		return ispunct((unsigned char)*c);
}


bool is_word_mbchar(const char *c, bool allow_punct)
{
	if (*c == '\0')
		return FALSE;

	if (is_alnum_mbchar(c))
		return TRUE;

	if (word_chars != NULL && *word_chars != '\0') {
		char symbol[MAXCHARLEN + 1];
		int symlen = parse_mbchar(c, symbol, NULL);

		symbol[symlen] = '\0';
		return (strstr(word_chars, symbol) != NULL);
	}

	return (allow_punct && is_punct_mbchar(c));
}


char control_rep(const signed char c)
{
	if (c == DEL_CODE)
		return '?';
	else if (c == -97)
		return '=';
	else if (c < 0)
		return c + 224;
	else
		return c + 64;
}


char control_mbrep(const char *c, bool isdata)
{
	
	if (*c == '\n' && (isdata || as_an_at))
		return '@';

#ifdef ENABLE_UTF8
	if (use_utf8) {
		if ((unsigned char)c[0] < 128)
			return control_rep(c[0]);
		else
			return control_rep(c[1]);
	} else
#endif
		return control_rep(*c);
}


int length_of_char(const char *c, int *width)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc;
		int charlen = mbtowc(&wc, c, MAXCHARLEN);

		
		if (charlen < 0) {
			mbtowc_reset();
			return -1;
		}

		
		if (!is_valid_unicode(wc))
			return charlen - 8;
		else {
			*width = wcwidth(wc);
			
			if (*width < 0)
				*width = 1;
			return charlen;
		}
	} else
#endif
		return 1;
}


int mbwidth(const char *c)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc;
		int width;

		if (mbtowc(&wc, c, MAXCHARLEN) < 0) {
			mbtowc_reset();
			return 1;
		}

		width = wcwidth(wc);

		if (width == -1)
			return 1;

		return width;
	} else
#endif
		return 1;
}


char *make_mbchar(long chr, int *chr_mb_len)
{
	char *chr_mb;

#ifdef ENABLE_UTF8
	if (use_utf8) {
		chr_mb = charalloc(MAXCHARLEN);
		*chr_mb_len = wctomb(chr_mb, (wchar_t)chr);

		
		if (*chr_mb_len < 0 || !is_valid_unicode((wchar_t)chr)) {
			IGNORE_CALL_RESULT(wctomb(NULL, 0));
			*chr_mb_len = 0;
		}
	} else
#endif
	{
		*chr_mb_len = 1;
		chr_mb = mallocstrncpy(NULL, (char *)&chr, 1);
	}

	return chr_mb;
}


int parse_mbchar(const char *buf, char *chr, size_t *col)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		
		int length = mblen(buf, MAXCHARLEN);

		
		if (length <= 0) {
			IGNORE_CALL_RESULT(mblen(NULL, 0));
			length = 1;
		}

		
		if (chr != NULL) {
			int i;

			for (i = 0; i < length; i++)
				chr[i] = buf[i];
		}

		
		if (col != NULL) {
			
			if (*buf == '\t')
				*col += tabsize - *col % tabsize;
			
			else if (is_cntrl_mbchar(buf)) {
				*col += 2;
			
			} else
				*col += mbwidth(buf);
		}

		return length;
	} else
#endif
	{
		
		if (chr != NULL)
			*chr = *buf;

		
		if (col != NULL) {
			
			if (*buf == '\t')
				*col += tabsize - *col % tabsize;
			
			else if (is_cntrl_char((unsigned char)*buf))
				*col += 2;
			
			else
				(*col)++;
		}

		return 1;
	}
}


size_t move_mbleft(const char *buf, size_t pos)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		size_t before, char_len = 0;

		if (pos < 4)
			before = 0;
		else {
			const char *ptr = buf + pos;

			
			if ((signed char)*(--ptr) > -65)
				before = pos - 1;
			else if ((signed char)*(--ptr) > -65)
				before = pos - 2;
			else if ((signed char)*(--ptr) > -65)
				before = pos - 3;
			else if ((signed char)*(--ptr) > -65)
				before = pos - 4;
			else
				before = pos - 1;
		}

		
		while (before < pos) {
			char_len = parse_mbchar(buf + before, NULL, NULL);
			before += char_len;
		}

		return before - char_len;
	} else
#endif
	return (pos == 0 ? 0 : pos - 1);
}


size_t move_mbright(const char *buf, size_t pos)
{
	return pos + parse_mbchar(buf + pos, NULL, NULL);
}


int mbstrcasecmp(const char *s1, const char *s2)
{
	return mbstrncasecmp(s1, s2, HIGHEST_POSITIVE);
}


int mbstrncasecmp(const char *s1, const char *s2, size_t n)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		wchar_t wc1, wc2;

		while (*s1 != '\0' && *s2 != '\0' && n > 0) {
			bool bad1 = FALSE, bad2 = FALSE;

			if (mbtowc(&wc1, s1, MAXCHARLEN) < 0) {
				mbtowc_reset();
				bad1 = TRUE;
			}

			if (mbtowc(&wc2, s2, MAXCHARLEN) < 0) {
				mbtowc_reset();
				bad2 = TRUE;
			}

			if (bad1 || bad2) {
				if (*s1 != *s2)
					return (unsigned char)*s1 - (unsigned char)*s2;

				if (bad1 != bad2)
					return (bad1 ? 1 : -1);
			} else {
				int difference = towlower(wc1) - towlower(wc2);

				if (difference != 0)
					return difference;
			}

			s1 += move_mbright(s1, 0);
			s2 += move_mbright(s2, 0);
			n--;
		}

		return (n > 0) ? ((unsigned char)*s1 - (unsigned char)*s2) : 0;
	} else
#endif
		return strncasecmp(s1, s2, n);
}


char *mbstrcasestr(const char *haystack, const char *needle)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		size_t needle_len = mbstrlen(needle);

		while (*haystack != '\0') {
			if (mbstrncasecmp(haystack, needle, needle_len) == 0)
				return (char *)haystack;

			haystack += move_mbright(haystack, 0);
		}

		return NULL;
	} else
#endif
		return (char *) strcasestr(haystack, needle);
}


char *revstrstr(const char *haystack, const char *needle,
		const char *pointer)
{
	size_t needle_len = strlen(needle);
	size_t tail_len = strlen(pointer);

	if (tail_len < needle_len)
		pointer += tail_len - needle_len;

	while (pointer >= haystack) {
		if (strncmp(pointer, needle, needle_len) == 0)
			return (char *)pointer;
		pointer--;
	}

	return NULL;
}


char *revstrcasestr(const char *haystack, const char *needle,
		const char *pointer)
{
	size_t needle_len = strlen(needle);
	size_t tail_len = strlen(pointer);

	if (tail_len < needle_len)
		pointer += tail_len - needle_len;

	while (pointer >= haystack) {
		if (strncasecmp(pointer, needle, needle_len) == 0)
			return (char *)pointer;
		pointer--;
	}

	return NULL;
}


char *mbrevstrcasestr(const char *haystack, const char *needle,
		const char *pointer)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		size_t needle_len = mbstrlen(needle);
		size_t tail_len = mbstrlen(pointer);

		if (tail_len < needle_len)
			pointer += tail_len - needle_len;

		if (pointer < haystack)
			return NULL;

		while (TRUE) {
			if (mbstrncasecmp(pointer, needle, needle_len) == 0)
				return (char *)pointer;

			if (pointer == haystack)
				return NULL;

			pointer = haystack + move_mbleft(haystack, pointer - haystack);
		}
	} else
#endif
		return revstrcasestr(haystack, needle, pointer);
}


size_t mbstrlen(const char *s)
{
	return mbstrnlen(s, (size_t)-1);
}


size_t mbstrnlen(const char *s, size_t maxlen)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		size_t n = 0;

		for (; *s != '\0' && maxlen > 0; s += move_mbright(s, 0),
				maxlen--, n++)
			;

		return n;
	} else
#endif
		return strnlen(s, maxlen);
}

#if !defined(NANO_TINY) || defined(ENABLE_JUSTIFY)

char *mbstrchr(const char *s, const char *c)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		bool bad_s_mb = FALSE, bad_c_mb = FALSE;
		char symbol[MAXCHARLEN];
		const char *q = s;
		wchar_t ws, wc;

		if (mbtowc(&wc, c, MAXCHARLEN) < 0) {
			mbtowc_reset();
			wc = (unsigned char)*c;
			bad_c_mb = TRUE;
		}

		while (*s != '\0') {
			int sym_len = parse_mbchar(s, symbol, NULL);

			if (mbtowc(&ws, symbol, sym_len) < 0) {
				mbtowc_reset();
				ws = (unsigned char)*s;
				bad_s_mb = TRUE;
			}

			if (bad_s_mb == bad_c_mb && ws == wc)
				break;

			s += sym_len;
			q += sym_len;
		}

		if (*s == '\0')
			q = NULL;

		return (char *)q;
	} else
#endif
		return (char *) strchr(s, *c);
}
#endif 

#ifndef NANO_TINY

char *mbstrpbrk(const char *s, const char *accept)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		for (; *s != '\0'; s += move_mbright(s, 0)) {
			if (mbstrchr(accept, s) != NULL)
				return (char *)s;
		}

		return NULL;
	} else
#endif
		return (char *) strpbrk(s, accept);
}


char *revstrpbrk(const char *head, const char *accept, const char *pointer)
{
	if (*pointer == '\0') {
		if (pointer == head)
			return NULL;
		pointer--;
	}

	while (pointer >= head) {
		if (strchr(accept, *pointer) != NULL)
			return (char *)pointer;
		pointer--;
	}

	return NULL;
}


char *mbrevstrpbrk(const char *head, const char *accept, const char *pointer)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		if (*pointer == '\0') {
			if (pointer == head)
				return NULL;
			pointer = head + move_mbleft(head, pointer - head);
		}

		while (TRUE) {
			if (mbstrchr(accept, pointer) != NULL)
				return (char *)pointer;

			
			if (pointer == head)
				return NULL;

			pointer = head + move_mbleft(head, pointer - head);
		}
	} else
#endif
		return revstrpbrk(head, accept, pointer);
}
#endif 

#if defined(ENABLE_NANORC) && (!defined(NANO_TINY) || defined(ENABLE_JUSTIFY))

bool has_blank_chars(const char *s)
{
	for (; *s != '\0'; s++) {
		if (isblank((unsigned char)*s))
			return TRUE;
	}

	return FALSE;
}


bool has_blank_mbchars(const char *s)
{
#ifdef ENABLE_UTF8
	if (use_utf8) {
		char symbol[MAXCHARLEN];

		for (; *s != '\0'; s += move_mbright(s, 0)) {
			parse_mbchar(s, symbol, NULL);

			if (is_blank_mbchar(symbol))
				return TRUE;
		}

		return FALSE;
	} else
#endif
		return has_blank_chars(s);
}
#endif 

#ifdef ENABLE_UTF8

bool is_valid_unicode(wchar_t wc)
{
	return ((0 <= wc && wc <= 0xD7FF) ||
				(0xE000 <= wc && wc <= 0xFDCF) ||
				(0xFDF0 <= wc && wc <= 0xFFFD) ||
				(0xFFFF < wc && wc <= 0x10FFFF && (wc & 0xFFFF) <= 0xFFFD));
}
#endif

#ifdef ENABLE_NANORC

bool is_valid_mbstring(const char *s)
{
#ifdef ENABLE_UTF8
	if (use_utf8)
		return (mbstowcs(NULL, s, 0) != (size_t)-1);
	else
#endif
		return TRUE;
}
#endif 
