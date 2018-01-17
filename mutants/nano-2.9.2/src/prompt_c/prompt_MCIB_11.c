

#include "proto.h"

#include <string.h>

static char *prompt = NULL;
		
static size_t statusbar_x = HIGHEST_POSITIVE;
		


int do_statusbar_input(bool *ran_func, bool *finished)
{
	int input;
		
	static int *kbinput = NULL;
		
	static size_t kbinput_len = 0;
		
	const sc *s;
	bool have_shortcut = FALSE;
	const subnfunc *f;

	*ran_func = FALSE;
	*finished = FALSE;

	
	input = get_kbinput(bottomwin, VISIBLE);

#ifndef NANO_TINY
	if (input == KEY_WINCH)
		return KEY_WINCH;
#endif

#ifdef ENABLE_MOUSE
	
	if (input == KEY_MOUSE) {
		if (do_statusbar_mouse() == 1)
			input = get_kbinput(bottomwin, BLIND);
		else
			return ERR;
	}
#endif

	
	s = get_shortcut(&input);

	
	have_shortcut = (s != NULL);

	
	if (!have_shortcut) {
		if (is_ascii_cntrl_char(input) || meta_key || !is_byte(input)) {
			beep();
			input = ERR;
		}
	}

	
	if (input != ERR && !have_shortcut) {
		
		if (!ISSET(RESTRICTED) || currmenu != MWRITEFILE ||
						openfile->filename[0] == '\0') {
			kbinput_len++;
			kbinput = (int *)nrealloc(kbinput, kbinput_len * sizeof(int));
			kbinput[kbinput_len - 1] = input;
		}
	}

	
	if ((have_shortcut || get_key_buffer_len() == 0) && kbinput != NULL) {
		
		do_statusbar_output(kbinput, kbinput_len, TRUE);

		
		kbinput_len = 0;
		free(kbinput);
		kbinput = NULL;
	}

	if (have_shortcut) {
		if (s->scfunc == do_tab || s->scfunc == do_enter)
			;
		else if (s->scfunc == do_left)
			do_statusbar_left();
		else if (s->scfunc == do_right)
			do_statusbar_right();
#ifndef NANO_TINY
		else if (s->scfunc == do_prev_word_void)
			do_statusbar_prev_word();
		else if (s->scfunc == do_next_word_void)
			do_statusbar_next_word();
#endif
		else if (s->scfunc == do_home)
			do_statusbar_home();
		else if (s->scfunc == do_end)
			do_statusbar_end();
		
		else if (ISSET(RESTRICTED) && currmenu == MWRITEFILE &&
								openfile->filename[0] != '\0' &&
								(s->scfunc == do_verbatim_input ||
								s->scfunc == do_cut_text_void ||
								s->scfunc == do_delete ||
								s->scfunc == do_backspace))
			;
		else if (s->scfunc == do_verbatim_input)
			do_statusbar_verbatim_input();
		else if (s->scfunc == do_cut_text_void)
			do_statusbar_cut_text();
		else if (s->scfunc == do_delete)
			do_statusbar_delete();
		else if (s->scfunc == do_backspace)
			do_statusbar_backspace();
		else if (s->scfunc == do_uncut_text) {
			if (cutbuffer != NULL)
				do_statusbar_uncut_text();
		} else {
			
			f = sctofunc(s);
			if (s->scfunc != NULL) {
				*ran_func = TRUE;
				if (f && (!ISSET(VIEW_MODE) || f->viewok) &&
								f->scfunc != do_gotolinecolumn_void)
					f->scfunc();
			}
			*finished = TRUE;
		}
	}

	return input;
}

#ifdef ENABLE_MOUSE

int do_statusbar_mouse(void)
{
	int mouse_x, mouse_y;
	int retval = get_mouseinput(&mouse_x, &mouse_y, TRUE);

	
	if (retval == 0 && wmouse_trafo(bottomwin, &mouse_y, &mouse_x, FALSE)) {
		size_t start_col;

		start_col = strlenpt(prompt) + 2;

		
		if (mouse_x >= start_col && mouse_y == 0) {
			statusbar_x = actual_x(answer,
						get_statusbar_page_start(start_col, start_col +
						statusbar_xplustabs()) + mouse_x - start_col);
			update_the_statusbar();
		}
	}

	return retval;
}
#endif


