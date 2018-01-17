/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * json.c: Encoding and decoding JSON.
 *
 * Follows this standard: https://tools.ietf.org/html/rfc7159.html
 */
#define USING_FLOAT_STUFF

#include "vim.h"

#if defined(FEAT_EVAL) || defined(PROTO)

static int json_encode_item(garray_T *gap, typval_T *val, int copyID, int options);
static int json_decode_item(js_read_T *reader, typval_T *res, int options);

/*
 * Encode "val" into a JSON format string.
 * The result is added to "gap"
 * Returns FAIL on failure and makes gap->ga_data empty.
 */
    static int
json_encode_gap(garray_T *gap, typval_T *val, int options)
{
    if (json_encode_item(gap, val, get_copyID(), options) == FAIL)
    {
	ga_clear(gap);
	gap->ga_data = vim_strsave((char_u *)"");
	return FAIL;
    }
    return OK;
}

/*
 * Encode "val" into a JSON format string.
 * The result is in allocated memory.
 * The result is empty when encoding fails.
 * "options" can contain JSON_JS, JSON_NO_NONE and JSON_NL.
 */
    char_u *
json_encode(typval_T *val, int options)
{
    garray_T ga;

    /* Store bytes in the growarray. */
    ga_init2(&ga, 1, 4000);
    json_encode_gap(&ga, val, options);
    return ga.ga_data;
}

/*
 * Encode ["nr", "val"] into a JSON format string in allocated memory.
 * "options" can contain JSON_JS, JSON_NO_NONE and JSON_NL.
 * Returns NULL when out of memory.
 */
    char_u *
json_encode_nr_expr(int nr, typval_T *val, int options)
{
    typval_T	listtv;
    typval_T	nrtv;
    garray_T	ga;

    nrtv.v_type = VAR_NUMBER;
    nrtv.vval.v_number = nr;
    if (rettv_list_alloc(&listtv) == FAIL)
	return NULL;
    if (list_append_tv(listtv.vval.v_list, &nrtv) == FAIL
	    || list_append_tv(listtv.vval.v_list, val) == FAIL)
    {
	list_unref(listtv.vval.v_list);
	return NULL;
    }

    ga_init2(&ga, 1, 4000);
    if (json_encode_gap(&ga, &listtv, options) == OK && (options & JSON_NL))
	ga_append(&ga, '\n');
    list_unref(listtv.vval.v_list);
    return ga.ga_data;
}

    static void
write_string(garray_T *gap, char_u *str)
{
    char_u	*res = str;
    char_u	numbuf[NUMBUFLEN];

    if (res == NULL)
	ga_concat(gap, (char_u *)"null");
    else
    {
#if defined(FEAT_MBYTE) && defined(USE_ICONV)
	vimconv_T   conv;
	char_u	    *converted = NULL;

	if (!enc_utf8)
	{
	    /* Convert the text from 'encoding' to utf-8, the JSON string is
	     * always utf-8. */
	    conv.vc_type = CONV_NONE;
	    convert_setup(&conv, p_enc, (char_u*)"utf-8");
	    if (conv.vc_type != CONV_NONE)
		converted = res = string_convert(&conv, res, NULL);
	    convert_setup(&conv, NULL, NULL);
	}
#endif
	ga_append(gap, '"');
	while (*res != NUL)
	{
	    int c;
#ifdef FEAT_MBYTE
	    /* always use utf-8 encoding, ignore 'encoding' */
	    c = utf_ptr2char(res);
#else
	    c = *res;
#endif

	    switch (c)
	    {
		case 0x08:
		    ga_append(gap, '\\'); ga_append(gap, 'b'); break;
		case 0x09:
		    ga_append(gap, '\\'); ga_append(gap, 't'); break;
		case 0x0a:
		    ga_append(gap, '\\'); ga_append(gap, 'n'); break;
		case 0x0c:
		    ga_append(gap, '\\'); ga_append(gap, 'f'); break;
		case 0x0d:
		    ga_append(gap, '\\'); ga_append(gap, 'r'); break;
		case 0x22: /* " */
		case 0x5c: /* \ */
		    ga_append(gap, '\\');
		    ga_append(gap, c);
		    break;
		default:
		    if (c >= 0x20)
		    {
#ifdef FEAT_MBYTE
			numbuf[utf_char2bytes(c, numbuf)] = NUL;
#else
			numbuf[0] = c;
			numbuf[1] = NUL;
#endif
			ga_concat(gap, numbuf);
		    }
		    else
		    {
			vim_snprintf((char *)numbuf, NUMBUFLEN,
							 "\\u%04lx", (long)c);
			ga_concat(gap, numbuf);
		    }
	    }
#ifdef FEAT_MBYTE
	    res += utf_ptr2len(res);
#else
	    ++res;
#endif
	}
	ga_append(gap, '"');
#if defined(FEAT_MBYTE) && defined(USE_ICONV)
	vim_free(converted);
#endif
    }
}

