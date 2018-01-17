

#include "proto.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#ifdef ENABLE_BROWSER

static char **filelist = NULL;
		
static size_t filelist_len = 0;
		
static int width = 0;
		
static int longest = 0;
		
static size_t selected = 0;
		


char *do_browser(char *path)
{
	char *retval = NULL;
	int kbinput;
	char *present_name = NULL;
		
	size_t old_selected;
		
	functionptrtype func;
		
	DIR *dir;
		

  read_directory_contents:
		

	path = free_and_assign(path, get_full_path(path));

	if (path != NULL)
		dir = opendir(path);

	if (path == NULL || dir == NULL) {
		statusline(ALERT, _("Cannot open directory: %s"), strerror(errno));
		
		if (filelist == NULL) {
			napms(1200);
			lastmessage = HUSH;
			free(path);
			free(present_name);
			return NULL;
		}
		path = mallocstrcpy(path, present_path);
		present_name = mallocstrcpy(present_name, filelist[selected]);
	}

	assert(path != NULL && path[strlen(path) - 1] == '/');

	if (dir != NULL) {
		
		read_the_list(path, dir);
		closedir(dir);
		dir = NULL;
	}

	
	if (present_name != NULL) {
		browser_select_dirname(present_name);

		free(present_name);
		present_name = NULL;
	
	} else
		selected = 0;

	old_selected = (size_t)-1;

	present_path = mallocstrcpy(present_path, path);

	titlebar(path);

	while (TRUE) {
		lastmessage = HUSH;

		bottombars(MBROWSER);

		
		if (old_selected != selected || ISSET(SHOW_CURSOR))
			browser_refresh();

		old_selected = selected;

		kbinput = get_kbinput(edit, ISSET(SHOW_CURSOR));

#ifdef ENABLE_MOUSE
		if (kbinput == KEY_MOUSE) {
			int mouse_x, mouse_y;

			
			if (get_mouseinput(&mouse_x, &mouse_y, TRUE) == 0 &&
						wmouse_trafo(edit, &mouse_y, &mouse_x, FALSE)) {
				
				selected = selected - selected % (editwinrows * width) +
								(mouse_y * width) + (mouse_x / (longest + 2));

				
				if (mouse_x > width * (longest + 2))
					selected--;

				
				if (selected > filelist_len - 1)
					selected = filelist_len - 1;

				
				if (old_selected == selected)
					unget_kbinput(KEY_ENTER, FALSE);
			}

			continue;
		}
#endif 

		func = parse_browser_input(&kbinput);

		if (func == total_refresh) {
			total_redraw();
#ifndef NANO_TINY
			
			kbinput = KEY_WINCH;
#endif
		} else if (func == do_help_void) {
#ifdef ENABLE_HELP
			do_help_void();
#ifndef NANO_TINY
			
			kbinput = KEY_WINCH;
#endif
#else
			say_there_is_no_help();
#endif
		} else if (func == do_search_forward) {
			do_filesearch();
		} else if (func == do_research) {
			do_fileresearch(TRUE);
#ifndef NANO_TINY
		} else if (func == do_findprevious) {
			do_fileresearch(FALSE);
		} else if (func == do_findnext) {
			do_fileresearch(TRUE);
#endif
		} else if (func == do_left) {
			if (selected > 0)
				selected--;
		} else if (func == do_right) {
			if (selected < filelist_len - 1)
				selected++;
		} else if (func == do_prev_word_void) {
			selected -= (selected % width);
		} else if (func == do_next_word_void) {
			selected += width - 1 - (selected % width);
			if (selected >= filelist_len)
				selected = filelist_len - 1;
		} else if (func == do_up_void) {
			if (selected >= width)
				selected -= width;
		} else if (func == do_down_void) {
			if (selected + width <= filelist_len - 1)
				selected += width;
		} else if (func == do_prev_block) {
			selected = ((selected / (editwinrows * width)) *
								editwinrows * width) + selected % width;
		} else if (func == do_next_block) {
			selected = ((selected / (editwinrows * width)) *
								editwinrows * width) + selected % width +
								editwinrows * width - width;
			if (selected >= filelist_len)
				selected = (filelist_len / width) * width + selected % width;
			if (selected >= filelist_len)
				selected -= width;
		} else if (func == do_page_up) {
			if (selected < width)
				selected = 0;
			else if (selected < editwinrows * width)
				selected = selected % width;
			else
				selected -= editwinrows * width;
		} else if (func == do_page_down) {
			if (selected + width >= filelist_len - 1)
				selected = filelist_len - 1;
			else if (selected + editwinrows * width >= filelist_len)
				selected = (selected + editwinrows * width - filelist_len) %
								width + filelist_len - width;
			else
				selected += editwinrows * width;
		} else if (func == to_first_file) {
			selected = 0;
		} else if (func == to_last_file) {
			selected = filelist_len - 1;
		} else if (func == goto_dir_void) {
			
			int i = do_prompt(TRUE, FALSE, MGOTODIR, NULL, NULL,
						
						browser_refresh, _("Go To Directory"));

			if (i < 0) {
				statusbar(_("Cancelled"));
				continue;
			}

			path = free_and_assign(path, real_dir_from_tilde(answer));

			
			if (*path != '/') {
				path = charealloc(path, strlen(present_path) +
												strlen(answer) + 1);
				sprintf(path, "%s%s", present_path, answer);
			}

#ifdef ENABLE_OPERATINGDIR
			if (outside_of_confinement(path, FALSE)) {
				
				statusline(ALERT, _("Can't go outside of %s"), operating_dir);
				path = mallocstrcpy(path, present_path);
				continue;
			}
#endif
			
			while (strlen(path) > 1 && path[strlen(path) - 1] == '/')
				path[strlen(path) - 1] = '\0';

			
			for (i = 0; i < filelist_len; i++)
				if (strcmp(filelist[i], path) == 0)
					selected = i;

			
			goto read_directory_contents;
		} else if (func == do_enter) {
			struct stat st;

			
			if (strcmp(filelist[selected], "/..") == 0) {
				statusline(ALERT, _("Can't move up a directory"));
				continue;
			}

#ifdef ENABLE_OPERATINGDIR
			
			if (outside_of_confinement(filelist[selected], FALSE)) {
				statusline(ALERT, _("Can't go outside of %s"), operating_dir);
				continue;
			}
#endif
			
			if (stat(filelist[selected], &st) == -1) {
				statusline(ALERT, _("Error reading %s: %s"),
								filelist[selected], strerror(errno));
				continue;
			}

			
			if (!S_ISDIR(st.st_mode)) {
				retval = mallocstrcpy(NULL, filelist[selected]);
				break;
			}

			
			if (strcmp(tail(filelist[selected]), "..") == 0)
				present_name = strip_last_component(filelist[selected]);

			
			path = mallocstrcpy(path, filelist[selected]);
			goto read_directory_contents;
		} else if (func == do_exit) {
			
			break;
#ifndef NANO_TINY
		} else if (kbinput == KEY_WINCH) {
			;
#endif
		} else
			unbound_key(kbinput);

#ifndef NANO_TINY
		
		if (kbinput == KEY_WINCH) {
			
			present_name = mallocstrcpy(NULL, filelist[selected]);
			
			goto read_directory_contents;
		}
#endif
	}

	titlebar(NULL);
	edit_refresh();

	free(path);

	free_chararray(filelist, filelist_len);
	filelist = NULL;
	filelist_len = 0;

	return retval;
}