void do_statusbar_output(int *the_input, size_t input_len,
		bool filtering)
{
	char *output = charalloc(input_len + 1);
	char onechar[MAXCHARLEN];
	int i, char_len;

	
	for (i = 0; i < input_len; i++)
		output[i] = (char)the_input[i];
	output[i] = '\0';

	i = 0;

	while (i < input_len) {
		
		if (output[i] == '\0')
			output[i] = '\n';

		
		char_len = parse_mbchar(output + i, onechar, NULL);

		i += char_len;

		
		if (filtering && is_ascii_cntrl_char(*(output + i - char_len)))
			continue;

		
		answer = charealloc(answer, strlen(answer) + char_len + 1);
		charmove(answer + statusbar_x + char_len, answer + statusbar_x,
								strlen(answer) - statusbar_x + 1);
		strncpy(answer + statusbar_x, onechar, char_len);

		statusbar_x += char_len;
	}

	free(output);

	update_the_statusbar();
}


void do_statusbar_home(void)
{
	statusbar_x = 0;
	update_the_statusbar();
}


void do_statusbar_end(void)
{
	statusbar_x = strlen(answer);
	update_the_statusbar();
}


void do_statusbar_left(void)
{
	if (statusbar_x > 0) {
		statusbar_x = move_mbleft(answer, statusbar_x);
		update_the_statusbar();
	}
}


void do_statusbar_right(void)
{
	if (answer[statusbar_x] != '\0') {
		statusbar_x = move_mbright(answer, statusbar_x);
		update_the_statusbar();
	}
}


void do_statusbar_backspace(void)
{
	if (statusbar_x > 0) {
		statusbar_x = move_mbleft(answer, statusbar_x);
		do_statusbar_delete();
	}
}


void do_statusbar_delete(void)
{
	if (answer[statusbar_x] != '\0') {
		int char_len = parse_mbchar(answer + statusbar_x, NULL, NULL);

		charmove(answer + statusbar_x, answer + statusbar_x + char_len,
						strlen(answer) - statusbar_x - char_len + 1);

		update_the_statusbar();
	}
}


void do_statusbar_cut_text(void)
{
	if (!ISSET(CUT_FROM_CURSOR))
		statusbar_x = 0;

	answer[statusbar_x] = '\0';

	update_the_statusbar();
}

#ifndef NANO_TINY

void do_statusbar_next_word(void)
{
	bool seen_space = !is_word_mbchar(answer + statusbar_x, FALSE);

	
	while (answer[statusbar_x] != '\0') {
		statusbar_x = move_mbright(answer, statusbar_x);

		
		if (!is_word_mbchar(answer + statusbar_x, FALSE))
			seen_space = TRUE;
		else if (seen_space)
			break;
	}

	update_the_statusbar();
}


void do_statusbar_prev_word(void)
{
	bool seen_a_word = FALSE, step_forward = FALSE;

	
	while (statusbar_x != 0) {
		statusbar_x = move_mbleft(answer, statusbar_x);

		if (is_word_mbchar(answer + statusbar_x, FALSE))
			seen_a_word = TRUE;
		else if (seen_a_word) {
			
			step_forward = TRUE;
			break;
		}
	}

	if (step_forward)
		
		statusbar_x = move_mbright(answer, statusbar_x);

	update_the_statusbar();
}
#endif 


void do_statusbar_verbatim_input(void)
{
	int *kbinput;
	size_t kbinput_len;

	kbinput = get_verbatim_kbinput(bottomwin, &kbinput_len);

	do_statusbar_output(kbinput, kbinput_len, FALSE);
}


size_t statusbar_xplustabs(void)
{
	return strnlenpt(answer, statusbar_x);
}


void do_statusbar_uncut_text(void)
{
	size_t pastelen = strlen(cutbuffer->data);
	char *fusion = charalloc(strlen(answer) + pastelen + 1);

	
	strncpy(fusion, answer, statusbar_x);
	strncpy(fusion + statusbar_x, cutbuffer->data, pastelen);
	strcpy(fusion + statusbar_x + pastelen, answer + statusbar_x);

	free(answer);
	answer = fusion;
	statusbar_x += pastelen;
}


size_t get_statusbar_page_start(size_t base, size_t column)
{
	if (column == base || column < COLS - 1)
		return 0;
	else if (COLS > base + 2)
		return column - base - 1 - (column - base - 1) % (COLS - base - 2);
	else
		return column - 2;
}


