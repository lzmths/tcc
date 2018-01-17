

#include "proto.h"

#include <errno.h>
#include <string.h>

#ifdef ENABLE_HISTORIES

#ifndef SEARCH_HISTORY
#define SEARCH_HISTORY "search_history"
#endif

static bool history_changed = FALSE;
#ifndef POSITION_HISTORY
#define POSITION_HISTORY "filepos_history"
#endif


		
static struct stat stat_of_positions_file;
		
static char *poshistname = NULL;
		


void history_init(void)
{
	search_history = make_new_node(NULL);
	search_history->data = mallocstrcpy(NULL, "");
	searchtop = search_history;
	searchbot = search_history;

	replace_history = make_new_node(NULL);
	replace_history->data = mallocstrcpy(NULL, "");
	replacetop = replace_history;
	replacebot = replace_history;

	execute_history = make_new_node(NULL);
	execute_history->data = mallocstrcpy(NULL, "");
	executetop = execute_history;
	executebot = execute_history;
}


void history_reset(const filestruct *list)
{
	if (list == search_history)
		search_history = searchbot;
	else if (list == replace_history)
		replace_history = replacebot;
	else if (list == execute_history)
		execute_history = executebot;
}


filestruct *find_history(const filestruct *start, const filestruct *end,
		const char *text, size_t len)
{
	const filestruct *item;

	for (item = start; item != end->prev && item != NULL; item = item->prev) {
		if (strncmp(item->data, text, len) == 0)
			return (filestruct *)item;
	}

	return NULL;
}


void update_history(filestruct **item, const char *text)
{
	filestruct **htop = NULL, **hbot = NULL, *thesame;

	if (*item == search_history) {
		htop = &searchtop;
		hbot = &searchbot;
	} else if (*item == replace_history) {
		htop = &replacetop;
		hbot = &replacebot;
	} else if (*item == execute_history) {
		htop = &executetop;
		hbot = &executebot;
	}

	
	thesame = find_history(*hbot, *htop, text, HIGHEST_POSITIVE);

	
	if (thesame != NULL) {
		filestruct *after = thesame->next;

		
		if (thesame == *htop)
			*htop = after;

		unlink_node(thesame);
		renumber(after);
	}

	
	if ((*hbot)->lineno == MAX_SEARCH_HISTORY + 1) {
		filestruct *oldest = *htop;

		*htop = (*htop)->next;
		unlink_node(oldest);
		renumber(*htop);
	}

	
	(*hbot)->data = mallocstrcpy((*hbot)->data, text);
	splice_node(*hbot, make_new_node(*hbot));
	*hbot = (*hbot)->next;
	(*hbot)->data = mallocstrcpy(NULL, "");

	
	history_changed = TRUE;

	
	*item = *hbot;
}


char *get_history_older(filestruct **h)
{
	if ((*h)->prev == NULL)
		return NULL;

	*h = (*h)->prev;

	return (*h)->data;
}


char *get_history_newer(filestruct **h)
{
	if ((*h)->next == NULL)
		return NULL;

	*h = (*h)->next;

	return (*h)->data;
}


void get_history_newer_void(void)
{
	;
}
void get_history_older_void(void)
{
	;
}

#ifdef ENABLE_TABCOMP

char *get_history_completion(filestruct **h, char *s, size_t len)
{
	if (len > 0) {
		filestruct *htop = NULL, *hbot = NULL, *p;

		if (*h == search_history) {
			htop = searchtop;
			hbot = searchbot;
		} else if (*h == replace_history) {
			htop = replacetop;
			hbot = replacebot;
		} else if (*h == execute_history) {
			htop = executetop;
			hbot = executebot;
		}

		
		p = find_history((*h)->prev, htop, s, len);

		while (p != NULL && strcmp(p->data, s) == 0)
			p = find_history(p->prev, htop, s, len);

		if (p != NULL) {
			*h = p;
			return mallocstrcpy(s, (*h)->data);
		}

		
		p = find_history(hbot, *h, s, len);

		while (p != NULL && strcmp(p->data, s) == 0)
			p = find_history(p->prev, *h, s, len);

		if (p != NULL) {
			*h = p;
			return mallocstrcpy(s, (*h)->data);
		}
	}

	
	return (char *)s;
}
#endif 

void history_error(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	vfprintf(stderr, _(msg), ap);
	va_end(ap);

	fprintf(stderr, _("\nPress Enter to continue\n"));
	while (getchar() != '\n')
		;
}


