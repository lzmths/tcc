#include "first.h"

#include "buffer.h"

#include <stdlib.h>
#include <string.h>

static const char hex_chars[] = "0123456789abcdef";



buffer* buffer_init(void) {
	buffer *b;

	b = malloc(sizeof(*b));
	force_assert(b);

	b->ptr = NULL;
	b->size = 0;
	b->used = 0;

	return b;
}

buffer *buffer_init_buffer(const buffer *src) {
	buffer *b = buffer_init();
	buffer_copy_buffer(b, src);
	return b;
}

buffer *buffer_init_string(const char *str) {
	buffer *b = buffer_init();
	buffer_copy_string(b, str);
	return b;
}

void buffer_free(buffer *b) {
	if (NULL == b) return;

	free(b->ptr);
	free(b);
}

void buffer_reset(buffer *b) {
	if (NULL == b) return;

	
	if (b->size > BUFFER_MAX_REUSE_SIZE) {
		free(b->ptr);
		b->ptr = NULL;
		b->size = 0;
	} else if (b->size > 0) {
		b->ptr[0] = '\0';
	}

	b->used = 0;
}

void buffer_move(buffer *b, buffer *src) {
	buffer tmp;

	if (NULL == b) {
		buffer_reset(src);
		return;
	}
	buffer_reset(b);
	if (NULL == src) return;

	tmp = *src; *src = *b; *b = tmp;
}

#define BUFFER_PIECE_SIZE 64
static size_t buffer_align_size(size_t size) {
	size_t align = BUFFER_PIECE_SIZE - (size % BUFFER_PIECE_SIZE);
	
	if (size + align < size) return size;
	return size + align;
}


static void buffer_alloc(buffer *b, size_t size) {
	force_assert(NULL != b);
	if (0 == size) size = 1;

	if (size <= b->size) return;

	if (NULL != b->ptr) free(b->ptr);

	b->used = 0;
	b->size = buffer_align_size(size);
	b->ptr = malloc(b->size);

	force_assert(NULL != b->ptr);
}


static void buffer_realloc(buffer *b, size_t size) {
	force_assert(NULL != b);
	if (0 == size) size = 1;

	if (size <= b->size) return;

	b->size = buffer_align_size(size);
	b->ptr = realloc(b->ptr, b->size);

	force_assert(NULL != b->ptr);
}


char* buffer_string_prepare_copy(buffer *b, size_t size) {
	force_assert(NULL != b);
	force_assert(size + 1 > size);

	buffer_alloc(b, size + 1);

	b->used = 1;
	b->ptr[0] = '\0';

	return b->ptr;
}

char* buffer_string_prepare_append(buffer *b, size_t size) {
	force_assert(NULL !=  b);

	if (buffer_string_is_empty(b)) {
		return buffer_string_prepare_copy(b, size);
	} else {
		size_t req_size = b->used + size;

		
		force_assert(req_size >= b->used);

		
		force_assert(req_size >= b->used);

		buffer_realloc(b, req_size);

		return b->ptr + b->used - 1;
	}
}

void buffer_string_set_length(buffer *b, size_t len) {
	force_assert(NULL != b);
	force_assert(len + 1 > len);

	buffer_realloc(b, len + 1);

	b->used = len + 1;
	b->ptr[len] = '\0';
}

void buffer_commit(buffer *b, size_t size)
{
	force_assert(NULL != b);
	force_assert(b->size > 0);

	if (0 == b->used) b->used = 1;

	if (size > 0) {
		
		force_assert(b->used + size > b->used);

		force_assert(b->used + size <= b->size);
		b->used += size;
	}

	b->ptr[b->used - 1] = '\0';
}

void buffer_copy_string(buffer *b, const char *s) {
	buffer_copy_string_len(b, s, NULL != s ? strlen(s) : 0);
}

void buffer_copy_string_len(buffer *b, const char *s, size_t s_len) {
	force_assert(NULL != b);
	force_assert(NULL != s || s_len == 0);

	buffer_string_prepare_copy(b, s_len);

	if (0 != s_len) memcpy(b->ptr, s, s_len);

	buffer_commit(b, s_len);
}

