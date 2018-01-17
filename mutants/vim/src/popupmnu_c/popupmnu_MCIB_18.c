


#include "vim.h"

#if defined(FEAT_INS_EXPAND) || defined(PROTO)

static pumitem_T *pum_array = NULL;	
static int pum_size;			
static int pum_selected;		
static int pum_first = 0;		

static int pum_height;			
static int pum_width;			
static int pum_base_width;		
static int pum_kind_width;		
static int pum_scrollbar;		

static int pum_row;			
static int pum_col;			

static int pum_do_redraw = FALSE;	

static int pum_set_selected(int n, int repeat);

#define PUM_DEF_HEIGHT 10
#define PUM_DEF_WIDTH  15


    void
pum_display(
    pumitem_T	*array,
    int		size,
    int		selected)	
{
    int		w;
    int		def_width;
    int		max_width;
    int		kind_width;
    int		extra_width;
    int		i;
    int		row;
    int		context_lines;
    int		col;
    int		above_row;
    int		below_row;
    int		redo_count = 0;
#if defined(FEAT_QUICKFIX)
    win_T	*pvwin;
#endif

    do
    {
	def_width = PUM_DEF_WIDTH;
	max_width = 0;
	kind_width = 0;
	extra_width = 0;
	above_row = 0;
	below_row = cmdline_row;

	
	pum_array = (pumitem_T *)1;
	validate_cursor_col();
	pum_array = NULL;

	row = curwin->w_wrow + W_WINROW(curwin);

#if defined(FEAT_QUICKFIX)
	FOR_ALL_WINDOWS(pvwin)
	    if (pvwin->w_p_pvw)
		break;
	if (pvwin != NULL)
	{
	    if (W_WINROW(pvwin) < W_WINROW(curwin))
		above_row = W_WINROW(pvwin) + pvwin->w_height;
	    else if (W_WINROW(pvwin) > W_WINROW(curwin) + curwin->w_height)
		below_row = W_WINROW(pvwin);
	}
#endif

	
	if (size < PUM_DEF_HEIGHT)
	    pum_height = size;
	else
	    pum_height = PUM_DEF_HEIGHT;
	if (p_ph > 0 && pum_height > p_ph)
	    pum_height = p_ph;

	
	if (row + 2 >= below_row - pum_height
				&& row - above_row > (below_row - above_row) / 2)
	{
	    

	    
	    if (curwin->w_wrow - curwin->w_cline_row >= 2)
		context_lines = 2;
	    else
		context_lines = curwin->w_wrow - curwin->w_cline_row;

	    if (row >= size + context_lines)
	    {
		pum_row = row - size - context_lines;
		pum_height = size;
	    }
	    else
	    {
		pum_row = 0;
		pum_height = row - context_lines;
	    }
	    if (p_ph > 0 && pum_height > p_ph)
	    {
		pum_row += pum_height - p_ph;
		pum_height = p_ph;
	    }
	}
	else
	{
	    

	    
	    if (curwin->w_cline_row
				+ curwin->w_cline_height - curwin->w_wrow >= 3)
		context_lines = 3;
	    else
		context_lines = curwin->w_cline_row
				    + curwin->w_cline_height - curwin->w_wrow;

	    pum_row = row + context_lines;
	    if (size > below_row - pum_row)
		pum_height = below_row - pum_row;
	    else
		pum_height = size;
	    if (p_ph > 0 && pum_height > p_ph)
		pum_height = p_ph;
	}

	
	if (pum_height < 1 || (pum_height == 1 && size > 1))
	    return;

#if defined(FEAT_QUICKFIX)
	
	if (pvwin != NULL && pum_row < above_row && pum_height > above_row)
	{
	    pum_row += above_row;
	    pum_height -= above_row;
	}
#endif

	
	for (i = 0; i < size; ++i)
	{
	    w = vim_strsize(array[i].pum_text);
	    if (max_width < w)
		max_width = w;
	    if (array[i].pum_kind != NULL)
	    {
		w = vim_strsize(array[i].pum_kind) + 1;
		if (kind_width < w)
		    kind_width = w;
	    }
	    if (array[i].pum_extra != NULL)
	    {
		w = vim_strsize(array[i].pum_extra) + 1;
		if (extra_width < w)
		    extra_width = w;
	    }
	}
	pum_base_width = max_width;
	pum_kind_width = kind_width;

	
#ifdef FEAT_RIGHTLEFT
	if (curwin->w_p_rl)
	    col = curwin->w_wincol + curwin->w_width - curwin->w_wcol - 1;
	else
#endif
	    col = curwin->w_wincol + curwin->w_wcol;

	
	if (pum_height < size)
	{
	    pum_scrollbar = 1;
	    ++max_width;
	}
	else
	    pum_scrollbar = 0;

	if (def_width < max_width)
	    def_width = max_width;

	if (((col < Columns - PUM_DEF_WIDTH || col < Columns - max_width
#ifdef FEAT_RIGHTLEFT
		    && !curwin->w_p_rl)
		|| (curwin->w_p_rl && (col > PUM_DEF_WIDTH || col > max_width)
#endif
	   )
))
	{
	    
	    pum_col = col;

#ifdef FEAT_RIGHTLEFT
	    if (curwin->w_p_rl)
		pum_width = pum_col - pum_scrollbar + 1;
	    else
#endif
		pum_width = Columns - pum_col - pum_scrollbar;

	    if (pum_width > max_width + kind_width + extra_width + 1
						     && pum_width > PUM_DEF_WIDTH)
	    {
		pum_width = max_width + kind_width + extra_width + 1;
		if (pum_width < PUM_DEF_WIDTH)
		    pum_width = PUM_DEF_WIDTH;
	    }
	}
	else if (Columns < def_width)
	{
	    
#ifdef FEAT_RIGHTLEFT
	    if (curwin->w_p_rl)
		pum_col = Columns - 1;
	    else
#endif
		pum_col = 0;
	    pum_width = Columns - 1;
	}
	else
	{
	    if (max_width > PUM_DEF_WIDTH)
		max_width = PUM_DEF_WIDTH;	
#ifdef FEAT_RIGHTLEFT
	    if (curwin->w_p_rl)
		pum_col = max_width - 1;
	    else
#endif
		pum_col = Columns - max_width;
	    pum_width = max_width - pum_scrollbar;
	}

	pum_array = array;
	pum_size = size;

	
    } while (pum_set_selected(selected, redo_count) && ++redo_count <= 2);
}


    void
