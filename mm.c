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
    "awesome_steminists_power_rangers_the_justice_league",
    /* First member's full name */
    "Khaled AlHosani",
    /* First member's email address */
    "kah579@nyu.edu",
    /* Second member's full name (leave blank if none) */
    "Yana Chala",
    /* Second member's email address (leave blank if none) */
    "ydc223@nyu.edu"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x,y) ((x)>(y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size)|(alloc))

/* Read and write word at adress p*/
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from adress p*/
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute adress of its header and footer p*/
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute adress of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static void *heap_listp; 
static void *extend_heap(size_t words);
static void *find_fit (size_t asize);
static void place(void *bp, size_t asize);
static void *coalesce(void *bp);
static void mm_check(char caller, void *ptr, int size);


//CHECKER SETTINGS
#define CHECK         1 /* Kill bit: Set to 0 to disable checking
                           (Checking is currently disabled through comments) */
#define CHECK_MALLOC  1 /* Check allocation operations */
#define CHECK_FREE    1 /* Check free operations */
#define CHECK_REALLOC 0 /* Check reallocation operations */
#define DISPLAY_BLOCK 1 /* Describe blocks in heap after each check */
//#define DISPLAY_LIST  1 /* Describe free blocks in lists after each check */
#define PAUSE         0 /* Pause after each check, also enables the function to
                           skip displaying mm_check messages*/
#define LINE_OFFSET   4
  
char *prologue_block;

// Variables for checking function 
int line_count; // Running count of operations performed
int skip;


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *) -1)
    	return -1;
    PUT(heap_listp, 0); /* Alignment padding*/
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue heaer*/ 
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer*/
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); /* Epilogue header*/
    heap_listp += (2*WSIZE);
    prologue_block = heap_listp + DSIZE;

    /* Extend the empty heao with a free block of CHUNKSIZE bytes*/
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;


  line_count = LINE_OFFSET;
  skip = 0;
    return 0;
}


static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocates an even number of words to maintain alignment*/
    size = ALIGN(words);
    if((bp = mem_sbrk(size)) == (void *) -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size,0)); /* Free block header*/
    PUT(FTRP(bp), PACK(size,0)); /* Free block footer*/
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0 , 1)); /* New epilogue header */

    /* Coalesce if the prev block was free */
    return coalesce(bp);
}


/* 
 * mm_malloc 
 */
void *mm_malloc(size_t size)
{
	size_t asize;		//Adjusted block size
	size_t extendsize;	//Amount to extend hea if no fit
	char *bp;
	size_t checksize = size;
    //Ignore spurious requests 
	if (size==0)
		return NULL;

	//Adjust block size to include overhead and alignment reqs
	if (size<=DSIZE)
		asize=2*DSIZE;

	else
		asize = DSIZE *((size + (DSIZE)+(DSIZE-1))/DSIZE);

	//search the free list for a fit
	if((bp=find_fit(asize))!=NULL)
	{
		place(bp,asize);
		return bp;
	}

    //No fit found. Get more memory and place the block
	extendsize=MAX(asize,CHUNKSIZE);
	if((bp=extend_heap(extendsize/WSIZE))==NULL)
		return NULL;

	place(bp,asize);

	  // Check heap for consistency
	  line_count++;
	  if (CHECK && CHECK_MALLOC) {
	    mm_check('a', bp, checksize);
	  }
	return bp;

 //Given commented out


 //    int newsize = ALIGN(size + SIZE_T_SIZE);
 //    void *p = mem_sbrk(newsize);
 //    if (p == (void *)-1)
	// return NULL;
 //    else {
 //        *(size_t *)p = size;
 //        return (void *)((char *)p + SIZE_T_SIZE);
 //    	}
}

/*
 * mm_free
 */
void mm_free(void *bp)
{
	size_t size = GET_SIZE(HDRP(bp));

	PUT(HDRP(bp) , PACK(size, 0));
	PUT(FTRP(bp) , PACK(size, 0));
	coalesce(bp);

	// Check heap for consistency
	  line_count++;
	  if (CHECK && CHECK_FREE) {
	    mm_check('f', bp, size);
	  }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
     void *oldptr = ptr;
     void *newptr;
     size_t copySize=0;
    
     newptr = mm_malloc(size);
     if (newptr == NULL)
       return NULL;
     //copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
     if (size < copySize)
       copySize = size;
     memcpy(newptr, oldptr, copySize);
     mm_free(oldptr);
     return newptr;
 }

/*
 * coalesce
 */

static void *coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(FTRP(NEXT_BLKP(bp)));

	size_t size = GET_SIZE(HDRP(bp));

	//Case 1
	if(prev_alloc && next_alloc)
	{
		return bp;
	}

	//Case 2
	else if(prev_alloc && !next_alloc)
	{
		size+=GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}

	//Case 3
	else if(!prev_alloc && next_alloc)
	{
		size+=GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
		bp = PREV_BLKP(bp);
	}

	//Case 4
	else 
	{
		size+=GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
		PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
		bp=PREV_BLKP(bp);
	}

	return bp;
}

/*
 * find_fit
 */
static void *find_fit (size_t asize)
{
	/* First-fit search */
	void *bp;
	for ( bp = heap_listp; GET_SIZE(HDRP(bp)) >0; bp = NEXT_BLKP(bp) )
	{
		if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
		{
			return bp;
		}
	}
	return (NULL); /* No fit */
}


/*
 * place
 */