void buffer_copy_buffer(buffer *b, const buffer *src) {
	if (NULL == src || 0 == src->used) {
		buffer_string_prepare_copy(b, 0);
		b->used = 0; 
	} else {
		buffer_copy_string_len(b, src->ptr, buffer_string_length(src));
	}
}

void buffer_append_string(buffer *b, const char *s) {
	buffer_append_string_len(b, s, NULL != s ? strlen(s) : 0);
}



void buffer_append_string_len(buffer *b, const char *s, size_t s_len) {
	char *target_buf;

	force_assert(NULL != b);
	force_assert(NULL != s || s_len == 0);

	target_buf = buffer_string_prepare_append(b, s_len);

	if (0 == s_len) return; 

	memcpy(target_buf, s, s_len);

	buffer_commit(b, s_len);
}

void buffer_append_string_buffer(buffer *b, const buffer *src) {
	if (NULL == src) {
		buffer_append_string_len(b, NULL, 0);
	} else {
		buffer_append_string_len(b, src->ptr, buffer_string_length(src));
	}
}

void buffer_append_uint_hex(buffer *b, uintmax_t value) {
	char *buf;
	unsigned int shift = 0;

	{
		uintmax_t copy = value;
		do {
			copy >>= 8;
			shift += 8; 
		} while (0 != copy);
	}

	buf = buffer_string_prepare_append(b, shift >> 2); 
	buffer_commit(b, shift >> 2); 

	while (shift > 0) {
		shift -= 4;
		*(buf++) = hex_chars[(value >> shift) & 0x0F];
	}
}

static char* utostr(char * const buf_end, uintmax_t val) {
	char *cur = buf_end;
	do {
		int mod = val % 10;
		val /= 10;
		
		*(--cur) = (char) ('0' + mod);
	} while (0 != val);
	return cur;
}

static char* itostr(char * const buf_end, intmax_t val) {
	
	uintmax_t uval = val >= 0 ? (uintmax_t)val : ((uintmax_t)~val) + 1;
	char *cur = utostr(buf_end, uval);
	if (val < 0) *(--cur) = '-';

	return cur;
}

void buffer_append_int(buffer *b, intmax_t val) {
	char buf[LI_ITOSTRING_LENGTH];
	char* const buf_end = buf + sizeof(buf);
	char *str;

	force_assert(NULL != b);

	str = itostr(buf_end, val);
	force_assert(buf_end > str && str >= buf);

	buffer_append_string_len(b, str, buf_end - str);
}

void buffer_copy_int(buffer *b, intmax_t val) {
	force_assert(NULL != b);

	b->used = 0;
	buffer_append_int(b, val);
}

void buffer_append_strftime(buffer *b, const char *format, const struct tm *tm) {
	size_t r;
	char* buf;
	force_assert(NULL != b);
	force_assert(NULL != tm);

	if (NULL == format || '\0' == format[0]) {
		
		buffer_string_prepare_append(b, 0);
		return;
	}

	buf = buffer_string_prepare_append(b, 255);
	r = strftime(buf, buffer_string_space(b), format, tm);

	
	if (0 == r || r >= buffer_string_space(b)) {
		
		buf = buffer_string_prepare_append(b, 4095);
		r = strftime(buf, buffer_string_space(b), format, tm);
	}

	if (r >= buffer_string_space(b)) r = 0;

	buffer_commit(b, r);
}


void li_itostrn(char *buf, size_t buf_len, intmax_t val) {
	char p_buf[LI_ITOSTRING_LENGTH];
	char* const p_buf_end = p_buf + sizeof(p_buf);
	char* str = p_buf_end - 1;
	*str = '\0';

	str = itostr(str, val);
	force_assert(p_buf_end > str && str >= p_buf);

	force_assert(buf_len >= (size_t) (p_buf_end - str));
	memcpy(buf, str, p_buf_end - str);
}