char *do_browse_from(const char *inpath)
{
	struct stat st;
	char *path;
		

	path = real_dir_from_tilde(inpath);

	
	if (stat(path, &st) == -1 || !S_ISDIR(st.st_mode)) {
		path = free_and_assign(path, strip_last_component(path));

		if (stat(path, &st) == -1 || !S_ISDIR(st.st_mode)) {
			char * currentdir = charalloc(PATH_MAX + 1);

			free(path);
			path = getcwd(currentdir, PATH_MAX + 1);

			if (path == NULL) {
				free(currentdir);
				statusline(MILD, _("The working directory has disappeared"));
				beep();
				napms(1200);
				return NULL;
			}
		}
	}

#ifdef ENABLE_OPERATINGDIR
	
	if (outside_of_confinement(path, FALSE))
		path = mallocstrcpy(path, operating_dir);
#endif

	return do_browser(path);
}


void read_the_list(const char *path, DIR *dir)
{
	const struct dirent *nextdir;
	size_t i = 0, path_len = strlen(path);

	assert(path != NULL && path[strlen(path) - 1] == '/' && dir != NULL);

	longest = 0;

	
	while ((nextdir = readdir(dir)) != NULL) {
		size_t name_len = strlenpt(nextdir->d_name);

		if (name_len > longest)
			longest = name_len;

		i++;
	}

	
	longest += 10;

	
	if (longest < 15)
		longest = 15;
	
	if (longest > COLS)
		longest = COLS;

	rewinddir(dir);

	free_chararray(filelist, filelist_len);

	filelist_len = i;

	filelist = (char **)nmalloc(filelist_len * sizeof(char *));

	i = 0;

	while ((nextdir = readdir(dir)) != NULL && i < filelist_len) {
		
		if (strcmp(nextdir->d_name, ".") == 0)
			continue;

		filelist[i] = charalloc(path_len + strlen(nextdir->d_name) + 1);
		sprintf(filelist[i], "%s%s", path, nextdir->d_name);

		i++;
	}

	
	filelist_len = i;

	assert(filelist != NULL);

	
	qsort(filelist, filelist_len, sizeof(char *), diralphasort);

	
	width = (COLS + 2) / (longest + 2);
}