static void place(void *bp, size_t asize)
{
	size_t csize = GET_SIZE(HDRP(bp));
	
	if((csize - asize) >= (2*DSIZE)) 
	{
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize-asize, 0));
		PUT(FTRP(bp), PACK(csize-asize, 0));
	}

	else
	{
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	} 
}

void mm_check(char caller, void* caller_ptr, int caller_size)
{
  int size;  // Size of block
  int alloc; // Allocation bit
  char *ptr = prologue_block + DSIZE;
  int block_count = 1;
  //int count_size;
  //int count_list;
  int loc;   // Location of block relative to first block
  int caller_loc = (char *)caller_ptr - ptr;
  //int list;
  //char *scan_ptr;
  char skip_input;
  
  if (!skip)
    printf("\n[%d] %c %d %d: Checking heap...\n",
      line_count, caller, caller_size, caller_loc);
  
  while (1) {
    loc = ptr - prologue_block - DSIZE;
    size = GET_SIZE(HDRP(ptr));
	//printf("\n\n size is %d\n\n",size);
    if (size == 0)
      break;
    
    alloc = GET_ALLOC(HDRP(ptr));
    
    // Print block information
    if (DISPLAY_BLOCK && !skip) {
      printf("%d: Block at location %d has size %d and allocation %d\n",
        block_count, loc, size, alloc);
      // if (GET_TAG(HDRP(ptr))) {
      //   printf("%d: Block at location %d is tagged\n",
      //     block_count, loc);
      // }
    }
    
    // Check consistency of size and allocation in HDRPer and FTRPer
    if (size != GET_SIZE(FTRP(ptr))) {
      printf("%d: Header size of %d does not match Footer size of %d\n",
        block_count, size, GET_SIZE(FTRP(ptr)));
    }
    if (alloc != GET_ALLOC(FTRP(ptr))) {
      printf("%d: Header allocation of %d does not match Footer allocation "
        "of %d\n", block_count, alloc, GET_ALLOC(FTRP(ptr)));
    }
    
    // // Check if free block is in the appropriate list
    // if (!alloc) {
    //   // Select segregated list
    //   list = 0;
    //   count_size = size;
    //   while ((list < LISTS - 1) && (count_size > 1)) {
    //     count_size >>= 1;
    //     list++;
    //   }
      
    //   // Check list for free block
    //   scan_ptr = free_lists[list];
    //   while ((scan_ptr != NULL) && (scan_ptr != ptr)) {
    //     scan_ptr = PRED(scan_ptr);
    //   }
    //   if (scan_ptr == NULL) {
    //     printf("%d: Free block of size %d is not in list index %d\n",
    //       block_count, size, list);
    //   }
    // }
    
    ptr = NEXT_BLKP(ptr);
    block_count++;
  }
  
  // if (!skip)
  //   printf("[%d] %c %d %d: Checking lists...\n",
  //     line_count, caller, caller_size, caller_loc);
  
  // // Check every list of free blocks for validity
  // for (list = 0; list < LISTS; list++) {
  //   ptr = free_lists[list];
  //   block_count = 1;
    
  //   while (ptr != NULL) {
  //     loc = ptr - prologue_block - DSIZE;
  //     size = GET_SIZE(HDRP(ptr));
      
  //     // Print free block information
  //     if (DISPLAY_LIST && !skip) {
  //       printf("%d %d: Free block at location %d has size %d\n",
  //         list, block_count, loc, size);
  //       if (GET_TAG(HDRP(ptr))) {
  //         printf("%d %d: Block at location %d is tagged\n",
  //           list, block_count, loc);
  //       }
  //     }
      
  //     // Check if free block is in the appropriate list
  //     count_list = 0;
  //     count_size = size;
      
  //     while ((count_list < LISTS - 1) && (count_size > 1)) {
  //       count_size >>= 1;
  //       count_list++;
  //     }
  //     if (list != count_list) {
  //       printf("%d: Free block of size %d is in list %d instead of %d\n",
  //         loc, size, list, count_list);
  //     }
      
  //     // Check validity of allocation bit in HDRPer and FTRPer
  //     if (GET_ALLOC(HDRP(ptr)) != 0) {
  //       printf("%d: Free block has an invalid HDRPer allocation of %d\n",
  //         loc, GET_ALLOC(FTRP(ptr)));
  //     }
  //     if (GET_ALLOC(FTRP(ptr)) != 0) {
  //       printf("%d: Free block has an invalid FTRPer allocation of %d\n",
  //         loc, GET_ALLOC(FTRP(ptr)));
  //     }
      
  //     ptr = PRED(ptr);
  //     block_count++;
  //   }
  // }
  
  if (!skip)
    printf("[%d] %c %d %d: Finished check\n\n",
      line_count, caller, caller_size, caller_loc);
  
  // Pause and skip function, toggled by PAUSE preprocessor directive. Skip
  // allows checker to stop pausing and printing for a number of operations.
  // However, scans are still completed and errors will still be printed.
  if (PAUSE && !skip) {
    printf("Enter number of operations to skip or press <ENTER> to continue.\n");
    
    while ((skip_input = getchar()) != '\n') {
      if ((skip_input >= '0') && (skip_input <= '9')) {
        skip = skip * 10 + (skip_input - '0');
      }
    }
    
    if (skip)
      printf("Skipping %d operations...\n", skip);
      
  } else if (PAUSE && skip) {
    skip--;
  }
  
  return;
}
