/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "gg",
    /* Second member's email address (leave blank if none) */
    "gg"
};

/* single word (4) or double word (8) alignment */
#define WSIZE 4
#define DSIZE 8
/* Extend heap by this amount (bytes) */
#define CHUNKSIZE (1<<12) 

#define MAX(x,y) ((x) > (y)? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

/*read and write a word at address p*/
#define GET(p)     (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (unsigned int)(val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)    ((char*)(bp) - WSIZE)
#define FTRP(bp)    ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks*/
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE)))

/* Given block ptr bp, compute address of next and previous blocks in lists*/
#define NEXT_PTR(bp) ((char*)GET(bp))
#define PREV_PTR(bp) ((char*)GET((char*)(bp) + WSIZE))
#define LEFT_PTR(bp) ((char*)GET((char*)(bp) + 2*WSIZE))
#define RIGHT_PTR(bp) ((char*)GET((char*)(bp) + 3*WSIZE))

#define PTR_NEXT_PTR(bp) ((char*)(bp))
#define PTR_PREV_PTR(bp) ((char*)((char*)(bp) + WSIZE))
#define PTR_LEFT_PTR(bp) ((char*)((char*)(bp) + 2*WSIZE))
#define PTR_RIGHT_PTR(bp) ((char*)((char*)(bp) + 3*WSIZE))

