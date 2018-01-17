



#include "vim.h"

#if 0
# define HT_DEBUG	

static long hash_count_lookup = 0;	
static long hash_count_perturb = 0;	
#endif


#define PERTURB_SHIFT 5

static int hash_may_resize(hashtab_T *ht, int minitems);

#if 0 

    hashtab_T *
hash_create(void)
{
    hashtab_T *ht;

    ht = (hashtab_T *)alloc(sizeof(hashtab_T));
    if (ht != NULL)
	hash_init(ht);
    return ht;
}
#endif


    void
hash_init(hashtab_T *ht)
{
    
    vim_memset(ht, 0, sizeof(hashtab_T));
    ht->ht_array = ht->ht_smallarray;
    ht->ht_mask = HT_INIT_SIZE - 1;
}


    void
hash_clear(hashtab_T *ht)
{
    if (ht->ht_array != ht->ht_smallarray)
	vim_free(ht->ht_array);
}


    void
hash_clear_all(hashtab_T *ht, int off)
{
    long	todo;
    hashitem_T	*hi;

    todo = (long)ht->ht_used;
    for (hi = ht->ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    vim_free(hi->hi_key - off);
	    --todo;
	}
    }
    hash_clear(ht);
}


    hashitem_T *
hash_find(hashtab_T *ht, char_u *key)
{
    return hash_lookup(ht, key, hash_hash(key));
}


    hashitem_T *
hash_lookup(hashtab_T *ht, char_u *key, hash_T hash)
{
    hash_T	perturb;
    hashitem_T	*freeitem;
    hashitem_T	*hi;
    unsigned	idx;

idx = (unsigned)(hash & ht->ht_mask);
#ifdef HT_DEBUG
    ++hash_count_lookup;
#endif

    
    
    hi = &ht->ht_array[idx];

    if (hi->hi_key == NULL)
	return hi;
    if (hi->hi_key == HI_KEY_REMOVED)
	freeitem = hi;
    else if (hi->hi_hash == hash && STRCMP(hi->hi_key, key) == 0)
	return hi;
    else
	freeitem = NULL;

    
    for (perturb = hash; ; perturb >>= PERTURB_SHIFT)
    {
#ifdef HT_DEBUG
	++hash_count_perturb;	    
#endif
	idx = (unsigned)((idx << 2U) + idx + perturb + 1U);
	hi = &ht->ht_array[idx & ht->ht_mask];
	if (hi->hi_key == NULL)
	    return freeitem == NULL ? hi : freeitem;
	if (hi->hi_hash == hash
		&& hi->hi_key != HI_KEY_REMOVED
		&& STRCMP(hi->hi_key, key) == 0)
	    return hi;
	if (hi->hi_key == HI_KEY_REMOVED && freeitem == NULL)
	    freeitem = hi;
    }
}


    void
hash_debug_results(void)
{
#ifdef HT_DEBUG
    fprintf(stderr, "\r\n\r\n\r\n\r\n");
    fprintf(stderr, "Number of hashtable lookups: %ld\r\n", hash_count_lookup);
    fprintf(stderr, "Number of perturb loops: %ld\r\n", hash_count_perturb);
    fprintf(stderr, "Percentage of perturb loops: %ld%%\r\n",
				hash_count_perturb * 100 / hash_count_lookup);
#endif
}


    int
hash_add(hashtab_T *ht, char_u *key)
{
    hash_T	hash = hash_hash(key);
    hashitem_T	*hi;

    hi = hash_lookup(ht, key, hash);
    if (!HASHITEM_EMPTY(hi))
    {
	internal_error("hash_add()");
	return FAIL;
    }
    return hash_add_item(ht, hi, key, hash);
}


    int
hash_add_item(
    hashtab_T	*ht,
    hashitem_T	*hi,
    char_u	*key,
    hash_T	hash)
{
    
    if (ht->ht_error && hash_may_resize(ht, 0) == FAIL)
	return FAIL;

    ++ht->ht_used;
    if (hi->hi_key == NULL)
	++ht->ht_filled;
    hi->hi_key = key;
    hi->hi_hash = hash;

    
    return hash_may_resize(ht, 0);
}

#if 0  

    void
hash_set(hashitem_T *hi, char_u *key)
{
    hi->hi_key = key;
}
#endif


    void
