
#include <assert.h>

#include "memory_pool.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if IS_SHOWDEBUG_MESSAGE
#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)


#define mpl_memalign(alignment, size)		mpl_malloc(size)

static void *mpl_palloc_small(MPL_POOL_S *pool, size_t size, int align);
static void *mpl_palloc_block(MPL_POOL_S *pool, size_t size);
static void *mpl_palloc_large(MPL_POOL_S *pool, size_t size);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void* mpl_malloc(size_t size)
{
	void *p = NULL;

	p = malloc(size);

	if(p == NULL)
	{
		PERROR("There is something wrong about system malloc function\n");
	}

	return p;
}

void* mpl_calloc(size_t size)
{
	void *p = NULL;

	p = mpl_malloc(size);

	if(p)
	{
		memset(p,0,size);
	}

	return p;
}

#if 0
void* mpl_memalign_posix(size_t alignment, size_t size)
{
	void *p = NULL;
	int err = 0;
	
	err = posix_memalign(&p, alignment, size);
	//TODO
	
	return p;
}

void* mpl_memalign_(size_t alignment, size_t size)
{
	void *p = NULL;

	p = memalign(alignment, size);
	//TODO
	
	return p;
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MPL_POOL_S* mpl_create_pool(size_t size)
{
	MPL_POOL_S  *p = NULL;

	p = mpl_memalign(MPL_POOL_ALIGNMENT, size);
    if (p == NULL) {
        return NULL;
    }

	p->d.last = (u_char *) p + sizeof(MPL_POOL_S);
    p->d.end = (u_char *) p + size;
    p->d.next = NULL;
    p->d.failed = 0;

    size = size - sizeof(MPL_POOL_S);
    p->max = (size < MPL_MAX_ALLOC_FROM_POOL) ? size : MPL_MAX_ALLOC_FROM_POOL;

    p->current = p;
    p->large = NULL;

    return p;
}

void mpl_destroy_pool(MPL_POOL_S *pool)
{
	MPL_POOL_S          *p = NULL, *n = NULL;
    MPL_POOL_LARGE_S    *l = NULL;

	for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }

	for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        free(p);
        if (n == NULL) {
            break;
        }
    }
}

void mpl_reset_pool(MPL_POOL_S *pool)
{
	MPL_POOL_S        *p = NULL;
    MPL_POOL_LARGE_S  *l = NULL;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }

    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(MPL_POOL_S);
        p->d.failed = 0;
    }

    pool->current = pool;
    pool->large = NULL;
}


void* mpl_palloc(MPL_POOL_S *pool,size_t size)
{
	if (size <= pool->max) {
        return mpl_palloc_small(pool, size, 1);
    }

	return mpl_palloc_large(pool, size);
}

void* mpl_pnalloc(MPL_POOL_S *pool,size_t size)
{
	if (size <= pool->max) {
        return mpl_palloc_small(pool, size, 0);
    }

	return mpl_palloc_large(pool, size);
}

static void* mpl_palloc_small(MPL_POOL_S *pool, size_t size, int align)
{
    u_char      *m = NULL;
    MPL_POOL_S  *p = NULL;

    p = pool->current;

    do {
        m = p->d.last;

        if (align) {
            m = mpl_align_ptr(m, MPL_ALIGNMENT);
        }

        if ((size_t) (p->d.end - m) >= size) {
            p->d.last = m + size;

            return m;
        }

        p = p->d.next;

    } while (p);

    return mpl_palloc_block(pool, size);
}

static void* mpl_palloc_block(MPL_POOL_S *pool, size_t size)
{
    u_char      *m = NULL;
    size_t       psize = 0;
    MPL_POOL_S  *p = NULL, *new = NULL;

    psize = (size_t) (pool->d.end - (u_char *) pool);

    m = mpl_memalign(MPL_POOL_ALIGNMENT, psize);
    if (m == NULL) {
        return NULL;
    }

    new = (MPL_POOL_S *) m;

    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    m += sizeof(MPL_POOL_DATA_S);
    m = mpl_align_ptr(m, MPL_ALIGNMENT);
    new->d.last = m + size;

    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;
        }
    }

    p->d.next = new;

    return m;
}


static void* mpl_palloc_large(MPL_POOL_S *pool, size_t size)
{
    void				*p = NULL;
    int					n = 0;
    MPL_POOL_LARGE_S	*large = NULL;

    p = mpl_malloc(size);
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = mpl_palloc_small(pool, sizeof(MPL_POOL_LARGE_S), 1);
    if (large == NULL) {
        free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}

