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
    "Risc_lt",
    /* First member's full name */
    "Ruan Letian",
    /* First member's email address */
    "Ruan_lt@outlook.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
    Begin: Selfdefined Macro
*/

/* Basic constants and macros */
#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/*
    End: Selfdefined Macro
*/

/*
    Selfdefined function declaraiton
*/
static void *extend_heap(size_t words);
static void *coalesce(void *ptr);
static void *find_fit(size_t asize);
static void place(void *ptr, size_t asize);

int mm_check(void);

static char *heap_listp = 0;


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{   // Create the initial empty heap 
    heap_listp = mem_sbrk(4 * WSIZE);

    if (heap_listp == (void *)-1) {
        return -1;
    }

    // use 4 blocks of 1 byte to initialize the heap
    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2 * WSIZE);

    // Extend the empty heap with a free block of CHUNKSIZE bytes
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }

    return 0;
}

static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    // Allocate an even number of words to maintain alignment
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) return NULL;

    // Initialize free block header/footer and the epilogue header
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header*/

    return coalesce(bp);
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) { 
    size_t asize; /* Adjust block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *ptr;

    // Ignore spurious requests
    if(size == 0){
        return NULL;
    }

    // Adjust block size to include overhead and alignment part.
    if(size <= DSIZE){
        asize = 2 * DSIZE; // at least 16 bytes
    } else {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE); // around up to the smallest multiple of 8 bytes
    }

    /* Search the free list for a fit */
    if((ptr = find_fit(asize)) != NULL) {
        place(ptr, asize);
        return ptr;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if((ptr = extend_heap(extendsize/WSIZE)) == NULL) {
        return NULL;
    }
    place(ptr, asize);
    return ptr;
}

static void *find_fit(size_t asize) {
    void *ptr;
    void *res = NULL;
    size_t min_size = 0xffffffff;

    // traverse the heap
    for (ptr = heap_listp; GET_SIZE(HDRP(ptr)) > 0; ptr = NEXT_BLKP(ptr)) { 
        // skip the allocated block
        if (GET_ALLOC(HDRP(ptr))) {
            continue; 
        }

        // find the smallest but fitted free block
        size_t block_size = GET_SIZE(HDRP(ptr));
        
        if (block_size >= asize && block_size < min_size) { 
            min_size = block_size;
            res = ptr;
        }
    }
    return res; // if not found, return NULL (then extend the heap)
}


/*
    If the difference between the size of a free block and the size of the space to be allocated 
    exceeds the minimum required size for a free block, it is necessary to do a truncation
    otherwise the whole block is directly recorded as allocated
*/
static void place(void* ptr, size_t asize){
    size_t block_size = GET_SIZE(HDRP(ptr));

    if((block_size - asize) >= (2 * DSIZE)){ 
        // Truncation
        PUT(HDRP(ptr), PACK(asize, 1)); 
        PUT(FTRP(ptr), PACK(asize, 1)); 

        ptr = NEXT_BLKP(ptr); // Finding the next free block

        // Modify the size to be the difference between asize and the original size
        PUT(HDRP(ptr), PACK(block_size - asize, 0)); 
        PUT(FTRP(ptr), PACK(block_size - asize, 0));
    } else { 
        // no need for truncation
        PUT(HDRP(ptr), PACK(block_size, 1));
        PUT(FTRP(ptr), PACK(block_size, 1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) { 
    size_t size = GET_SIZE(HDRP(ptr));

    // set the valid bit of both head and foot to 0
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    coalesce(ptr);
}

static void *coalesce(void *ptr) {
    // Get the valid bits for the prev and next block
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));

    // Init the size of the freed block with current block size and add subsequent blocks if needed
    size_t size = GET_SIZE(HDRP(ptr));

    if (prev_alloc && next_alloc) { /* Case 1 */
        // No adjacent free blocks 
        return ptr;  
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        // prev: occupied, next: free 
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));  // update the block size

        PUT(HDRP(ptr), PACK(size, 0));  // current head
        PUT(FTRP(ptr), PACK(size, 0));  // next foot
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        // prev: free, next: occupied
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));  // update the block size

        PUT(FTRP(ptr), PACK(size, 0));  //current foot
        PUT(HDRP(PREV_BLKP(ptr)),PACK(size, 0));  // prev head
        
        ptr = PREV_BLKP(ptr);  // ptr is pointed to the prev head as the new big block
    }

    else { /* Case 4 */
        // all adjacent blocks are free 
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(FTRP(NEXT_BLKP(ptr)));  // update the block size

        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));  // prev head
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));  // next foot

        ptr = PREV_BLKP(ptr);  // ptr is pointed to the prev head as the new big block
    }
    
    return ptr;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    size_t oldsize;
    void *newptr;

    // If size = 0 then this it is equal to mm_free()
    if (size == 0) {
        mm_free(ptr);
        return 0;
    }

    // If oldptr is NULL, then it is equal to mm_malloc()
    if (ptr == NULL) {
        return mm_malloc(size);
    }

    newptr = mm_malloc(size);

    // If realloc() fails, break with no change
    if (!newptr) {
        return 0;
    }

    // Copy the previous data and paste
    oldsize = GET_SIZE(HDRP(ptr)) - DSIZE;
    if (size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);

    return newptr;
}

/*
    mm_check: 4 parts to check
            1. Traversing from the head of heap memory to the tail
            2. The payload of each block is aligned to 8 bytes.
            3. Header and footer information is consistent for each block
            4. There are no two consecutive free memory blocks
    This should be commented out to avoid affecting performance while being tested.
*/
// int mm_check(void) {
//     void *ptr, *prev_ptr;
//     size_t prev_alloc = 1;
//     for (ptr = heap_listp; GET_SIZE(HDRP(ptr)) > 0; ptr = NEXT_BLKP(ptr)) {
//         if ((size_t)(ptr) & 0x7) {  // check alighment
//             printf("Block pointer %p not aligned.\n", ptr);
//             return 0;
//         }

//         size_t header = GET(HDRP(ptr));
//         size_t footer = GET(FTRP(ptr));

//         if(header != footer) { // check consistency of header and footer information 
//             printf("Block %p data corrupted, header and footer don't match.\n", ptr);
//             return 0;
//         }

//         size_t cur_alloc = GET_ALLOC(HDRP(ptr));

//         if (!prev_alloc && !cur_alloc) {  // check if any adjacent free block
//             printf("Contiguous free blocks detected starting from %p.\n", prev_ptr);
//             return 0;
//         }

//         prev_alloc = cur_alloc;
//         prev_ptr = ptr;
//     }
//     if (ptr != mem_heap_hi() + 1) {  // Check if the requested heap memory has been used up
//         printf("Blocks not using up the memory?\n");
//         printf("Epilogue ptr: %p, Heap high ptr: %p\n", ptr, mem_heap_hi() + 1);
//         printf("Heap size: %zu\n", mem_heapsize());
//         return 0;
//     }
//     return 1;
// }