/*
 * Return TRUE if "key" can be used without quotes.
 * That is when it starts with a letter and only contains letters, digits and
 * underscore.
 */
    static int
is_simple_key(char_u *key)
{
    char_u *p;

    if (!ASCII_ISALPHA(*key))
	return FALSE;
    for (p = key + 1; *p != NUL; ++p)
	if (!ASCII_ISALPHA(*p) && *p != '_' && !vim_isdigit(*p))
	    return FALSE;
    return TRUE;
}

/*
 * Encode "val" into "gap".
 * Return FAIL or OK.
 */
    static int
json_encode_item(garray_T *gap, typval_T *val, int copyID, int options)
{
    char_u	numbuf[NUMBUFLEN];
    char_u	*res;
    list_T	*l;
    dict_T	*d;

    switch (val->v_type)
    {
	case VAR_SPECIAL:
	    switch (val->vval.v_number)
	    {
		case VVAL_FALSE: ga_concat(gap, (char_u *)"false"); break;
		case VVAL_TRUE: ga_concat(gap, (char_u *)"true"); break;
		case VVAL_NONE: if ((options & JSON_JS) != 0
					     && (options & JSON_NO_NONE) == 0)
				    /* empty item */
				    break;
				/* FALLTHROUGH */
		case VVAL_NULL: ga_concat(gap, (char_u *)"null"); break;
	    }
	    break;

	case VAR_NUMBER:
	    vim_snprintf((char *)numbuf, NUMBUFLEN, "%lld",
						    val->vval.v_number);
	    ga_concat(gap, numbuf);
	    break;

	case VAR_STRING:
	    res = val->vval.v_string;
	    write_string(gap, res);
	    break;

	case VAR_FUNC:
	case VAR_PARTIAL:
	case VAR_JOB:
	case VAR_CHANNEL:
	    /* no JSON equivalent TODO: better error */
	    EMSG(_(e_invarg));
	    return FAIL;

	case VAR_LIST:
	    l = val->vval.v_list;
	    if (l == NULL)
		ga_concat(gap, (char_u *)"null");
	    else
	    {
		if (l->lv_copyID == copyID)
		    ga_concat(gap, (char_u *)"[]");
		else
		{
		    listitem_T	*li;

		    l->lv_copyID = copyID;
		    ga_append(gap, '[');
		    for (li = l->lv_first; li != NULL && !got_int; )
		    {
			if (json_encode_item(gap, &li->li_tv, copyID,
						   options & JSON_JS) == FAIL)
			    return FAIL;
			if ((options & JSON_JS)
				&& li->li_next == NULL
				&& li->li_tv.v_type == VAR_SPECIAL
				&& li->li_tv.vval.v_number == VVAL_NONE)
			    /* add an extra comma if the last item is v:none */
			    ga_append(gap, ',');
			li = li->li_next;
			if (li != NULL)
			    ga_append(gap, ',');
		    }
		    ga_append(gap, ']');
		    l->lv_copyID = 0;
		}
	    }
	    break;

	case VAR_DICT:
	    d = val->vval.v_dict;
	    if (d == NULL)
		ga_concat(gap, (char_u *)"null");
	    else
	    {
		if (d->dv_copyID == copyID)
		    ga_concat(gap, (char_u *)"{}");
		else
		{
		    int		first = TRUE;
		    int		todo = (int)d->dv_hashtab.ht_used;
		    hashitem_T	*hi;

		    d->dv_copyID = copyID;
		    ga_append(gap, '{');

		    for (hi = d->dv_hashtab.ht_array; todo > 0 && !got_int;
									 ++hi)
			if (!HASHITEM_EMPTY(hi))
			{
			    --todo;
			    if (first)
				first = FALSE;
			    else
				ga_append(gap, ',');
			    if ((options & JSON_JS)
						 && is_simple_key(hi->hi_key))
				ga_concat(gap, hi->hi_key);
			    else
				write_string(gap, hi->hi_key);
			    ga_append(gap, ':');
			    if (json_encode_item(gap, &dict_lookup(hi)->di_tv,
				      copyID, options | JSON_NO_NONE) == FAIL)
				return FAIL;
			}
		    ga_append(gap, '}');
		    d->dv_copyID = 0;
		}
	    }
	    break;

	case VAR_FLOAT:
#ifdef FEAT_FLOAT
# if defined(HAVE_MATH_H)
	    if (isnan(val->vval.v_float))
		ga_concat(gap, (char_u *)"NaN");
	    else if (isinf(val->vval.v_float))
		ga_concat(gap, (char_u *)"Infinity");
	    else
# endif
	    {
		vim_snprintf((char *)numbuf, NUMBUFLEN, "%g",
							   val->vval.v_float);
		ga_concat(gap, numbuf);
	    }
	    break;
#endif
	case VAR_UNKNOWN:
	    internal_error("json_encode_item()");
	    return FAIL;
    }
    return OK;
}

