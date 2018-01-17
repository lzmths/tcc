/**************************************************************************
 *   chars.c  --  This file is part of GNU nano.                          *
 *                                                                        *
 *   Copyright (C) 2001-2011, 2013-2017 Free Software Foundation, Inc.    *
 *   Copyright (C) 2016-2017 Benno Schulenberg                            *
 *                                                                        *
 *   GNU nano is free software: you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published    *
 *   by the Free Software Foundation, either version 3 of the License,    *
 *   or (at your option) any later version.                               *
 *                                                                        *
 *   GNU nano is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#include "proto.h"

#include <ctype.h>
#include <string.h>

#ifdef ENABLE_UTF8
#include <wchar.h>
#include <wctype.h>

static bool use_utf8 = FALSE;
		/* Whether we've enabled UTF-8 support. */

/* Enable UTF-8 support. */
void utf8_init(void)
{
	use_utf8 = TRUE;
}

/* Is UTF-8 support enabled? */
bool using_utf8(void)
{
	return use_utf8;
}
#endif /* ENABLE_UTF8 */

/* Concatenate two allocated strings, and free the second. */
char *addstrings(char* str1, size_t len1, char* str2, size_t len2)
{
	str1 = charealloc(str1, len1 + len2 + 1);
	str1[len1] = '\0';

	strncat(&str1[len1], str2, len2);
	free(str2);

	return str1;
}

/* Return TRUE if the value of c is in byte range, and FALSE otherwise. */
bool is_byte(int c)
{
	return ((unsigned int)c == (unsigned char)c);
}

void mbtowc_reset(void)
{
	IGNORE_CALL_RESULT(mbtowc(NULL, NULL, 0));
}

/* This function is equivalent to isalpha() for multibyte characters. */
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

/* This function is equivalent to isalnum() for multibyte characters. */
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

/* This function is equivalent to isblank() for multibyte characters. */
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

/* This function is equivalent to iscntrl(), except in that it only
 * handles non-high-bit control characters. */
bool is_ascii_cntrl_char(int c)
{
	return (0 <= c && c < 32);
}

/* This function is equivalent to iscntrl(), except in that it also
 * handles high-bit control characters. */
bool is_cntrl_char(int c)
{
	return ((c & 0x60) == 0 || c == 127);
}

/* This function is equivalent to iscntrl() for multibyte characters,
 * except in that it also handles multibyte control characters with
 * their high bits set. */
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

/* This function is equivalent to ispunct() for multibyte characters. */
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

/* Return TRUE when the given multibyte character c is a word-forming
 * character (that is: alphanumeric, or specified in wordchars, or
 * punctuation when allow_punct is TRUE), and FALSE otherwise. */
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

/* Return the visible representation of control character c. */
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

/* Return the visible representation of multibyte control character c. */
char control_mbrep(const char *c, bool isdata)
{
	/* An embedded newline is an encoded NUL if it is data. */
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

/* Assess how many bytes the given (multibyte) character occupies.  Return -1
 * if the byte sequence is invalid, and return the number of bytes minus 8
 * when it encodes an invalid codepoint.  Also, in the second parameter,
 * return the number of columns that the character occupies. */












































 /* !NANO_TINY || ENABLE_JUSTIFY */

#ifndef NANO_TINY
/* This function is equivalent to strpbrk() for multibyte strings. */
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

/* Locate, in the string that starts at head, the first occurrence of any of
 * the characters in the string accept, starting from pointer and searching
 * backwards. */
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

/* The same as the preceding function but then for multibyte strings. */
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

			/* If we've reached the head of the string, we found nothing. */
			if (pointer == head)
				return NULL;

			pointer = head + move_mbleft(head, pointer - head);
		}
	} else
#endif
		return revstrpbrk(head, accept, pointer);
}
#endif /* !NANO_TINY */

#if defined(ENABLE_NANORC) && (!defined(NANO_TINY) || defined(ENABLE_JUSTIFY))
/* Return TRUE if the string s contains one or more blank characters,
 * and FALSE otherwise. */
bool has_blank_chars(const char *s)
{
	for (; *s != '\0'; s++) {
		if (isblank((unsigned char)*s))
			return TRUE;
	}

	return FALSE;
}

/* Return TRUE if the multibyte string s contains one or more blank
 * multibyte characters, and FALSE otherwise. */
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
#endif /* ENABLE_NANORC && (!NANO_TINY || ENABLE_JUSTIFY) */

#ifdef ENABLE_UTF8
/* Return TRUE if wc is valid Unicode, and FALSE otherwise. */
bool is_valid_unicode(wchar_t wc)
{
	return ((0 <= wc && wc <= 0xD7FF) ||
				(0xE000 <= wc && wc <= 0xFDCF) ||
				(0xFDF0 <= wc && wc <= 0xFFFD) ||
				(0xFFFF < wc && wc <= 0x10FFFF && (wc & 0xFFFF) <= 0xFFFD));
}
#endif

#ifdef ENABLE_NANORC
/* Check if the string s is a valid multibyte string.  Return TRUE if it
 * is, and FALSE otherwise. */
bool is_valid_mbstring(const char *s)
{
#ifdef ENABLE_UTF8
	if (use_utf8)
		return (mbstowcs(NULL, s, 0) != (size_t)-1);
	else
#endif
		return TRUE;
}
#endif /* ENABLE_NANORC */