bool have_statedir(void)
{
	struct stat dirstat;
	const char *xdgdatadir;

	get_homedir();

	if (homedir != NULL) {
		statedir = concatenate(homedir, "/.nano/");

		if (stat(statedir, &dirstat) == 0 && S_ISDIR(dirstat.st_mode)) {
			poshistname = concatenate(statedir, POSITION_HISTORY);
			return TRUE;
		}
	}

	free(statedir);
	xdgdatadir = getenv("XDG_DATA_HOME");

	if (homedir == NULL && xdgdatadir == NULL)
		return FALSE;

	if (xdgdatadir != NULL)
		statedir = concatenate(xdgdatadir, "/nano/");
	else
		statedir = concatenate(homedir, "/.local/share/nano/");

	if (stat(statedir, &dirstat) == -1) {
		if (xdgdatadir == NULL) {
			char *statepath = concatenate(homedir, "/.local");
			mkdir(statepath, S_IRWXU | S_IRWXG | S_IRWXO);
			free(statepath);
			statepath = concatenate(homedir, "/.local/share");
			mkdir(statepath, S_IRWXU);
			free(statepath);
		}
		if (mkdir(statedir, S_IRWXU) == -1) {
			history_error(N_("Unable to create directory %s: %s\n"
								"It is required for saving/loading "
								"search history or cursor positions.\n"),
								statedir, strerror(errno));
			return FALSE;
		}
	} else if (!S_ISDIR(dirstat.st_mode)) {
		history_error(N_("Path %s is not a directory and needs to be.\n"
								"Nano will be unable to load or save "
								"search history or cursor positions.\n"),
								statedir);
		return FALSE;
	}

	poshistname = concatenate(statedir, POSITION_HISTORY);
	return TRUE;
}


void load_history(void)
{
	char *histname = concatenate(statedir, SEARCH_HISTORY);
	FILE *hisfile = fopen(histname, "rb");

	if (hisfile == NULL) {
		if (errno != ENOENT) {
			
			UNSET(HISTORYLOG);
			history_error(N_("Error reading %s: %s"), histname,
						strerror(errno));
		}
	} else {
		
		filestruct **history = &search_history;
		char *line = NULL;
		size_t buf_len = 0;
		ssize_t read;

		while ((read = getline(&line, &buf_len, hisfile)) > 0) {
			line[--read] = '\0';
			if (read > 0) {
				
				unsunder(line, read);
				update_history(history, line);
			} else if (history == &search_history)
				history = &replace_history;
			else
				history = &execute_history;
		}

		fclose(hisfile);
		free(line);
	}

	
	history_changed = FALSE;

	free(histname);
}


bool write_list(const filestruct *head, FILE *hisfile)
{
	const filestruct *item;

	for (item = head; item != NULL; item = item->next) {
		size_t length = strlen(item->data);

		
		sunder(item->data);

		if (fwrite(item->data, sizeof(char), length, hisfile) < length)
			return FALSE;
		if (putc('\n', hisfile) == EOF)
			return FALSE;
	}

	return TRUE;
}


void save_history(void)
{
	char *histname;
	FILE *hisfile;

	
	if (!history_changed)
		return;

	histname = concatenate(statedir, SEARCH_HISTORY);
	hisfile = fopen(histname, "wb");

	if (hisfile == NULL)
		fprintf(stderr, _("Error writing %s: %s\n"), histname,
						strerror(errno));
	else {
		
		chmod(histname, S_IRUSR | S_IWUSR);

		if (!write_list(searchtop, hisfile) ||
						!write_list(replacetop, hisfile) ||
						!write_list(executetop, hisfile))
			fprintf(stderr, _("Error writing %s: %s\n"), histname,
						strerror(errno));

		fclose(hisfile);
	}

	free(histname);
}


