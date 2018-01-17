

#include "proto.h"

#include <string.h>


void to_first_line(void)
{
	openfile->current = openfile->fileage;
	openfile->current_x = 0;
	openfile->placewewant = 0;

	refresh_needed = TRUE;
}


void to_last_line(void)
{
	openfile->current = openfile->filebot;
	openfile->current_x = strlen(openfile->filebot->data);
	openfile->placewewant = xplustabs();

	
	openfile->current_y = editwinrows - 1;

	refresh_needed = TRUE;
	focusing = FALSE;
}


void get_edge_and_target(size_t *leftedge, size_t *target_column)
{
#ifndef NANO_TINY
	if (ISSET(SOFTWRAP)) {
		size_t shim = editwincols * (1 + (tabsize / editwincols));

		*leftedge = leftedge_for(xplustabs(), openfile->current);
		*target_column = (openfile->placewewant + shim - *leftedge) % editwincols;
	} else
#endif
	{
		*leftedge = 0;
		*target_column = openfile->placewewant;
	}
}


size_t proper_x(filestruct *line, size_t *leftedge, bool forward,
				size_t column, bool *shifted)
{
	

#ifndef NANO_TINY
	if (ISSET(SOFTWRAP) && line->data[index] == '\t' &&
				((forward && strnlenpt(line->data, index) < *leftedge) ||
				(!forward && column / tabsize == (*leftedge - 1) / tabsize &&
				column / tabsize < (*leftedge + editwincols - 1) / tabsize))) {
		index++;

		if (shifted != NULL)
			*shifted = TRUE;
	}

	if (ISSET(SOFTWRAP))
		*leftedge = leftedge_for(strnlenpt(line->data, index), line);
#endif

	size_t index = actual_x(line->data, column);
return index;
}


void set_proper_index_and_pww(size_t *leftedge, size_t target, bool forward)
{
	bool shifted = FALSE;
	size_t was_edge = *leftedge;

	openfile->current_x = proper_x(openfile->current, leftedge, forward,
						actual_last_column(*leftedge, target), &shifted);

	
	if (shifted || *leftedge < was_edge)
		openfile->current_x = proper_x(openfile->current, leftedge, forward,
						actual_last_column(*leftedge, target), &shifted);

	openfile->placewewant = *leftedge + target;
}


void do_page_up(void)
{
	int mustmove = (editwinrows < 3) ? 1 : editwinrows - 2;
	size_t leftedge, target_column;

	
	if (!ISSET(SMOOTH_SCROLL)) {
		openfile->current = openfile->edittop;
		leftedge = openfile->firstcolumn;
		openfile->current_y = 0;
		target_column = 0;
	} else
		get_edge_and_target(&leftedge, &target_column);

	
	if (go_back_chunks(mustmove, &openfile->current, &leftedge) > 0) {
		to_first_line();
		return;
	}

	set_proper_index_and_pww(&leftedge, target_column, FALSE);

	
	adjust_viewport(STATIONARY);
	refresh_needed = TRUE;
}


void do_page_down(void)
{
	int mustmove = (editwinrows < 3) ? 1 : editwinrows - 2;
	size_t leftedge, target_column;

	
	if (!ISSET(SMOOTH_SCROLL)) {
		openfile->current = openfile->edittop;
		leftedge = openfile->firstcolumn;
		openfile->current_y = 0;
		target_column = 0;
	} else
		get_edge_and_target(&leftedge, &target_column);

	
	if (go_forward_chunks(mustmove, &openfile->current, &leftedge) > 0) {
		to_last_line();
		return;
	}

	set_proper_index_and_pww(&leftedge, target_column, TRUE);

	
	adjust_viewport(STATIONARY);
	refresh_needed = TRUE;
}

#ifdef ENABLE_JUSTIFY

void do_para_begin(bool update_screen)
{
	filestruct *was_current = openfile->current;

	if (openfile->current != openfile->fileage)
		openfile->current = openfile->current->prev;

	while (!begpar(openfile->current))
		openfile->current = openfile->current->prev;

	openfile->current_x = 0;

	if (update_screen)
		edit_redraw(was_current, CENTERING);
}