void li_utostrn(char *buf, size_t buf_len, uintmax_t val) {
	char p_buf[LI_ITOSTRING_LENGTH];
	char* const p_buf_end = p_buf + sizeof(p_buf);
	char* str = p_buf_end - 1;
	*str = '\0';

	str = utostr(str, val);
	force_assert(p_buf_end > str && str >= p_buf);

	force_assert(buf_len >= (size_t) (p_buf_end - str));
	memcpy(buf, str, p_buf_end - str);
}

char int2hex(char c) {
	return hex_chars[(c & 0x0F)];
}


char hex2int(unsigned char hex) {
	unsigned char value = hex - '0';
	if (value > 9) {
		hex |= 0x20; 
		value = hex - 'a' + 10;
		if (value < 10) value = 0xff;
	}
	if (value > 15) value = 0xff;

	return value;
}



int buffer_is_equal(const buffer *a, const buffer *b) {
	force_assert(NULL != a && NULL != b);

	if (a->used != b->used) return 0;
	if (a->used == 0) return 1;

	return (0 == memcmp(a->ptr, b->ptr, a->used));
}

int buffer_is_equal_string(const buffer *a, const char *s, size_t b_len) {
	force_assert(NULL != a && NULL != s);
	force_assert(b_len + 1 > b_len);

	if (a->used != b_len + 1) return 0;
	if (0 != memcmp(a->ptr, s, b_len)) return 0;
	if ('\0' != a->ptr[a->used-1]) return 0;

	return 1;
}


int buffer_is_equal_caseless_string(const buffer *a, const char *s, size_t b_len) {
	force_assert(NULL != a);
	if (a->used != b_len + 1) return 0;
	force_assert('\0' == a->ptr[a->used - 1]);

	return (0 == strcasecmp(a->ptr, s));
}

int buffer_caseless_compare(const char *a, size_t a_len, const char *b, size_t b_len) {
	size_t const len = (a_len < b_len) ? a_len : b_len;
	size_t i;

	for (i = 0; i < len; ++i) {
		unsigned char ca = a[i], cb = b[i];
		if (ca == cb) continue;

		
		if (ca >= 'A' && ca <= 'Z') ca |= 32;
		if (cb >= 'A' && cb <= 'Z') cb |= 32;

		if (ca == cb) continue;
		return ((int)ca) - ((int)cb);
	}
	if (a_len == b_len) return 0;
	return a_len < b_len ? -1 : 1;
}

int buffer_is_equal_right_len(const buffer *b1, const buffer *b2, size_t len) {
	
	if (len == 0) return 1;

	
	if (b1->used == 0 || b2->used == 0) return 0;

	
	if (b1->used - 1 < len || b2->used - 1 < len) return 0;

	return 0 == memcmp(b1->ptr + b1->used - 1 - len, b2->ptr + b2->used - 1 - len, len);
}

void li_tohex(char *buf, size_t buf_len, const char *s, size_t s_len) {
	size_t i;
	force_assert(2 * s_len > s_len);
	force_assert(2 * s_len < buf_len);

	for (i = 0; i < s_len; i++) {
		buf[2*i] = hex_chars[(s[i] >> 4) & 0x0F];
		buf[2*i+1] = hex_chars[s[i] & 0x0F];
	}
	buf[2*s_len] = '\0';
}

void buffer_copy_string_hex(buffer *b, const char *in, size_t in_len) {
	
	force_assert(in_len * 2 > in_len);

	buffer_string_set_length(b, 2 * in_len);
	li_tohex(b->ptr, buffer_string_length(b)+1, in, in_len);
}


void buffer_substr_replace (buffer * const b, const size_t offset,
                            const size_t len, const buffer * const replace)
{
    const size_t blen = buffer_string_length(b);
    const size_t rlen = buffer_string_length(replace);

    if (rlen > len) {
        buffer_string_set_length(b, blen-len+rlen);
        memmove(b->ptr+offset+rlen, b->ptr+offset+len, blen-offset-len);
    }

    memcpy(b->ptr+offset, replace->ptr, rlen);

    if (rlen < len) {
        memmove(b->ptr+offset+rlen, b->ptr+offset+len, blen-offset-len);
        buffer_string_set_length(b, blen-len+rlen);
    }
}