/*
 * When "reader" has less than NUMBUFLEN bytes available, call the fill
 * callback to get more.
 */
    static void
fill_numbuflen(js_read_T *reader)
{
    if (reader->js_fill != NULL && (int)(reader->js_end - reader->js_buf)
						- reader->js_used < NUMBUFLEN)
    {
	if (reader->js_fill(reader))
	    reader->js_end = reader->js_buf + STRLEN(reader->js_buf);
    }
}

/*
 * Skip white space in "reader".  All characters <= space are considered white
 * space.
 * Also tops up readahead when needed.
 */
    static void
json_skip_white(js_read_T *reader)
{
    int c;

    for (;;)
    {
	c = reader->js_buf[reader->js_used];
	if (reader->js_fill != NULL && c == NUL)
	{
	    if (reader->js_fill(reader))
	    {
		reader->js_end = reader->js_buf + STRLEN(reader->js_buf);
		continue;
	    }
	}
	if (c == NUL || c > ' ')
	    break;
	++reader->js_used;
    }
    fill_numbuflen(reader);
}

    static int
json_decode_string(js_read_T *reader, typval_T *res, int quote)
{
    garray_T    ga;
    int		len;
    char_u	*p;
    int		c;
    varnumber_T	nr;

    if (res != NULL)
	ga_init2(&ga, 1, 200);

    p = reader->js_buf + reader->js_used + 1; /* skip over " or ' */
    while (*p != quote)
    {
	/* The JSON is always expected to be utf-8, thus use utf functions
	 * here. The string is converted below if needed. */
	if (*p == NUL || p[1] == NUL
#ifdef FEAT_MBYTE
		|| utf_ptr2len(p) < utf_byte2len(*p)
#endif
		)
	{
	    /* Not enough bytes to make a character or end of the string. Get
	     * more if possible. */
	    if (reader->js_fill == NULL)
		break;
	    len = (int)(reader->js_end - p);
	    reader->js_used = (int)(p - reader->js_buf);
	    if (!reader->js_fill(reader))
		break; /* didn't get more */
	    p = reader->js_buf + reader->js_used;
	    reader->js_end = reader->js_buf + STRLEN(reader->js_buf);
	    continue;
	}

	if (*p == '\\')
	{
	    c = -1;
	    switch (p[1])
	    {
		case '\\': c = '\\'; break;
		case '"': c = '"'; break;
		case 'b': c = BS; break;
		case 't': c = TAB; break;
		case 'n': c = NL; break;
		case 'f': c = FF; break;
		case 'r': c = CAR; break;
		case 'u':
		    if (reader->js_fill != NULL
				     && (int)(reader->js_end - p) < NUMBUFLEN)
		    {
			reader->js_used = (int)(p - reader->js_buf);
			if (reader->js_fill(reader))
			{
			    p = reader->js_buf + reader->js_used;
			    reader->js_end = reader->js_buf
						     + STRLEN(reader->js_buf);
			}
		    }
		    nr = 0;
		    len = 0;
		    vim_str2nr(p + 2, NULL, &len,
				     STR2NR_HEX + STR2NR_FORCE, &nr, NULL, 4);
		    p += len + 2;
		    if (0xd800 <= nr && nr <= 0xdfff
			    && (int)(reader->js_end - p) >= 6
			    && *p == '\\' && *(p+1) == 'u')
		    {
			varnumber_T	nr2 = 0;

			/* decode surrogate pair: \ud812\u3456 */
			len = 0;
			vim_str2nr(p + 2, NULL, &len,
				     STR2NR_HEX + STR2NR_FORCE, &nr2, NULL, 4);
			if (0xdc00 <= nr2 && nr2 <= 0xdfff)
			{
			    p += len + 2;
			    nr = (((nr - 0xd800) << 10) |
				((nr2 - 0xdc00) & 0x3ff)) + 0x10000;
			}
		    }
		    if (res != NULL)
		    {
#ifdef FEAT_MBYTE
			char_u	buf[NUMBUFLEN];
			buf[utf_char2bytes((int)nr, buf)] = NUL;
			ga_concat(&ga, buf);
#else
			ga_append(&ga, (int)nr);
#endif
		    }
		    break;
		default:
		    /* not a special char, skip over \ */
		    ++p;
		    continue;
	    }
	    if (c > 0)
	    {
		p += 2;
		if (res != NULL)
		    ga_append(&ga, c);
	    }
	}
	else
	{
#ifdef FEAT_MBYTE
	    len = utf_ptr2len(p);
#else
	    len = 1;
#endif
	    if (res != NULL)
	    {
		if (ga_grow(&ga, len) == FAIL)
		{
		    ga_clear(&ga);
		    return FAIL;
		}
		mch_memmove((char *)ga.ga_data + ga.ga_len, p, (size_t)len);
		ga.ga_len += len;
	    }
	    p += len;
	}
    }

    reader->js_used = (int)(p - reader->js_buf);
    if (*p == quote)
    {
	++reader->js_used;
	if (res != NULL)
	{
	    ga_append(&ga, NUL);
	    res->v_type = VAR_STRING;
#if defined(FEAT_MBYTE) && defined(USE_ICONV)
	    if (!enc_utf8)
	    {
		vimconv_T   conv;

		/* Convert the utf-8 string to 'encoding'. */
		conv.vc_type = CONV_NONE;
		convert_setup(&conv, (char_u*)"utf-8", p_enc);
		if (conv.vc_type != CONV_NONE)
		{
		    res->vval.v_string =
				      string_convert(&conv, ga.ga_data, NULL);
		    vim_free(ga.ga_data);
		}
		convert_setup(&conv, NULL, NULL);
	    }
	    else
#endif
		res->vval.v_string = ga.ga_data;
	}
	return OK;
    }
    if (res != NULL)
    {
	res->v_type = VAR_SPECIAL;
	res->vval.v_number = VVAL_NONE;
	ga_clear(&ga);
    }
    return MAYBE;
}

typedef enum {
    JSON_ARRAY,		/* parsing items in an array */
    JSON_OBJECT_KEY,	/* parsing key of an object */
    JSON_OBJECT		/* parsing item in an object, after the key */
} json_decode_T;

typedef struct {
    json_decode_T jd_type;
    typval_T	  jd_tv;	/* the list or dict */
    typval_T	  jd_key_tv;
    char_u	  *jd_key;
} json_dec_item_T;

/*
 * Decode one item and put it in "res".  If "res" is NULL only advance.
 * Must already have skipped white space.
 *
 * Return FAIL for a decoding error (and give an error).
 * Return MAYBE for an incomplete message.
 */
    


    


    


    