functionptrtype parse_browser_input(int *kbinput)
{
	if (!meta_key) {
		switch (*kbinput) {
			case ' ':
				return do_page_down;
			case '-':
				return do_page_up;
			case '?':
				return do_help_void;
			case 'E':
			case 'e':
			case 'Q':
			case 'q':
			case 'X':
			case 'x':
				return do_exit;
			case 'G':
			case 'g':
				return goto_dir_void;
			case 'S':
			case 's':
				return do_enter;
			case 'W':
			case 'w':
			case '/':
				return do_search_forward;
			case 'N':
case 'n':
#ifndef NANO_TINY
				return do_findprevious;
#endif
			
				return do_research;
		}
	}
	return func_from_key(kbinput);
}


void browser_refresh(void)
{
	size_t i;
	int row = 0, col = 0;
		
	int the_row = 0, the_column = 0;
		
	char *info;
		

	titlebar(present_path);
	blank_edit();

	wmove(edit, 0, 0);

	i = selected - selected % (editwinrows * width);

	for (; i < filelist_len && row < editwinrows; i++) {
		struct stat st;
		const char *thename = tail(filelist[i]);
				
		size_t namelen = strlenpt(thename);
				
		size_t infolen;
				
		int infomaxlen = 7;
				
		bool dots = (COLS >= 15 && namelen >= longest - infomaxlen);
				
		char *disp = display_string(thename, dots ?
				namelen + infomaxlen + 4 - longest : 0, longest, FALSE);
				

		
		if (i == selected) {
			wattron(edit, interface_color_pair[SELECTED_TEXT]);
			the_row = row;
			the_column = col;
		}

		blank_row(edit, row, col, longest);

		
		if (dots)
			mvwaddstr(edit, row, col, "...");
		mvwaddstr(edit, row, dots ? col + 3 : col, disp);

		free(disp);

		col += longest;

		
		if (lstat(filelist[i], &st) == -1 || S_ISLNK(st.st_mode)) {
			if (stat(filelist[i], &st) == -1 || !S_ISDIR(st.st_mode))
				info = mallocstrcpy(NULL, "--");
			else
				
				info = mallocstrcpy(NULL, _("(dir)"));
		} else if (S_ISDIR(st.st_mode)) {
			if (strcmp(thename, "..") == 0) {
				
				info = mallocstrcpy(NULL, _("(parent dir)"));
				infomaxlen = 12;
			} else
				info = mallocstrcpy(NULL, _("(dir)"));
		} else {
			off_t result = st.st_size;
			char modifier;

			info = charalloc(infomaxlen + 1);

			
			if (st.st_size < (1 << 10))
				modifier = ' ';  
			else if (st.st_size < (1 << 20)) {
				result >>= 10;
				modifier = 'K';  
			} else if (st.st_size < (1 << 30)) {
				result >>= 20;
				modifier = 'M';  
			} else {
				result >>= 30;
				modifier = 'G';  
			}

			
			if (result < (1 << 10))
				sprintf(info, "%4ju %cB", (intmax_t)result, modifier);
			else
				
				info = mallocstrcpy(info, _("(huge)"));
		}

		
		infolen = strlenpt(info);
		if (infolen > infomaxlen) {
			info[actual_x(info, infomaxlen)] = '\0';
			infolen = infomaxlen;
		}

		mvwaddstr(edit, row, col - infolen, info);

		
		if (i == selected)
			wattroff(edit, interface_color_pair[SELECTED_TEXT]);

		free(info);

		
		col += 2;

		
		if (col > COLS - longest) {
			row++;
			col = 0;
		}
	}

	
	if (ISSET(SHOW_CURSOR)) {
		wmove(edit, the_row, the_column);
		curs_set(1);
	}

	wnoutrefresh(edit);
}