void reinit_statusbar_x(void)
{
	statusbar_x = HIGHEST_POSITIVE;
}


void update_the_statusbar(void)
{
	size_t base = strlenpt(prompt) + 2;
	size_t the_page, end_page, column;
	char *expanded;

	the_page = get_statusbar_page_start(base, base + strnlenpt(answer, statusbar_x));
	end_page = get_statusbar_page_start(base, base + strlenpt(answer) - 1);

	
	wattron(bottomwin, interface_color_pair[TITLE_BAR]);
	blank_statusbar();

	mvwaddstr(bottomwin, 0, 0, prompt);
	waddch(bottomwin, ':');
	waddch(bottomwin, (the_page == 0) ? ' ' : '<');

	expanded = display_string(answer, the_page, COLS - base - 1, FALSE);
	waddstr(bottomwin, expanded);
	free(expanded);

	waddch(bottomwin, (the_page >= end_page) ? ' ' : '>');

	wattroff(bottomwin, interface_color_pair[TITLE_BAR]);

	
	wmove(bottomwin, 0, 0);
	wrefresh(bottomwin);

	
	column = base + statusbar_xplustabs();
	wmove(bottomwin, 0, column - get_statusbar_page_start(base, column));
	wnoutrefresh(bottomwin);
}


functionptrtype acquire_an_answer(int *actual, bool allow_tabs,
		bool allow_files, bool *listed, filestruct **history_list,
		void (*refresh_func)(void))
{
	int kbinput = ERR;
	bool ran_func, finished;
	
#ifdef ENABLE_TABCOMP
	bool tabbed = FALSE;
		
#endif
functionptrtype func;
#ifdef ENABLE_HISTORIES
	char *history = NULL;
		
	char *magichistory = NULL;
		
#ifdef ENABLE_TABCOMP
	int last_kbinput = ERR;
		
	size_t complete_len = 0;
		
#endif
#endif 

	if (statusbar_x > strlen(answer))
		statusbar_x = strlen(answer);

	update_the_statusbar();

	while (TRUE) {
		kbinput = do_statusbar_input(&ran_func, &finished);

#ifndef NANO_TINY
		
		if (kbinput == KEY_WINCH) {
			refresh_func();
			*actual = KEY_WINCH;
#ifdef ENABLE_HISTORIES
			free(magichistory);
#endif
			return NULL;
		}
#endif 

		func = func_from_key(&kbinput);

		if (func == do_cancel || func == do_enter)
			break;

#ifdef ENABLE_TABCOMP
		if (func != do_tab)
			tabbed = FALSE;

		if (func == do_tab) {
#ifdef ENABLE_HISTORIES
			if (history_list != NULL) {
				if (last_kbinput != the_code_for(do_tab, TAB_CODE))
					complete_len = strlen(answer);

				if (complete_len > 0) {
					answer = get_history_completion(history_list,
										answer, complete_len);
					statusbar_x = strlen(answer);
				}
			} else
#endif
			if (allow_tabs)
				answer = input_tab(answer, allow_files, &statusbar_x,
										&tabbed, refresh_func, listed);
		} else
#endif 
#ifdef ENABLE_HISTORIES
		if (func == get_history_older_void) {
			if (history_list != NULL) {
				
				if ((*history_list)->next == NULL && *answer != '\0')
					magichistory = mallocstrcpy(magichistory, answer);

				
				if ((history = get_history_older(history_list)) != NULL) {
					answer = mallocstrcpy(answer, history);
					statusbar_x = strlen(answer);
				}

				
				finished = FALSE;
			}
		} else if (func == get_history_newer_void) {
			if (history_list != NULL) {
				
				if ((history = get_history_newer(history_list)) != NULL) {
					answer = mallocstrcpy(answer, history);
					statusbar_x = strlen(answer);
				}

				
				if ((*history_list)->next == NULL &&
						*answer == '\0' && magichistory != NULL) {
					answer = mallocstrcpy(answer, magichistory);
					statusbar_x = strlen(answer);
				}

				
				finished = FALSE;
			}
		} else
#endif 
		if (func == do_help_void) {
			
			finished = FALSE;
		}

		
		if (finished)
			break;

		update_the_statusbar();

#if defined(ENABLE_HISTORIES) && defined(ENABLE_TABCOMP)
		last_kbinput = kbinput;
#endif
	}

#ifdef ENABLE_HISTORIES
	
	if (history_list != NULL) {
		history_reset(*history_list);
		free(magichistory);
	}
#endif

	*actual = kbinput;

	return func;
}