void do_para_end(bool update_screen)
{
	filestruct *was_current = openfile->current;

	while (openfile->current != openfile->filebot &&
				!inpar(openfile->current))
		openfile->current = openfile->current->next;

	while (openfile->current != openfile->filebot &&
				inpar(openfile->current->next) &&
				!begpar(openfile->current->next)) {
		openfile->current = openfile->current->next;
	}

	if (openfile->current != openfile->filebot) {
		openfile->current = openfile->current->next;
		openfile->current_x = 0;
	} else
		openfile->current_x = strlen(openfile->current->data);

	if (update_screen)
		edit_redraw(was_current, CENTERING);
}


void do_para_begin_void(void)
{
	do_para_begin(TRUE);
}


void do_para_end_void(void)
{
	do_para_end(TRUE);
}
#endif 


void do_prev_block(void)
{
	filestruct *was_current = openfile->current;
	bool is_text = FALSE, seen_text = FALSE;

	
	while (openfile->current->prev != NULL && (!seen_text || is_text)) {
		openfile->current = openfile->current->prev;
		is_text = !white_string(openfile->current->data);
		seen_text = seen_text || is_text;
	}

	
	if (openfile->current->next != NULL &&
				white_string(openfile->current->data))
		openfile->current = openfile->current->next;

	openfile->current_x = 0;
	edit_redraw(was_current, CENTERING);
}


void do_next_block(void)
{
	filestruct *was_current = openfile->current;
	bool is_white = white_string(openfile->current->data);
	bool seen_white = is_white;

	
	while (openfile->current->next != NULL && (!seen_white || is_white)) {
		openfile->current = openfile->current->next;
		is_white = white_string(openfile->current->data);
		seen_white = seen_white || is_white;
	}

	openfile->current_x = 0;
	edit_redraw(was_current, CENTERING);
}


void do_prev_word(bool allow_punct, bool update_screen)
{
	filestruct *was_current = openfile->current;
	bool seen_a_word = FALSE, step_forward = FALSE;

	
	while (TRUE) {
		
		if (openfile->current_x == 0) {
			if (openfile->current->prev == NULL)
				break;
			openfile->current = openfile->current->prev;
			openfile->current_x = strlen(openfile->current->data);
		}

		
		openfile->current_x = move_mbleft(openfile->current->data,
												openfile->current_x);

		if (is_word_mbchar(openfile->current->data + openfile->current_x,
								allow_punct)) {
			seen_a_word = TRUE;
			
			if (openfile->current_x == 0)
				break;
		} else if (seen_a_word) {
			
			step_forward = TRUE;
			break;
		}
	}

	if (step_forward)
		
		openfile->current_x = move_mbright(openfile->current->data,
												openfile->current_x);

	if (update_screen)
		edit_redraw(was_current, FLOWING);
}


bool do_next_word(bool allow_punct, bool update_screen)
{
	filestruct *was_current = openfile->current;
	bool started_on_word = is_word_mbchar(openfile->current->data +
								openfile->current_x, allow_punct);
	bool seen_space = !started_on_word;

	
	while (TRUE) {
		
		if (openfile->current->data[openfile->current_x] == '\0') {
			
			if (openfile->current->next == NULL)
				break;
			openfile->current = openfile->current->next;
			openfile->current_x = 0;
			seen_space = TRUE;
		} else {
			
			openfile->current_x = move_mbright(openfile->current->data,
												openfile->current_x);
		}

		
		if (!is_word_mbchar(openfile->current->data + openfile->current_x,
								allow_punct))
			seen_space = TRUE;
		else if (seen_space)
			break;
	}

	if (update_screen)
		edit_redraw(was_current, FLOWING);

	
	return started_on_word;
}


void do_prev_word_void(void)
{
	do_prev_word(ISSET(WORD_BOUNDS), TRUE);
}


void do_next_word_void(void)
{
	do_next_word(ISSET(WORD_BOUNDS), TRUE);
}