pum_redraw(void)
{
    int		row = pum_row;
    int		col;
    int		attr_norm = highlight_attr[HLF_PNI];
    int		attr_select = highlight_attr[HLF_PSI];
    int		attr_scroll = highlight_attr[HLF_PSB];
    int		attr_thumb = highlight_attr[HLF_PST];
    int		attr;
    int		i;
    int		idx;
    char_u	*s;
    char_u	*p = NULL;
    int		totwidth, width, w;
    int		thumb_pos = 0;
    int		thumb_heigth = 1;
    int		round;
    int		n;

    
    if (pum_first > pum_size - pum_height)
	pum_first = pum_size - pum_height;

    if (pum_scrollbar)
    {
	thumb_heigth = pum_height * pum_height / pum_size;
	if (thumb_heigth == 0)
	    thumb_heigth = 1;
	thumb_pos = (pum_first * (pum_height - thumb_heigth)
			    + (pum_size - pum_height) / 2)
						    / (pum_size - pum_height);
    }

    for (i = 0; i < pum_height; ++i)
    {
	idx = i + pum_first;
	attr = (idx == pum_selected) ? attr_select : attr_norm;

	
#ifdef FEAT_RIGHTLEFT
	if (curwin->w_p_rl)
	{
	    if (pum_col < curwin->w_wincol + curwin->w_width - 1)
		screen_putchar(' ', row, pum_col + 1, attr);
	}
	else
#endif
	    if (pum_col > 0)
		screen_putchar(' ', row, pum_col - 1, attr);

	
	col = pum_col;
	totwidth = 0;
	for (round = 1; round <= 3; ++round)
	{
	    width = 0;
	    s = NULL;
	    switch (round)
	    {
		case 1: p = pum_array[idx].pum_text; break;
		case 2: p = pum_array[idx].pum_kind; break;
		case 3: p = pum_array[idx].pum_extra; break;
	    }
	    if (p != NULL)
		for ( ; ; MB_PTR_ADV(p))
		{
		    if (s == NULL)
			s = p;
		    w = ptr2cells(p);
		    if (*p == NUL || *p == TAB || totwidth + w > pum_width)
		    {
			
			char_u	*st;
			int	saved = *p;

			*p = NUL;
			st = transstr(s);
			*p = saved;
#ifdef FEAT_RIGHTLEFT
			if (curwin->w_p_rl)
			{
			    if (st != NULL)
			    {
				char_u	*rt = reverse_text(st);

				if (rt != NULL)
				{
				    char_u	*rt_start = rt;
				    int		size;

				    size = vim_strsize(rt);
				    if (size > pum_width)
				    {
					do
					{
					    size -= has_mbyte
						    ? (*mb_ptr2cells)(rt) : 1;
					    MB_PTR_ADV(rt);
					} while (size > pum_width);

					if (size < pum_width)
					{
					    
					    *(--rt) = '<';
					    size++;
					}
				    }
				    screen_puts_len(rt, (int)STRLEN(rt),
						   row, col - size + 1, attr);
				    vim_free(rt_start);
				}
				vim_free(st);
			    }
			    col -= width;
			}
			else
#endif
			{
			    if (st != NULL)
			    {
				screen_puts_len(st, (int)STRLEN(st), row, col,
									attr);
				vim_free(st);
			    }
			    col += width;
			}

			if (*p != TAB)
			    break;

			
#ifdef FEAT_RIGHTLEFT
			if (curwin->w_p_rl)
			{
			    screen_puts_len((char_u *)"  ", 2, row, col - 1,
									attr);
			    col -= 2;
			}
			else
#endif
			{
			    screen_puts_len((char_u *)"  ", 2, row, col, attr);
			    col += 2;
			}
			totwidth += 2;
			s = NULL;	    
			width = 0;
		    }
		    else
			width += w;
		}

	    if (round > 1)
		n = pum_kind_width + 1;
	    else
		n = 1;

	    
	    if (round == 3
		    || (round == 2 && pum_array[idx].pum_extra == NULL)
		    || (round == 1 && pum_array[idx].pum_kind == NULL
					  && pum_array[idx].pum_extra == NULL)
		    || pum_base_width + n >= pum_width)
		break;
#ifdef FEAT_RIGHTLEFT
	    if (curwin->w_p_rl)
	    {
		screen_fill(row, row + 1, pum_col - pum_base_width - n + 1,
						    col + 1, ' ', ' ', attr);
		col = pum_col - pum_base_width - n + 1;
	    }
	    else
#endif
	    {
		screen_fill(row, row + 1, col, pum_col + pum_base_width + n,
							      ' ', ' ', attr);
		col = pum_col + pum_base_width + n;
	    }
	    totwidth = pum_base_width + n;
	}

#ifdef FEAT_RIGHTLEFT
	if (curwin->w_p_rl)
	    screen_fill(row, row + 1, pum_col - pum_width + 1, col + 1, ' ',
								    ' ', attr);
	else
#endif
	    screen_fill(row, row + 1, col, pum_col + pum_width, ' ', ' ',
									attr);
	if (pum_scrollbar > 0)
	{
#ifdef FEAT_RIGHTLEFT
	    if (curwin->w_p_rl)
		screen_putchar(' ', row, pum_col - pum_width,
			i >= thumb_pos && i < thumb_pos + thumb_heigth
						  ? attr_thumb : attr_scroll);
	    else
#endif
		screen_putchar(' ', row, pum_col + pum_width,
			i >= thumb_pos && i < thumb_pos + thumb_heigth
						  ? attr_thumb : attr_scroll);
	}

	++row;
    }
}


    static int
