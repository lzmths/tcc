/**************************************************************************
 *   browser.c  --  This file is part of GNU nano.                        *
 *                                                                        *
 *   Copyright (C) 2001-2011, 2013-2017 Free Software Foundation, Inc.    *
 *   Copyright (C) 2015-2016 Benno Schulenberg                            *
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

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#ifdef ENABLE_BROWSER

static char **filelist = NULL;
		/* The list of files to display in the file browser. */
static size_t filelist_len = 0;
		/* The number of files in the list. */
static int width = 0;
		/* The number of files that we can display per screen row. */
static int longest = 0;
		/* The number of columns in the longest filename in the list. */
static size_t selected = 0;
		/* The currently selected filename in the list; zero-based. */

/* Our main file browser function.  path is the tilde-expanded path we
 * start browsing from. */






































 /* ENABLE_BROWSER */