void browser_select_dirname(const char *needle)
{
	size_t looking_at = 0;

	for (; looking_at < filelist_len; looking_at++) {
		if (strcmp(filelist[looking_at], needle) == 0) {
			selected = looking_at;
			break;
		}
	}

	
	if (looking_at == filelist_len) {
		--selected;

		
		if (selected >= filelist_len)
			selected = filelist_len - 1;
	}
}


int filesearch_init(void)
{
	int input;
	char *buf;

	if (*last_search != '\0') {
		char *disp = display_string(last_search, 0, COLS / 3, FALSE);

		buf = charalloc(strlen(disp) + 7);
		
		sprintf(buf, " [%s%s]", disp,
				(strlenpt(last_search) > COLS / 3) ? "..." : "");
		free(disp);
	} else
		buf = mallocstrcpy(NULL, "");

	
	input = do_prompt(FALSE, FALSE, MWHEREISFILE, NULL, &search_history,
				browser_refresh, "%s%s", _("Search"), buf);

	
	free(buf);

	
	if (input == -2 && *last_search != '\0')
		return 0;

	
	if (input < 0)
		statusbar(_("Cancelled"));

	return input;
}


void findfile(const char *needle, bool forwards)
{
	size_t looking_at = selected;
		
	const char *thename;
		
	unsigned stash[sizeof(flags) / sizeof(flags[0])];
		

	
	memcpy(stash, flags, sizeof(flags));

	
	UNSET(BACKWARDS_SEARCH);
	UNSET(CASE_SENSITIVE);
	UNSET(USE_REGEXP);

	
	while (TRUE) {
		if (forwards) {
			if (looking_at++ == filelist_len - 1) {
				looking_at = 0;
				statusbar(_("Search Wrapped"));
			}
		} else {
			if (looking_at-- == 0) {
				looking_at = filelist_len - 1;
				statusbar(_("Search Wrapped"));
			}
		}

		
		thename = tail(filelist[looking_at]);

		
		if (strstrwrapper(thename, needle, thename)) {
			if (looking_at == selected)
				statusbar(_("This is the only occurrence"));
			break;
		}

		
		if (looking_at == selected) {
			not_found_msg(needle);
			break;
		}
	}

	
	memcpy(flags, stash, sizeof(flags));

	
	selected = looking_at;
}


void do_filesearch(void)
{
	
	if (filesearch_init() != 0)
		return;

	
	if (*answer == '\0')
		answer = mallocstrcpy(answer, last_search);
	else
		last_search = mallocstrcpy(last_search, answer);

#ifdef ENABLE_HISTORIES
	
	if (*answer != '\0')
		update_history(&search_history, answer);
#endif

	findfile(answer, TRUE);
}


void do_fileresearch(bool forwards)
{
	if (*last_search == '\0')
		statusbar(_("No current search pattern"));
	else
		findfile(last_search, forwards);
}


void to_first_file(void)
{
	selected = 0;
}


void to_last_file(void)
{
	selected = filelist_len - 1;
}


char *strip_last_component(const char *path)
{
	char *copy = mallocstrcpy(NULL, path);
	char *last_slash = strrchr(copy, '/');

	if (last_slash != NULL)
		*last_slash = '\0';

	return copy;
}

#endif 