pum_set_selected(int n, int repeat)
{
    int	    resized = FALSE;
    int	    context = pum_height / 2;

    pum_selected = n;

    if (pum_selected >= 0 && pum_selected < pum_size)
    {
	if (pum_first > pum_selected - 4)
	{
	    
	    if (pum_first > pum_selected - 2)
	    {
		pum_first -= pum_height - 2;
		if (pum_first < 0)
		    pum_first = 0;
		else if (pum_first > pum_selected)
		    pum_first = pum_selected;
	    }
	    else
		pum_first = pum_selected;
	}
	else if (pum_first < pum_selected - pum_height + 5)
	{
	    
	    if (pum_first < pum_selected - pum_height + 1 + 2)
	    {
		pum_first += pum_height - 2;
		if (pum_first < pum_selected - pum_height + 1)
		    pum_first = pum_selected - pum_height + 1;
	    }
	    else
		pum_first = pum_selected - pum_height + 1;
	}

	
	if (context > 3)
	    context = 3;
	if (pum_height > 2)
	{
	    if (pum_first > pum_selected - context)
	    {
		
		pum_first = pum_selected - context;
		if (pum_first < 0)
		    pum_first = 0;
	    }
	    else if (pum_first < pum_selected + context - pum_height + 1)
	    {
		
		pum_first = pum_selected + context - pum_height + 1;
	    }
	}

#if defined(FEAT_QUICKFIX)
	
	if (pum_array[pum_selected].pum_info != NULL
		&& Rows > 10
		&& repeat <= 1
		&& vim_strchr(p_cot, 'p') != NULL)
	{
	    win_T	*curwin_save = curwin;
	    int		res = OK;

	    
	    g_do_tagpreview = 3;
	    if (p_pvh > 0 && p_pvh < g_do_tagpreview)
		g_do_tagpreview = p_pvh;
	    ++RedrawingDisabled;
	    
	    ++no_u_sync;
	    resized = prepare_tagpreview(FALSE);
	    --no_u_sync;
	    --RedrawingDisabled;
	    g_do_tagpreview = 0;

	    if (curwin->w_p_pvw)
	    {
		if (!resized
			&& curbuf->b_nwindows == 1
			&& curbuf->b_fname == NULL
			&& curbuf->b_p_bt[0] == 'n' && curbuf->b_p_bt[2] == 'f'
			&& curbuf->b_p_bh[0] == 'w')
		{
		    
		    while (!BUFEMPTY())
			ml_delete((linenr_T)1, FALSE);
		}
		else
		{
		    
		    ++no_u_sync;
		    res = do_ecmd(0, NULL, NULL, NULL, ECMD_ONE, 0, NULL);
		    --no_u_sync;
		    if (res == OK)
		    {
			
			set_option_value((char_u *)"swf", 0L, NULL, OPT_LOCAL);
			set_option_value((char_u *)"bt", 0L,
					       (char_u *)"nofile", OPT_LOCAL);
			set_option_value((char_u *)"bh", 0L,
						 (char_u *)"wipe", OPT_LOCAL);
			set_option_value((char_u *)"diff", 0L,
							     NULL, OPT_LOCAL);
		    }
		}
		if (res == OK)
		{
		    char_u	*p, *e;
		    linenr_T	lnum = 0;

		    for (p = pum_array[pum_selected].pum_info; *p != NUL; )
		    {
			e = vim_strchr(p, '\n');
			if (e == NULL)
			{
			    ml_append(lnum++, p, 0, FALSE);
			    break;
			}
			else
			{
			    *e = NUL;
			    ml_append(lnum++, p, (int)(e - p + 1), FALSE);
			    *e = '\n';
			    p = e + 1;
			}
		    }

		    
		    if (repeat == 0)
		    {
			if (lnum > p_pvh)
			    lnum = p_pvh;
			if (curwin->w_height < lnum)
			{
			    win_setheight((int)lnum);
			    resized = TRUE;
			}
		    }

		    curbuf->b_changed = 0;
		    curbuf->b_p_ma = FALSE;
		    curwin->w_cursor.lnum = 1;
		    curwin->w_cursor.col = 0;

		    if (curwin != curwin_save && win_valid(curwin_save))
		    {
			
			if (ins_compl_active() && !resized)
			    curwin->w_redr_status = FALSE;

			
			validate_cursor();
			redraw_later(SOME_VALID);

			
			if (resized)
			{
			    ++no_u_sync;
			    win_enter(curwin_save, TRUE);
			    --no_u_sync;
			    update_topline();
			}

			
			pum_do_redraw = TRUE;
			update_screen(0);
			pum_do_redraw = FALSE;

			if (!resized && win_valid(curwin_save))
			{
			    ++no_u_sync;
			    win_enter(curwin_save, TRUE);
			    --no_u_sync;
			}

			
			pum_do_redraw = TRUE;
			update_screen(0);
			pum_do_redraw = FALSE;
		    }
		}
	    }
	}
#endif
    }

    if (!resized)
	pum_redraw();

    return resized;
}


    void
pum_undisplay(void)
{
    pum_array = NULL;
    redraw_all_later(SOME_VALID);
    redraw_tabline = TRUE;
    status_redraw_all();
}


    void
pum_clear(void)
{
    pum_first = 0;
}


    int
pum_visible(void)
{
    return !pum_do_redraw && pum_array != NULL;
}


    int
pum_get_height(void)
{
    return pum_height;
}

#endif