static const char encoded_chars_rel_uri_part[] = {
	
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,  
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,  
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
};


static const char encoded_chars_rel_uri[] = {
	
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,  
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,  
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
};

static const char encoded_chars_html[] = {
	
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
};

static const char encoded_chars_minimal_xml[] = {
	
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
};

static const char encoded_chars_hex[] = {
	
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  
};

static const char encoded_chars_http_header[] = {
	
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
};



void buffer_append_string_encoded(buffer *b, const char *s, size_t s_len, buffer_encoding_t encoding) {
	unsigned char *ds, *d;
	size_t d_len, ndx;
	const char *map = NULL;

	force_assert(NULL != b);
	force_assert(NULL != s || 0 == s_len);

	if (0 == s_len) return;

	switch(encoding) {
	case ENCODING_REL_URI:
		map = encoded_chars_rel_uri;
		break;
	case ENCODING_REL_URI_PART:
		map = encoded_chars_rel_uri_part;
		break;
	case ENCODING_HTML:
		map = encoded_chars_html;
		break;
	case ENCODING_MINIMAL_XML:
		map = encoded_chars_minimal_xml;
		break;
	case ENCODING_HEX:
		map = encoded_chars_hex;
		break;
	case ENCODING_HTTP_HEADER:
		map = encoded_chars_http_header;
		break;
	}

	force_assert(NULL != map);

	
	for (ds = (unsigned char *)s, d_len = 0, ndx = 0; ndx < s_len; ds++, ndx++) {
		if (map[*ds]) {
			switch(encoding) {
			case ENCODING_REL_URI:
			case ENCODING_REL_URI_PART:
				d_len += 3;
				break;
			case ENCODING_HTML:
			case ENCODING_MINIMAL_XML:
				d_len += 6;
				break;
			case ENCODING_HTTP_HEADER:
			case ENCODING_HEX:
				d_len += 2;
				break;
			}
		} else {
			d_len++;
		}
	}

	d = (unsigned char*) buffer_string_prepare_append(b, d_len);
	buffer_commit(b, d_len); 
	force_assert('\0' == *d);

	for (ds = (unsigned char *)s, d_len = 0, ndx = 0; ndx < s_len; ds++, ndx++) {
		if (map[*ds]) {
			switch(encoding) {
			case ENCODING_REL_URI:
			case ENCODING_REL_URI_PART:
				d[d_len++] = '%';
				d[d_len++] = hex_chars[((*ds) >> 4) & 0x0F];
				d[d_len++] = hex_chars[(*ds) & 0x0F];
				break;
			case ENCODING_HTML:
			case ENCODING_MINIMAL_XML:
				d[d_len++] = '&';
				d[d_len++] = '#';
				d[d_len++] = 'x';
				d[d_len++] = hex_chars[((*ds) >> 4) & 0x0F];
				d[d_len++] = hex_chars[(*ds) & 0x0F];
				d[d_len++] = ';';
				break;
			case ENCODING_HEX:
				d[d_len++] = hex_chars[((*ds) >> 4) & 0x0F];
				d[d_len++] = hex_chars[(*ds) & 0x0F];
				break;
			case ENCODING_HTTP_HEADER:
				d[d_len++] = *ds;
				d[d_len++] = '\t';
				break;
			}
		} else {
			d[d_len++] = *ds;
		}
	}
}

