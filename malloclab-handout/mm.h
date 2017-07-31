#include <stdio.h>

extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
extern void *mm_realloc(void *ptr, size_t size);

extern void *extend_heap(size_t words);
extern void *place(void *bp, size_t asize);
extern void *findfit(size_t asize);
extern void *find_precise(size_t asize);
extern void *coalesce(void *bp);
extern void delete_block(void *bp);
extern void add_block(void *bp);
extern void add_free_block(void *fl, void *bp) ;
extern void delete_free_block(void *fl, void *bp);
extern void add_block_list(void *bp);
extern void delete_block_list(void *bp);
extern void *find_biggest(void* hd);
extern int mm_check();
extern void print_block();
extern void print_tree();
extern void check_list_in(void* bp);
extern void check_free_block();
extern void* find_parent(void* bp);


/* 
 * Students work in teams of one or two.  Teams enter their team name, 
 * personal names and login IDs in a struct of this
 * type in their bits.c file.
 */
typedef struct {
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *id1;      /* login ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *id2;      /* login ID of second member */
} team_t;

extern team_t team;