/* rounds up to the nearest multiple of DSIZE*/
#define ALIGN(size) (((size) + (DSIZE-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* head ptr of free block list */
#define PTR_LIST(ptr,size) ((char*)(ptr) - 8*WSIZE + ((size)-DSIZE)/DSIZE*WSIZE)
static char *ptr_begin;
/* head ptr of big free block list */
static char *ptr_big;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	ptr_begin = (char*)mem_heap_lo();
	ptr_big = (ptr_begin+6*WSIZE);
	/*Create the initial empty heap*/
	if ((ptr_begin = mem_sbrk(10*WSIZE)) == (void *)-1)
		return -1;

	PUT(ptr_begin,0);
	PUT(ptr_begin+WSIZE, 0);
	PUT(ptr_begin+2*WSIZE,0);
	PUT(ptr_begin+3*WSIZE,0);
	PUT(ptr_begin+4*WSIZE,0);
	PUT(ptr_begin+5*WSIZE,0);
	PUT(ptr_big,0);
	PUT(ptr_big + WSIZE, PACK(DSIZE,1));
	PUT(ptr_big + 2*WSIZE, PACK(DSIZE,1));
	PUT(ptr_big + 3*WSIZE,PACK(0,1));
	ptr_begin = ptr_big + 2*WSIZE;

	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)	
		return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	size_t newsize;
	size_t extendsize;
	char *bp;

	if (size == 0) 
		return NULL;

    newsize = ALIGN(size + SIZE_T_SIZE);
    
    if ((bp = findfit(newsize)) != NULL) {
        place(bp, newsize);
        return bp;
    }

    extendsize = MAX(newsize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
    	return NULL;
    place(bp,newsize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	size_t size = GET_SIZE(HDRP(ptr));

	PUT(HDRP(ptr), PACK(size,0));
	PUT(FTRP(ptr), PACK(size,0));
	coalesce(ptr);
}
/*
 * extend_heap - extend heap size and add new free block into tree
 */
void *extend_heap(size_t words) { 
	char *bp;
	size_t size;

	/* Allocate an even number of words to maintain alignment*/
	size = ALIGN(words*WSIZE);
	if((long)(bp = mem_sbrk(size)) == -1)
		return NULL;

	/*Initialize free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));

	/*Coalesce if the previous block was free */
	return (void*)coalesce(bp);
}

/*
 * place - place requested block in the beginning of free block, split when needed.
 */
void *place(void *bp, size_t asize) {
	size_t csize = GET_SIZE(HDRP(bp)); /* clock size*/
	size_t lsize = csize - asize; /* left clock size*/
	if(asize > csize) {
		printf("Cannot place bigger size in small free block.\n");
	}
	/*delete this free block from tree or small list*/
	delete_block(bp);
	if (lsize >= 2*DSIZE) {
		PUT(HDRP(bp),PACK(asize,1));
		PUT(FTRP(bp),PACK(asize,1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp),PACK(lsize,0));
		PUT(FTRP(bp),PACK(lsize,0));
		add_block(bp);
	}
	else {
		PUT(HDRP(bp),PACK(csize,1));
		PUT(FTRP(bp),PACK(csize,1));
	}
	return (void*)bp;
}

/*
 * findfit - find the first free block fit for requested one
 */
void *findfit(size_t asize) {
	char *head;
	char *tmp;
	if (asize <= 40+DSIZE) {
		while (asize <= 40+DSIZE) {
			head = (char*)GET(PTR_LIST(ptr_begin,asize));
			if(head != NULL) {
				while((tmp = NEXT_PTR(head)) != NULL) head = tmp;
				return (void*)head;
			}
			else
				asize += DSIZE;
		}
		
	}

	head = (char*)GET(ptr_big);
	if (head == NULL) return NULL;
	if (GET_SIZE(HDRP(find_biggest(head))) < asize) return NULL;
	while(1) {
		if (GET_SIZE(HDRP(head)) > asize)
			if(LEFT_PTR(head) == NULL) break;
			else
				if(GET_SIZE(HDRP(find_biggest(LEFT_PTR(head)))) >= asize)
					head = LEFT_PTR(head);
				else break;
		else if (GET_SIZE(HDRP(head)) < asize)
			if(RIGHT_PTR(head) == NULL) 
				break;
			else
				if(GET_SIZE(HDRP(find_biggest(RIGHT_PTR(head)))) >= asize)
					head = RIGHT_PTR(head);
				else break;
		else 
			break;
	}
	while((tmp = NEXT_PTR(head)) != NULL) head = tmp;
	return (void*)head;
		
}


/*
 * find_precise - find the precise size free block fit 
 */
void *find_precise(size_t asize) {
	char *head = (char*)GET(ptr_big);
	size_t tmp;
	while(head != NULL) {
		tmp = GET_SIZE(HDRP(head));
		if (tmp < asize)
			head = RIGHT_PTR(head);
		else if (tmp > asize)
			head = LEFT_PTR(head);
		else
			break;
	}
	return (void*)head;
}

/* 
 * coalesce - coalesce the free blocks
 */
void* coalesce(void *bp) {
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	char* prev_b = PREV_BLKP(bp);
	char* next_b = NEXT_BLKP(bp);

	if (prev_alloc && next_alloc) {/*case 1 */
		add_block(bp);
		return bp;
	}

	else if(prev_alloc && !next_alloc) { /*case 2 */
		delete_block(next_b);
		size += GET_SIZE(HDRP(next_b));
		PUT(HDRP(bp),PACK(size,0));
		PUT(FTRP(bp),PACK(size,0));
		add_block(bp);
	}

	else if(!prev_alloc && next_alloc) { /*case 3 */
		delete_block(prev_b);
		size += GET_SIZE(HDRP(prev_b));
		PUT(FTRP(bp),PACK(size,0));
		PUT(HDRP(prev_b),PACK(size,0)); //////////////////
		bp = prev_b;
		add_block(bp);
	}

	else { 								/*case  4*/
		delete_block(prev_b);
		delete_block(next_b);
		size += GET_SIZE(HDRP(prev_b)) + GET_SIZE(HDRP(next_b));
		PUT(HDRP(prev_b),PACK(size,0));
		PUT(FTRP(next_b),PACK(size,0));
		bp = prev_b;
		add_block(bp);
	}
	return (void*)bp;
}
/* 
 * delete a free block
 */
void delete_block(void *bp) {
	size_t size = GET_SIZE(HDRP(bp));
	char *hd;
	if (size > 40+DSIZE) { 
		hd = find_precise(size);
		if (NEXT_PTR(hd) == NULL) {
			delete_block_list(hd); /*List in tree has only one block*/
		}
		else {
			delete_free_block(&hd,bp);
		}
	}else {
		hd = PTR_LIST(ptr_begin,size);
		delete_free_block(hd,bp);
	}
}
/* 
 * add a free block
 */
void add_block(void *bp) {
	size_t size = GET_SIZE(HDRP(bp));
	char *hd;

	if (size > 40+DSIZE) { /*add the free block to tree or small list*/
		hd = find_precise(size);
		if (hd == NULL) 
			add_block_list(bp);
		else
			add_free_block(&hd,bp);
		}
	else {
		hd = PTR_LIST(ptr_begin,size);
		if (hd == NULL) {
			fprintf(stderr, "Addressing small block lists error! 359\n");
			return;
		}
		add_free_block(hd,bp);
	}
}
/* 
 * add free block to double linked list
 */
void add_free_block(void *fl, void *bp) {
	char* head = (char*)GET(fl);
	char* tmp;

	if(head == NULL) {
		PUT(fl,(int)bp);
		PUT(PTR_NEXT_PTR(bp),0);
		PUT(PTR_PREV_PTR(bp),0);
		return;
	}
	while((tmp = NEXT_PTR(head)) != NULL) head = tmp;
	PUT(PTR_NEXT_PTR(head),(int)bp);
	PUT(PTR_NEXT_PTR(bp),0);
	PUT(PTR_PREV_PTR(bp),(int)head);
}

/* 
 * delete free block to linked list
 */
void delete_free_block(void *fl, void *bp) {
	//printf("407 delete 0x%x\n",(int)bp);
	char* head = (char*)GET(fl);
	char* parent;
	char* tmp;
	char* bp_next = NEXT_PTR(bp);
	char* bp_prev = PREV_PTR(bp);


	if(head == (char*)bp) {
		if (GET_SIZE(HDRP(bp))>48 && bp_prev == NULL) {
			parent = (char*)find_parent(bp);
			if(parent == ptr_big) {
				PUT(parent,bp_next);
			} else {
				if(GET_SIZE(HDRP(bp)) > GET_SIZE(HDRP(parent)))
					PUT(PTR_RIGHT_PTR(parent),bp_next);
				else if(GET_SIZE(HDRP(bp)) < GET_SIZE(HDRP(parent)))
					PUT(PTR_LEFT_PTR(parent),bp_next);

			}
			PUT(PTR_LEFT_PTR(bp_next),LEFT_PTR(bp));
			PUT(PTR_RIGHT_PTR(bp_next),RIGHT_PTR(bp));
			PUT(PTR_LEFT_PTR(bp),0);
			PUT(PTR_RIGHT_PTR(bp),0);
		}
		PUT(fl,bp_next);
		if (bp_next != NULL)
			PUT(PTR_PREV_PTR(bp_next),0);
		PUT(PTR_NEXT_PTR(bp),0);
		PUT(PTR_PREV_PTR(bp),0);
		return;
	}
	 while((tmp = NEXT_PTR(head)) != (char*)bp) {
	 	head = tmp;
	 }
	PUT(PTR_NEXT_PTR(head),bp_next);
	if (bp_next != NULL)
		PUT(PTR_PREV_PTR(bp_next),head);
	PUT(PTR_NEXT_PTR(bp),0);
	PUT(PTR_PREV_PTR(bp),0);
}

/* 
 * add free block list to big blocks tree 
 */
void add_block_list(void *bp) {
	char *head = ptr_big;
	if(GET(head) == 0){
		PUT(head,(int)bp);
	}
	else {
		head = (char*)GET(head);
		while(1) {
			if(GET_SIZE(HDRP(head)) > GET_SIZE(HDRP(bp)))
				if(LEFT_PTR(head) == NULL) {
					PUT(PTR_LEFT_PTR(head),(int)bp);
					break;
				}
				else
					head = LEFT_PTR(head);
			else
				if(RIGHT_PTR(head) == NULL) {
					PUT(PTR_RIGHT_PTR(head),(int)bp);
					break;
				}
				else
					head = RIGHT_PTR(head);
		}
	}
	PUT(PTR_NEXT_PTR(bp),0);
	PUT(PTR_PREV_PTR(bp),0);
	PUT(PTR_LEFT_PTR(bp),0);
	PUT(PTR_RIGHT_PTR(bp),0);
}

/* 
 * delete free block list to big blocks tree 
 */
void delete_block_list(void *bp) {
	char *head = (char*)ptr_big;
	char *de_parent = head;
	char *mm;
	//char *m_parent;
	int delete_head_flag = (GET(head) == (int)bp);

	head = (char*)GET(head);
	while(1){  /*Find the node to be deleted and its parent node*/
		if(GET_SIZE(HDRP(head)) > GET_SIZE(HDRP(bp))) {
				de_parent = head;
				head = LEFT_PTR(head);
			}
		else if (GET_SIZE(HDRP(head)) < GET_SIZE(HDRP(bp))) {
				de_parent = head;
				head = RIGHT_PTR(head);
			}
		else
			break;
	}

	if (LEFT_PTR(head) == NULL && RIGHT_PTR(head) == NULL) { /* delete leaf node*/
		if (delete_head_flag) {
			PUT(ptr_big,0);
			return;
		}

		if (LEFT_PTR(de_parent) == head)
			PUT(PTR_LEFT_PTR(de_parent), 0);
		else
			PUT(PTR_RIGHT_PTR(de_parent), 0);
	} 

	else if (LEFT_PTR(head) == NULL) {  /* deleted node has right child*/
		if (delete_head_flag) {
			PUT(ptr_big,(int)RIGHT_PTR(head));
			PUT(PTR_RIGHT_PTR(head),0);
			return;
		}
	
		if (LEFT_PTR(de_parent) == head)
			PUT(PTR_LEFT_PTR(de_parent), (int)RIGHT_PTR(head));
		else
			PUT(PTR_RIGHT_PTR(de_parent), (int)RIGHT_PTR(head));
	} 

	else if (RIGHT_PTR(head) == NULL) { /*deleted node has left child*/
		if (delete_head_flag) {
			PUT(ptr_big,(int)LEFT_PTR(head));
			PUT(PTR_LEFT_PTR(head),0);
			return;
		}

		if (LEFT_PTR(de_parent) == head)
			PUT(PTR_LEFT_PTR(de_parent), (int)LEFT_PTR(head));
		else
			PUT(PTR_RIGHT_PTR(de_parent), (int)LEFT_PTR(head));
	}

	else {  /* delete node has both left and right child*/
		mm = (char*)find_biggest(LEFT_PTR(head));
		delete_block_list(mm);

		if (delete_head_flag) {
			PUT(ptr_big,(int)mm);
		}
		else {
			if (LEFT_PTR(de_parent) == head)
				PUT(PTR_LEFT_PTR(de_parent), (int)mm);
			else
				PUT(PTR_RIGHT_PTR(de_parent), (int)mm);
		}

		if (LEFT_PTR(head) != mm)
			PUT(PTR_LEFT_PTR(mm), (int)LEFT_PTR(head));
		PUT(PTR_RIGHT_PTR(mm), (int)RIGHT_PTR(head));

		PUT(PTR_LEFT_PTR(head),0);
		PUT(PTR_RIGHT_PTR(head),0);
	}

}


/*
 * find_biggest - find the biggest free block of a tree
 */
void *find_biggest(void* hd) {
    char *head = (char*)hd;
    char *tmp;
    while((tmp = RIGHT_PTR(head)) != NULL) {
    	head = tmp;
    }
	return (void*)head;
}

/*
 * find_parent - find the parent node in tree
 */
void *find_parent(void* hd) {
    char *head = ptr_big;
    if (GET(head) == (unsigned int)hd)
		return (void*)head;
	head = (char*)GET(ptr_big);
	while(head != NULL) {
		if(GET_SIZE(HDRP(hd)) > GET_SIZE(HDRP(head)))
			if(RIGHT_PTR(head) == (char*)hd)
				return (void*)head;
			else
				head = RIGHT_PTR(head);
		else
			if(LEFT_PTR(head) == (char*)hd)
				return (void*)head;
			else
				head = LEFT_PTR(head);
	}
	printf("the node you want to know its parent node is not in tree\n");
	print_tree(ptr_big);
	return NULL;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    char *next_b = NEXT_BLKP(ptr);
    size_t copySize;
    size_t oldsize = GET_SIZE(HDRP(oldptr));
    size_t newsize = ALIGN(size + DSIZE);
    size_t freesize = oldsize;
    size_t next_alloc = GET_ALLOC(HDRP(next_b));

    if(oldsize == size + 8) return oldptr;
    if(!next_alloc) freesize += GET_SIZE(HDRP(next_b));
    if(freesize >= newsize + 2*DSIZE) {
    	if(!next_alloc)
    		delete_block(next_b);
    	PUT(HDRP(oldptr),PACK(newsize,1));
		PUT(FTRP(oldptr),PACK(newsize,1));
		newptr = oldptr;
		oldptr = NEXT_BLKP(oldptr);
		PUT(HDRP(oldptr),PACK(freesize-newsize,0));
		PUT(FTRP(oldptr),PACK(freesize-newsize,0));
		add_block(oldptr);
    }
	else if(freesize >= newsize) {
		if(!next_alloc)
    		delete_block(next_b);
		PUT(HDRP(oldptr),PACK(freesize,1));
		PUT(FTRP(oldptr),PACK(freesize,1));
		newptr = oldptr;
	}
	else {
	    newptr = mm_malloc(size);
	    if (newptr == NULL)
	      return NULL;
	    copySize = oldsize-DSIZE;
	    if (size < copySize)
	      copySize = size;
	    memcpy(newptr, oldptr, copySize);
	    mm_free(oldptr);
	}
    return newptr;
}
/*
 * check whether the heap is consistent with block lists
 */
int mm_check(void) {
	return 0;
}

/*
 * get through free lists from ptr to 
 check whether all free block listed in are marked as free;
 mark every free block listed in lists

 */
void check_free_block() {
	char* block_head = ptr_begin + DSIZE;
	while (block_head < (char*)mem_heap_hi()) {
		if(GET_ALLOC(HDRP(block_head)) == 0)
			check_list_in(block_head);
		if(GET_ALLOC(HDRP(block_head)) == 0 && GET_ALLOC(HDRP(NEXT_BLKP(block_head))) == 0){
			printf("free block in 0x%x(%u) and its next block in 0x%x(%u) are both free.\n",(unsigned int)block_head,GET_SIZE(HDRP(block_head)),(unsigned int)NEXT_BLKP(block_head),GET_SIZE(HDRP(NEXT_BLKP(block_head))));
			PUT(0,1); 
		}
		block_head = NEXT_BLKP(block_head);
	}
}



/*
 * print all blocks
 */
void print_block() {
	char* block_head = ptr_begin + DSIZE;
	while (block_head < (char*)mem_heap_hi()) {
		//if(GET_ALLOC(HDRP(block_head)) == 0)
			printf("0x%x %u %u\n", (int)block_head,GET_ALLOC(HDRP(block_head)), GET_SIZE(HDRP(block_head)));
		block_head = NEXT_BLKP(block_head);
	}
}
/*
 *print the big free tree
 */
void print_tree(void* head) {
	char* block_head;
	if((char*)head == ptr_big)
	    block_head = (char*)GET(head);
	else
		block_head = (char*)head;

	if (block_head != NULL) {
		printf("block list of %u size in 0x%x\n",GET_SIZE(HDRP(block_head)),(int)block_head);
		print_tree(LEFT_PTR(block_head));
		print_tree(RIGHT_PTR(block_head));
	}
}
/* check whether a free block is recorded
 */
void check_list_in(void* bp) {
	size_t size = GET_SIZE(HDRP(bp));
	char *head = find_precise(size);
	if(size <= 48) 
		head = (char*)GET(head);
	while(head != NULL && head != bp) head = NEXT_PTR(head);
	if(head == NULL){
		print_block();
		print_tree(ptr_big);
		printf("block  0x%x is not in %u size list\n",(int)bp,size);
		PUT(0,1);
	}
	return;

}