void buffer_append_string_c_escaped(buffer *b, const char *s, size_t s_len) {
	unsigned char *ds, *d;
	size_t d_len, ndx;

	force_assert(NULL != b);
	force_assert(NULL != s || 0 == s_len);

	if (0 == s_len) return;

	
	for (ds = (unsigned char *)s, d_len = 0, ndx = 0; ndx < s_len; ds++, ndx++) {
		if ((*ds < 0x20) 
				|| (*ds >= 0x7f)) { 
			switch (*ds) {
			case '\t':
			case '\r':
			case '\n':
				d_len += 2;
				break;
			default:
				d_len += 4; 
				break;
			}
		} else {
			d_len++;
		}
	}

	d = (unsigned char*) buffer_string_prepare_append(b, d_len);
	buffer_commit(b, d_len); 
	force_assert('\0' == *d);

	for (ds = (unsigned char *)s, d_len = 0, ndx = 0; ndx < s_len; ds++, ndx++) {
		if ((*ds < 0x20) 
				|| (*ds >= 0x7f)) { 
			d[d_len++] = '\\';
			switch (*ds) {
			case '\t':
				d[d_len++] = 't';
				break;
			case '\r':
				d[d_len++] = 'r';
				break;
			case '\n':
				d[d_len++] = 'n';
				break;
			default:
				d[d_len++] = 'x';
				d[d_len++] = hex_chars[((*ds) >> 4) & 0x0F];
				d[d_len++] = hex_chars[(*ds) & 0x0F];
				break;
			}
		} else {
			d[d_len++] = *ds;
		}
	}
}


void buffer_copy_string_encoded_cgi_varnames(buffer *b, const char *s, size_t s_len, int is_http_header) {
	size_t i, j;

	force_assert(NULL != b);
	force_assert(NULL != s || 0 == s_len);

	buffer_reset(b);

	if (is_http_header && NULL != s && 0 != strcasecmp(s, "CONTENT-TYPE")) {
		buffer_string_prepare_append(b, s_len + 5);
		buffer_copy_string_len(b, CONST_STR_LEN("HTTP_"));
	} else {
		buffer_string_prepare_append(b, s_len);
	}

	j = buffer_string_length(b);
	for (i = 0; i < s_len; ++i) {
		unsigned char cr = s[i];
		if (light_isalpha(cr)) {
			
			cr &= ~32;
		} else if (!light_isdigit(cr)) {
			cr = '_';
		}
		b->ptr[j++] = cr;
	}
	b->used = j;
	b->ptr[b->used++] = '\0';
}



static void buffer_urldecode_internal(buffer *url, int is_query) {
	unsigned char high, low;
	char *src;
	char *dst;

	force_assert(NULL != url);
	if (buffer_string_is_empty(url)) return;

	force_assert('\0' == url->ptr[url->used-1]);

	src = (char*) url->ptr;

	while ('\0' != *src) {
		if ('%' == *src) break;
		if (is_query && '+' == *src) *src = ' ';
		src++;
	}
	dst = src;

	while ('\0' != *src) {
		if (is_query && *src == '+') {
			*dst = ' ';
		} else if (*src == '%') {
			*dst = '%';

			high = hex2int(*(src + 1));
			if (0xFF != high) {
				low = hex2int(*(src + 2));
				if (0xFF != low) {
					high = (high << 4) | low;

					
					if (high < 32 || high == 127) high = '_';

					*dst = high;
					src += 2;
				}
			}
		} else {
			*dst = *src;
		}

		dst++;
		src++;
	}

	*dst = '\0';
	url->used = (dst - url->ptr) + 1;
}

void buffer_urldecode_path(buffer *url) {
	buffer_urldecode_internal(url, 0);
}

void buffer_urldecode_query(buffer *url) {
	buffer_urldecode_internal(url, 1);
}



void buffer_path_simplify(buffer *dest, buffer *src)
{
	
	char c, pre1, pre2;
	char *start, *slash, *walk, *out;

	force_assert(NULL != dest && NULL != src);

	if (buffer_string_is_empty(src)) {
		buffer_string_prepare_copy(dest, 0);
		return;
	}

	force_assert('\0' == src->ptr[src->used-1]);

	
	if (src == dest) {
		buffer_string_prepare_append(dest, 1);
	} else {
		buffer_string_prepare_copy(dest, buffer_string_length(src) + 1);
	}

#if defined(__WIN32) || defined(__CYGWIN__)
	
	{
		char *p;
		for (p = src->ptr; *p; p++) {
			if (*p == '\\') *p = '/';
		}
	}
#endif

	walk  = src->ptr;
	start = dest->ptr;
	out   = dest->ptr;
	slash = dest->ptr;

	
	while (*walk == ' ') {
		walk++;
	}

	pre1 = 0;
	c = *(walk++);
	
	if (c != '/') {
		pre1 = '/';
		*(out++) = '/';
	}

	while (c != '\0') {
		
		
		pre2 = pre1;
		pre1 = c;

		
		c    = *walk;
		*out = pre1;

		out++;
		walk++;
		

		if (c == '/' || c == '\0') {
			const size_t toklen = out - slash;
			if (toklen == 3 && pre2 == '.' && pre1 == '.') {
				
				out = slash;
				
				if (out > start) {
					out--;
					while (out > start && *out != '/') out--;
				}

				
				if (c == '\0') out++;
				
			} else if (toklen == 1 || (pre2 == '/' && pre1 == '.')) {
				
				out = slash;
				
				if (c == '\0') out++;
				
			}

			slash = out;
		}
	}

	buffer_string_set_length(dest, out - start);
}

