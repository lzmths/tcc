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
