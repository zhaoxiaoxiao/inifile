#ifndef __XIAOXIAO_MEMORY_POOL_H__
#define __XIAOXIAO_MEMORY_POOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MPL_ALIGNMENT   sizeof(unsigned long)    /* platform word */

#define mpl_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define mpl_align_ptr(p, a)                                                   \
    (u_char *) (((u_long) (p) + ((u_long) a - 1)) & ~((u_long) a - 1))

#define MPL_POOL_ALIGNMENT			16
#define MPL_MAX_ALLOC_FROM_POOL		(4096 - 1)
#define MPL_DEFAULT_POOL_SIZE    	(16 * 1024)

#define MPL_MIN_POOL_SIZE                                                     \
    mpl_align((sizeof(MPL_POOL_S) + 2 * sizeof(MPL_POOL_LARGE_S)),            \
              MPL_POOL_ALIGNMENT)
              
typedef struct mpl_pool_s MPL_POOL_S;

typedef struct mpl_pool_large_s{
	struct mpl_pool_large_s 	*next;
	void                 		*alloc;
}MPL_POOL_LARGE_S;

typedef struct mpl_pool_data_s{
	unsigned char		*last;
	unsigned char		*end;
	MPL_POOL_S			*next;
	unsigned int		failed;
}MPL_POOL_DATA_S;

struct mpl_pool_s{
	MPL_POOL_DATA_S		d;
	size_t				max;
	MPL_POOL_S			*current;
	MPL_POOL_LARGE_S	*large;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////

void* mpl_malloc(size_t size);

void* mpl_calloc(size_t size);

/////////////////////////////////////////////////////////////////////////////////////////////////////
MPL_POOL_S* mpl_create_pool(size_t size);

//destroy memory pool all resources
void mpl_destroy_pool(MPL_POOL_S *pool);

//reset memory pool all malloc memory
void mpl_reset_pool(MPL_POOL_S *pool);

//byte alignment malloc
void* mpl_palloc(MPL_POOL_S *pool,size_t size);

//malloc size byte ignore byte alignment
void* mpl_pnalloc(MPL_POOL_S *pool,size_t size);

//if free memory in large pool will be free.otherwise will no be free
int mpl_pfree(MPL_POOL_S *pool, void *p);

#ifdef __cplusplus
}
#endif
#endif