void do_home(void)
{
	filestruct *was_current = openfile->current;
	size_t was_column = xplustabs();
	bool moved_off_chunk = TRUE;
#ifndef NANO_TINY
	bool moved = FALSE;
	size_t leftedge = 0, leftedge_x = 0;

	if (ISSET(SOFTWRAP)) {
		leftedge = leftedge_for(was_column, openfile->current);
		leftedge_x = proper_x(openfile->current, &leftedge, FALSE, leftedge,
								NULL);
	}

	if (ISSET(SMART_HOME)) {
		size_t indent_x = indent_length(openfile->current->data);

		if (openfile->current->data[indent_x] != '\0') {
			
			if (openfile->current_x == indent_x) {
				openfile->current_x = 0;
				moved = TRUE;
			} else if (!ISSET(SOFTWRAP) || leftedge_x <= indent_x) {
				openfile->current_x = indent_x;
				moved = TRUE;
			}
		}
	}

	if (!moved && ISSET(SOFTWRAP)) {
		
		if (openfile->current_x == leftedge_x)
			openfile->current_x = 0;
		else {
			openfile->current_x = leftedge_x;
			openfile->placewewant = leftedge;
			moved_off_chunk = FALSE;
		}
	} else if (!moved)
#endif
		openfile->current_x = 0;

	if (moved_off_chunk)
		openfile->placewewant = xplustabs();

	
	if (ISSET(SOFTWRAP) && moved_off_chunk)
		edit_redraw(was_current, FLOWING);
	else if (line_needs_update(was_column, openfile->placewewant))
		update_line(openfile->current, openfile->current_x);
}


void do_end(void)
{
	filestruct *was_current = openfile->current;
	size_t was_column = xplustabs();
	size_t line_len = strlen(openfile->current->data);
	bool moved_off_chunk = TRUE;

#ifndef NANO_TINY
	if (ISSET(SOFTWRAP)) {
		bool last_chunk = FALSE;
		size_t leftedge = leftedge_for(was_column, openfile->current);
		size_t rightedge = get_softwrap_breakpoint(openfile->current->data,
												leftedge, &last_chunk);
		size_t rightedge_x;

		
		if (!last_chunk)
			rightedge--;

		rightedge_x = actual_x(openfile->current->data, rightedge);

		
		if (openfile->current_x == rightedge_x)
			openfile->current_x = line_len;
		else {
			openfile->current_x = rightedge_x;
			openfile->placewewant = rightedge;
			moved_off_chunk = FALSE;
		}
	} else
#endif
		openfile->current_x = line_len;

	if (moved_off_chunk)
		openfile->placewewant = xplustabs();

	
	if (ISSET(SOFTWRAP) && moved_off_chunk)
		edit_redraw(was_current, FLOWING);
	else if (line_needs_update(was_column, openfile->placewewant))
		update_line(openfile->current, openfile->current_x);
}


void do_up(bool scroll_only)
{
	filestruct *was_current = openfile->current;
	size_t leftedge, target_column;

	
	if (scroll_only && openfile->edittop == openfile->fileage &&
						openfile->firstcolumn == 0)
		return;

	get_edge_and_target(&leftedge, &target_column);

	
	if (go_back_chunks(1, &openfile->current, &leftedge) > 0)
		return;

	set_proper_index_and_pww(&leftedge, target_column, FALSE);

	if (scroll_only)
		edit_scroll(BACKWARD, 1);

	edit_redraw(was_current, FLOWING);

	
	openfile->placewewant = leftedge + target_column;
}


void do_down(bool scroll_only)
{
	filestruct *was_current = openfile->current;
	size_t leftedge, target_column;

	get_edge_and_target(&leftedge, &target_column);

	
	if (go_forward_chunks(1, &openfile->current, &leftedge) > 0)
		return;

	set_proper_index_and_pww(&leftedge, target_column, TRUE);

	if (scroll_only)
		edit_scroll(FORWARD, 1);

	edit_redraw(was_current, FLOWING);

	
	openfile->placewewant = leftedge + target_column;
}


void do_up_void(void)
{
	do_up(FALSE);
}


void do_down_void(void)
{
	do_down(FALSE);
}

#ifndef NANO_TINY

void do_scroll_up(void)
{
	do_up(TRUE);
}


void do_scroll_down(void)
{
	do_down(TRUE);
}
#endif


void do_left(void)
{
	filestruct *was_current = openfile->current;

	if (openfile->current_x > 0)
		openfile->current_x = move_mbleft(openfile->current->data,
												openfile->current_x);
	else if (openfile->current != openfile->fileage) {
		openfile->current = openfile->current->prev;
		openfile->current_x = strlen(openfile->current->data);
	}

	edit_redraw(was_current, FLOWING);
}


void do_right(void)
{
	filestruct *was_current = openfile->current;

	if (openfile->current->data[openfile->current_x] != '\0')
		openfile->current_x = move_mbright(openfile->current->data,
												openfile->current_x);
	else if (openfile->current != openfile->filebot) {
		openfile->current = openfile->current->next;
		openfile->current_x = 0;
	}

	edit_redraw(was_current, FLOWING);
}