hash_remove(hashtab_T *ht, hashitem_T *hi)
{
    --ht->ht_used;
    hi->hi_key = HI_KEY_REMOVED;
    hash_may_resize(ht, 0);
}


    void
hash_lock(hashtab_T *ht)
{
    ++ht->ht_locked;
}

#if 0	    

    void
hash_lock_size(hashtab_T *ht, int size)
{
    (void)hash_may_resize(ht, size);
    ++ht->ht_locked;
}
#endif


    void
hash_unlock(hashtab_T *ht)
{
    --ht->ht_locked;
    (void)hash_may_resize(ht, 0);
}


    static int
hash_may_resize(
    hashtab_T	*ht,
    int		minitems)		
{
    hashitem_T	temparray[HT_INIT_SIZE];
    hashitem_T	*oldarray, *newarray;
    hashitem_T	*olditem, *newitem;
    unsigned	newi;
    int		todo;
    long_u	oldsize, newsize;
    long_u	minsize;
    long_u	newmask;
    hash_T	perturb;

    
    if (ht->ht_locked > 0)
	return OK;

#ifdef HT_DEBUG
    if (ht->ht_used > ht->ht_filled)
	EMSG("hash_may_resize(): more used than filled");
    if (ht->ht_filled >= ht->ht_mask + 1)
	EMSG("hash_may_resize(): table completely filled");
#endif

    if (minitems == 0)
    {
	
	if (ht->ht_filled < HT_INIT_SIZE - 1
					 && ht->ht_array == ht->ht_smallarray)
	    return OK;

	
	oldsize = ht->ht_mask + 1;
	if (ht->ht_filled * 3 < oldsize * 2 && ht->ht_used > oldsize / 5)
	    return OK;

	if (ht->ht_used > 1000)
	    minsize = ht->ht_used * 2;  
	else
	    minsize = ht->ht_used * 4;  
    }
    else
    {
	
	if ((long_u)minitems < ht->ht_used)	
	    minitems = (int)ht->ht_used;
	minsize = minitems * 3 / 2;	
    }

    newsize = HT_INIT_SIZE;
    while (newsize < minsize)
    {
	newsize <<= 1;		
	if (newsize == 0)
	    return FAIL;	
    }

    if (newsize == HT_INIT_SIZE)
    {
	
	newarray = ht->ht_smallarray;
	if (ht->ht_array == newarray)
	{
	    
	    mch_memmove(temparray, newarray, sizeof(temparray));
	    oldarray = temparray;
	}
	else
	    oldarray = ht->ht_array;
    }
    else
    {
	
	newarray = (hashitem_T *)alloc((unsigned)
					      (sizeof(hashitem_T) * newsize));
	if (newarray == NULL)
	{
	    
	    if (ht->ht_filled < ht->ht_mask)
		return OK;
	    ht->ht_error = TRUE;
	    return FAIL;
	}
	oldarray = ht->ht_array;
    }
    vim_memset(newarray, 0, (size_t)(sizeof(hashitem_T) * newsize));

    
    newmask = newsize - 1;
    todo = (int)ht->ht_used;
    for (olditem = oldarray; todo > 0; ++olditem)
	if (!HASHITEM_EMPTY(olditem))
	{
	    
	    newi = (unsigned)(olditem->hi_hash & newmask);
	    newitem = &newarray[newi];

	    if (newitem->hi_key != NULL)
		for (perturb = olditem->hi_hash; ; perturb >>= PERTURB_SHIFT)
		{
		    newi = (unsigned)((newi << 2U) + newi + perturb + 1U);
		    newitem = &newarray[newi & newmask];
		    if (newitem->hi_key == NULL)
			break;
		}
	    *newitem = *olditem;
	    --todo;
	}

    if (ht->ht_array != ht->ht_smallarray)
	vim_free(ht->ht_array);
    ht->ht_array = newarray;
    ht->ht_mask = newmask;
    ht->ht_filled = ht->ht_used;
    ht->ht_error = FALSE;

    return OK;
}


    hash_T
hash_hash(char_u *key)
{
    hash_T	hash;
    char_u	*p;

    if ((hash = *key) == 0)
	return (hash_T)0;
    p = key + 1;

    
    while (*p != NUL)
	hash = hash * 101 + *p++;

    return hash;
}