void load_poshistory(void)
{
	FILE *hisfile = fopen(poshistname, "rb");

	if (hisfile == NULL) {
		if (errno != ENOENT) {
			
			UNSET(POS_HISTORY);
			history_error(N_("Error reading %s: %s"), poshistname, strerror(errno));
		}
	} else {
		char *line = NULL, *lineptr, *xptr;
		size_t buf_len = 0;
		ssize_t read, count = 0;
		poshiststruct *record_ptr = NULL, *newrecord;

		
		while ((read = getline(&line, &buf_len, hisfile)) > 5) {
			
			unsunder(line, read);

			
			xptr = revstrstr(line, " ", line + read - 3);
			if (xptr == NULL)
				continue;
			lineptr = revstrstr(line, " ", xptr - 2);
			if (lineptr == NULL)
				continue;

			
			*(xptr++) = '\0';
			*(lineptr++) = '\0';

			
			newrecord = (poshiststruct *)nmalloc(sizeof(poshiststruct));
			newrecord->filename = mallocstrcpy(NULL, line);
			newrecord->lineno = atoi(lineptr);
			newrecord->xno = atoi(xptr);
			newrecord->next = NULL;

			
			if (position_history == NULL)
				position_history = newrecord;
			else
				record_ptr->next = newrecord;

			record_ptr = newrecord;

			
			if (++count > 200) {
				poshiststruct *drop_record = position_history;

				position_history = position_history->next;

				free(drop_record->filename);
				free(drop_record);
			}
		}
		fclose(hisfile);
		free(line);

		stat(poshistname, &stat_of_positions_file);
	}
}


void save_poshistory(void)
{
	poshiststruct *posptr;
	FILE *hisfile = fopen(poshistname, "wb");

	if (hisfile == NULL)
		fprintf(stderr, _("Error writing %s: %s\n"), poshistname, strerror(errno));
	else {
		
		chmod(poshistname, S_IRUSR | S_IWUSR);

		for (posptr = position_history; posptr != NULL; posptr = posptr->next) {
			char *path_and_place;
			size_t length;

			
			path_and_place = charalloc(strlen(posptr->filename) + 44);
			sprintf(path_and_place, "%s %zd %zd\n",
						posptr->filename, posptr->lineno, posptr->xno);
			length = strlen(path_and_place);

			
			sunder(path_and_place);
			
			path_and_place[length - 1] = '\n';

			if (fwrite(path_and_place, sizeof(char), length, hisfile) < length)
				fprintf(stderr, _("Error writing %s: %s\n"),
										poshistname, strerror(errno));
			free(path_and_place);
		}
		fclose(hisfile);

		stat(poshistname, &stat_of_positions_file);
	}
}


void reload_positions_if_needed(void)
{
	struct stat newstat;

	stat(poshistname, &newstat);

	if (newstat.st_mtime != stat_of_positions_file.st_mtime) {
		poshiststruct *ptr, *nextone;

		for (ptr = position_history; ptr != NULL; ptr = nextone) {
			nextone = ptr->next;
			free(ptr->filename);
			free(ptr);
		}
		position_history = NULL;

		load_poshistory();
	}
}


void update_poshistory(char *filename, ssize_t lineno, ssize_t xpos)
{
	poshiststruct *posptr, *theone, *posprev = NULL;
	char *fullpath = get_full_path(filename);

	if (fullpath == NULL || fullpath[strlen(fullpath) - 1] == '/' || inhelp) {
		free(fullpath);
		return;
	}

	reload_positions_if_needed();

	
	for (posptr = position_history; posptr != NULL; posptr = posptr->next) {
		if (!strcmp(posptr->filename, fullpath))
			break;
		posprev = posptr;
	}

	
	if (lineno == 1 && xpos == 1) {
		if (posptr != NULL) {
			if (posprev == NULL)
				position_history = posptr->next;
			else
				posprev->next = posptr->next;
			free(posptr->filename);
			free(posptr);
			save_poshistory();
		}
		free(fullpath);
		return;
	}

	theone = posptr;

	
	if (theone == NULL) {
		theone = (poshiststruct *)nmalloc(sizeof(poshiststruct));
		theone->filename = mallocstrcpy(NULL, fullpath);
		if (position_history == NULL)
			position_history = theone;
		else
			posprev->next = theone;
	} else if (posptr->next != NULL) {
		if (posprev == NULL)
			position_history = posptr->next;
		else
			posprev->next = posptr->next;
		while (posptr->next != NULL)
			posptr = posptr->next;
		posptr->next = theone;
	}

	
	theone->lineno = lineno;
	theone->xno = xpos;
	theone->next = NULL;

	free(fullpath);

	save_poshistory();
}


bool has_old_position(const char *file, ssize_t *line, ssize_t *column)
{
	poshiststruct *posptr = position_history;
	char *fullpath = get_full_path(file);

	if (fullpath == NULL)
		return FALSE;

	reload_positions_if_needed();

	while (posptr != NULL && strcmp(posptr->filename, fullpath) != 0)
		posptr = posptr->next;

	free(fullpath);

	if (posptr == NULL)
		return FALSE;

	*line = posptr->lineno;
	*column = posptr->xno;
	return TRUE;
}
#endif 