int do_prompt(bool allow_tabs, bool allow_files,
		int menu, const char *curranswer, filestruct **history_list,
		void (*refresh_func)(void), const char *msg, ...)
{
	va_list ap;
	int retval;
	functionptrtype func = NULL;
	bool listed = FALSE;
	
	size_t was_statusbar_x = statusbar_x;
	char *saved_prompt = prompt;

	bottombars(menu);

	answer = mallocstrcpy(answer, curranswer);

#ifndef NANO_TINY
  redo_theprompt:
#endif
	prompt = charalloc((COLS * MAXCHARLEN) + 1);
	va_start(ap, msg);
	vsnprintf(prompt, COLS * MAXCHARLEN, msg, ap);
	va_end(ap);
	
	prompt[actual_x(prompt, (COLS < 5) ? 0 : COLS - 5)] = '\0';

	func = acquire_an_answer(&retval, allow_tabs, allow_files, &listed,
								history_list, refresh_func);
	free(prompt);
	prompt = saved_prompt;

#ifndef NANO_TINY
	if (retval == KEY_WINCH)
		goto redo_theprompt;
#endif

	
	if (func == do_cancel || func == do_enter)
		statusbar_x = was_statusbar_x;

	
	if (func == do_cancel)
		retval = -1;
	else if (func == do_enter)
		retval = (*answer == '\0') ? -2 : 0;

	wipe_statusbar();

#ifdef ENABLE_TABCOMP
	
	if (listed)
		refresh_func();
#endif

	return retval;
}


int do_yesno_prompt(bool all, const char *msg)
{
	int response = -2, width = 16;
	char *message = display_string(msg, 0, COLS, FALSE);

	
	const char *yesstr = _("Yy");
	const char *nostr = _("Nn");
	const char *allstr = _("Aa");

	

	while (response == -2) {
		int kbinput;

		if (!ISSET(NO_HELP)) {
			char shortstr[MAXCHARLEN + 2];
				

			if (COLS < 32)
				width = COLS / 2;

			
			blank_bottombars();

			
			sprintf(shortstr, " %c", yesstr[0]);
			wmove(bottomwin, 1, 0);
			post_one_key(shortstr, _("Yes"), width);

			if (all) {
				shortstr[1] = allstr[0];
				wmove(bottomwin, 1, width);
				post_one_key(shortstr, _("All"), width);
			}

			shortstr[1] = nostr[0];
			wmove(bottomwin, 2, 0);
			post_one_key(shortstr, _("No"), width);

			wmove(bottomwin, 2, width);
			post_one_key("^C", _("Cancel"), width);
		}

		
		wattron(bottomwin, interface_color_pair[TITLE_BAR]);
		blank_statusbar();
		mvwaddnstr(bottomwin, 0, 0, message, actual_x(message, COLS - 1));
		wattroff(bottomwin, interface_color_pair[TITLE_BAR]);
		wnoutrefresh(bottomwin);

		currmenu = MYESNO;

		
		kbinput = get_kbinput(bottomwin, !all);

		
		if (strchr(yesstr, kbinput) != NULL)
			response = 1;
		else if (strchr(nostr, kbinput) != NULL)
			response = 0;
		else if (all && strchr(allstr, kbinput) != NULL)
			response = 2;
		else if (func_from_key(&kbinput) == do_cancel)
			response = -1;
#ifdef ENABLE_MOUSE
		else if (kbinput == KEY_MOUSE) {
			int mouse_x, mouse_y;
			
			if (get_mouseinput(&mouse_x, &mouse_y, FALSE) == 0 &&
						wmouse_trafo(bottomwin, &mouse_y, &mouse_x, FALSE) &&
						mouse_x < (width * 2) && mouse_y > 0) {
				int x = mouse_x / width;
				int y = mouse_y - 1;

				
				response = -2 * x * y + x - y + 1;

				if (response == 2 && !all)
					response = -2;
			}
		}
#endif 
	}

	free(message);

	return response;
}
