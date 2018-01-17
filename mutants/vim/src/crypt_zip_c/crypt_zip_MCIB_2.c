


#include "vim.h"

#if defined(FEAT_CRYPT) || defined(PROTO)



typedef struct {
    u32_T keys[3];
} zip_state_T;
# if VIM_SIZEOF_INT >= 4
typedef unsigned int u32_T;	
# else
typedef unsigned long u32_T;	
# endif





static void make_crc_tab(void);

static u32_T crc_32_table[256];


    static void
make_crc_tab(void)
{
    u32_T	s, t, v;
    static int	done = FALSE;

    if (done)
	return;
    for (t = 0; t < 256; t++)
    {
	v = t;
	for (s = 0; s < 8; s++)
	    v = (v >> 1) ^ ((v & 1) * (u32_T)0xedb88320L);
	crc_32_table[t] = v;
    }
    done = TRUE;
}

#define CRC32(c, b) (crc_32_table[((int)(c) ^ (b)) & 0xff] ^ ((c) >> 8))


#define DECRYPT_BYTE_ZIP(keys, t) { \
    short_u temp = (short_u)keys[2] | 2; \
    t = (int)(((unsigned)(temp * (temp ^ 1U)) >> 8) & 0xff); \
}


#define UPDATE_KEYS_ZIP(keys, c) { \
    keys[0] = CRC32(keys[0], (c)); \
    keys[1] += keys[0] & 0xff; \
    keys[1] = keys[1] * 134775813L + 1; \
    keys[2] = CRC32(keys[2], (int)(keys[1] >> 24)); \
}


    void
crypt_zip_init(
    cryptstate_T    *state,
    char_u	    *key,
    char_u	    *salt UNUSED,
    int		    salt_len UNUSED,
    char_u	    *seed UNUSED,
    int		    seed_len UNUSED)
{
    char_u	*p;
    zip_state_T	*zs;

    zs = (zip_state_T *)alloc(sizeof(zip_state_T));
    state->method_state = zs;

    make_crc_tab();
    zs->keys[0] = 305419896L;
    zs->keys[1] = 591751049L;
    zs->keys[2] = 878082192L;
    for (p = key; *p != NUL; ++p)
    {
	UPDATE_KEYS_ZIP(zs->keys, (int)*p);
    }
}


    void
crypt_zip_encode(
    cryptstate_T *state,
    char_u	*from,
    size_t	len,
    char_u	*to)
{
    zip_state_T *zs = state->method_state;
    size_t	i;
    int		ztemp, t;

    for (i = 0; i < len; ++i)
    {
	ztemp = from[i];
	DECRYPT_BYTE_ZIP(zs->keys, t);
	UPDATE_KEYS_ZIP(zs->keys, ztemp);
	to[i] = t ^ ztemp;
    }
}


    void
crypt_zip_decode(
    cryptstate_T *state,
    char_u	*from,
    size_t	len,
    char_u	*to)
{
    zip_state_T *zs = state->method_state;
    size_t	i;
    short_u	temp;

    for (i = 0; i < len; ++i)
    {
	temp = (short_u)zs->keys[2] | 2;
	temp = (int)(((unsigned)(temp * (temp ^ 1U)) >> 8) & 0xff);
	UPDATE_KEYS_ZIP(zs->keys, to[i] = from[i] ^ temp);
    }
}

#endif 