int light_isdigit(int c) {
	return (c >= '0' && c <= '9');
}

int light_isxdigit(int c) {
	if (light_isdigit(c)) return 1;

	c |= 32;
	return (c >= 'a' && c <= 'f');
}

int light_isalpha(int c) {
	c |= 32;
	return (c >= 'a' && c <= 'z');
}

int light_isalnum(int c) {
	return light_isdigit(c) || light_isalpha(c);
}

void buffer_to_lower(buffer *b) {
	size_t i;

	for (i = 0; i < b->used; ++i) {
		char c = b->ptr[i];
		if (c >= 'A' && c <= 'Z') b->ptr[i] |= 0x20;
	}
}


void buffer_to_upper(buffer *b) {
	size_t i;

	for (i = 0; i < b->used; ++i) {
		char c = b->ptr[i];
		if (c >= 'A' && c <= 'Z') b->ptr[i] &= ~0x20;
	}
}




#ifdef HAVE_LIBUNWIND
# define UNW_LOCAL_ONLY
# include <libunwind.h>

static void print_backtrace(FILE *file) {
	unw_cursor_t cursor;
	unw_context_t context;
	int ret;
	unsigned int frame = 0;

	if (0 != (ret = unw_getcontext(&context))) goto error;
	if (0 != (ret = unw_init_local(&cursor, &context))) goto error;

	fprintf(file, "Backtrace:\n");

	while (0 < (ret = unw_step(&cursor))) {
		unw_word_t proc_ip = 0;
		unw_proc_info_t procinfo;
		char procname[256];
		unw_word_t proc_offset = 0;

		if (0 != (ret = unw_get_reg(&cursor, UNW_REG_IP, &proc_ip))) goto error;

		if (0 == proc_ip) {
			
			++frame;
			fprintf(file, "%u: (nil)\n", frame);
			continue;
		}

		if (0 != (ret = unw_get_proc_info(&cursor, &procinfo))) goto error;

		if (0 != (ret = unw_get_proc_name(&cursor, procname, sizeof(procname), &proc_offset))) {
			switch (-ret) {
			case UNW_ENOMEM:
				memset(procname + sizeof(procname) - 4, '.', 3);
				procname[sizeof(procname) - 1] = '\0';
				break;
			case UNW_ENOINFO:
				procname[0] = '?';
				procname[1] = '\0';
				proc_offset = 0;
				break;
			default:
				snprintf(procname, sizeof(procname), "?? (unw_get_proc_name error %d)", -ret);
				break;
			}
		}

		++frame;
		fprintf(file, "%u: %s (+0x%x) [%p]\n",
			frame,
			procname,
			(unsigned int) proc_offset,
			(void*)(uintptr_t)proc_ip);
	}

	if (0 != ret) goto error;

	return;

error:
	fprintf(file, "Error while generating backtrace: unwind error %i\n", (int) -ret);
}
#else
static void print_backtrace(FILE *file) {
	UNUSED(file);
}
#endif

#include <stdio.h>
void log_failed_assert(const char *filename, unsigned int line, const char *msg) {
	
	fprintf(stderr, "%s.%u: %s\n", filename, line, msg);
	print_backtrace(stderr);
	fflush(stderr);
	abort();
}
