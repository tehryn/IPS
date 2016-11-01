// xpumr00
/**
 * @file :	tmal.c
 * @date :	14-10-2016
 *
 * Source file for IPS project 1
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "tmal.h"

#ifdef DEBUG
#define DEBUG(...)											\
do {														\
	fprintf(stderr, __FILE__":%u:"__func__, __LINE__);		\
	fprintf(stderr, __VA_ARGS__);							\
} while ( 0 );
#else
#define DEBUG(...)
#endif


/**
 * Global base pointer to block tables. Thread index is the index to blk_table.
 */
struct blk_array_info_t *blks_table;

/**
 * Allocate sparse table of blocks for several threads.
 * @param  nthreads     number of threads/items in the table
 * @return              pointer to the first block array, NULL = failed
 */
struct blk_array_info_t *
tal_alloc_blks_table(unsigned nthreads)
{
	// Try to create new table for nthreads
	struct blk_array_info_t * newTable = NULL;
	blks_table = (struct blk_array_info_t *) malloc(nthreads * sizeof(struct blk_array_info_t));
	if (newTable == NULL)
	{
		return NULL;
	}

	// Initialzie new table
	for (unsigned i = 0; i < nthreads; i++)
	{
		blks_table[i].blk_info_arr = NULL;
		blks_table[i].nblks = 0;
		blks_table[i].heap_size = 0
	}
	
	return blks_table;
}

/**
 * Allocates and initialize array of blocks.
 * @param  tid      thread index.
 * @param  nblks    capacity in number of blocks in the array.
 * @param  theap    heap capacity for a given thread.
 * @return          pointer to the first block in an array.
 */
struct blk_info_t *tal_init_blks(unsigned tid, unsigned nblks, size_t theap)
{
	// Entry checks
	if (nblks == 0 || theap == 0)
	{
		goto ERR_ARGS;
	}
	// Allocate block array
	blks_table[tid].blk_info_arr = (struct blk_info_t *) malloc(nblks * sizeof(struct blk_info_t));
	if (blks_table[tid].blk_info_arr == NULL)
	{
		goto ERR_MALLOC_BLK_INFO_ARR;
	}
	// ??? SHOULD WE ALLOCATE HEAP FOR THREAD HERE TOO -> Yes
	// Allocate heap for one thread
	// At the beginnin just one block should be active, so first block will be used as heap holder
	blks_table[tid].blk_info_arr[0].ptr = (void *) malloc(theap);
	if (blks_table[tid].blk_info_arr[0].ptr == NULL)
	{
		goto ERR_MALLOC_HEAP;
	}
	
	// Initialize rest of blk_info_arr_t
	blks_table[tid].nblks		= nblks;
	blks_table[tid].heap_size	= theap;

	// Initialize holders of the heap blocks
	// Initialize rest of first heap holder
	blks_table[tid].blk_info_arr[0].size		= theap;
	blks_table[tid].blk_info_arr[0].prev_idx	= -1;
	blks_table[tid].blk_info_arr[0].next_idx	= -1;
	blks_table[tid].blk_info_arr[0].used		= false;		// We just holding heap, not using it yet
	
	// Initialize rest of holders
	for (unsigned i = 1; i < nblks; i++)
	{
		// All block holders should be deactivated at the beginning except of first one
		// !!! For all holders [prev|next]_idx = -1
		blks_table[tid].blk_info_arr[i].ptr			= NULL;
		blks_table[tid].blk_info_arr[i].size		= 0;
		blks_table[tid].blk_info_arr[i].prev_idx	= -1;
		blks_table[tid].blk_info_arr[i].next_idx	= -1;
		blks_table[tid].blk_info_arr[i].used		= false;
	}

	// !!! TODO Test just for this number of blocks : 1, 2
	return blks_table[tid].blk_info_arr;
	
	// Handle unexpected errors	
ERR_MALLOC_HEAP:
	free(blks_table[tid].blk_info_arr);
	blks_table[tid].blk_info_arr = NULL;
	
ERR_MALLOC_BLK_INFO_ARR:
ERR_ARGS:
	return NULL;
}

/**
 * Splits one block into two.
 * @param tid       thread index
 * @param blk_idx   index of the block to be split
 * @param req_size  requested size of the block
 * @return          index of a new block created as remainder.
 */
int tal_blk_split(unsigned tid, int blk_idx, size_t req_size)
{
	// Should we split only used blocks? Probably not ...
	// Is split even possible?
	// Better solution would be keep information about block in structure ...
	// sad is, that here is not such structure ... so, scan it ...
	unsigned freeBlock = 0;

	for (freeBlock = 0; freeBlock < blks_table[tid].nblks; freeBlock++)
	{
		if (blks_table[tid].blk_info_arr[freeBlock].ptr == NULL && blks_table[tid].blk_info_arr[freeBlock].used == false)
		{
			break;
		}
	}

	// Split is not possible
	if (blks_table[tid].nblks == freeBlock)
	{
		return -1;
	}

	// Heap holder will will holds end of splitted memory was founded
	// Bind both holders now ...
	blks_table[tid].blk_info_arr[freeBlock].ptr			= (void *)((char *) blks_table[tid].blk_info_arr[blk_idx].ptr + req_size);
	blks_table[tid].blk_info_arr[freeBlock].size		= blks_table[tid].blk_info_arr[blk_idx].size - req_size;
/*
	blks_table[tid].blk_info_arr[freeBlock].prev_idx 	= blk_idx;
	blks_table[tid].blk_info_arr[freeBlock].next_idx 	= blks_table[tid].blk_info_arr[blk_idx].next_idx;
*/
	blks_table[tid].blk_info_arr[freeBlock].used		= false;

	blks_table[tid].blk_info_arr[blk_idx].size			= req_size;
	blks_table[tid].blk_info_arr[blk_idx].next_idx		= freeBlock;

	/// ??? REQ_SIZE size of which block? splited block or newly created block ??? - probably newly created block
	return freeBlock;
}

/**
 * Merge two blocks in the block list/array.
 * @param tid       thread index
 * @param left_idx  index of the left block
 * @param right_idx index of the right block
 */
void tal_blk_merge(unsigned tid, int left_idx, int right_idx)
{
	// Check arguments
	if (left_idx < 0 || right_idx < 0 || left_idx == right_idx || (unsigned) left_idx > blks_table[tid].nblks -1 || (unsigned) right_idx > blks_table[tid].nblks -1)
	{
		return;
	}
	
	// Attach right block to the left block
	blks_table[tid].blk_info_arr[left_idx].size += blks_table[tid].blk_info_arr[right_idx].size;
	blks_table[tid].blk_info_arr[left_idx].next_idx = blks_table[tid].blk_info_arr[right_idx].next_idx;

	// Detach right block
	blks_table[tid].blk_info_arr[right_idx].ptr = NULL;
	blks_table[tid].blk_info_arr[right_idx].size = 0;
	blks_table[tid].blk_info_arr[right_idx].prev_idx = -1;
	blks_table[tid].blk_info_arr[right_idx].next_idx = -1;
	blks_table[tid].blk_info_arr[right_idx].used = false;
}

/**
 * Allocate memory for a given thread. Note that allocated memory will be
 * aligned to sizeof(size_t) bytes.
 * @param  tid  thread index (in the blocks table)
 * @param  size requested allocated size
 * @return      pointer to allocated space, NULL = failed
 */
void *tal_alloc(unsigned tid, size_t size)
{
	// Is allocation even possible? Find first free heap holder
	bool found = false;			// There is no free heap holder
	unsigned index = 0;

	// Best fit
	/*
	unsigned indexNew = 0;		// Index of newly assigned block (this block has better size than old block)
	unsigned indexOld = 0;		// Index of old block
	for (indexOld = 0; indexOld < blks_table[tid].nblks; indexOld++)
	{
		// We found first free index of block with enough size
		if (blks_table[tid].blk_info_arr[indexOld].used == false)
		{
			if (blks_table[tid].blk_info_arr[indexOld].size >= size)
			{
				// Use differnt heap holder, if it fits better
				if (indexNew == 0)
				{
					found = true;
					indexNew = indexOld;
				}
				else
				{
					if (blks_table[tid].blk_info_arr[indexOld].size < blks_table[tid].blk_info_arr[indexNew].size)
					{
						indexNew = indexOld;	
					}
				}
			}
		}
	}
	*/

	// First fit
	for (index = 0; index < blks_table[tid].nblks; index++)
	{
		if (blks_table[tid].blk_info_arr[index].used == false && blks_table[tid].blk_info_arr[index].size >= size)
		{
			found = true;
			break;
		}
	}
	
	// There is no free heap holder
	if (found == false)
	{
		return NULL;
	}

	//	int tal_blk_split(unsigned tid, int blk_idx, size_t req_size)
	size_t size_tmp = size;
	while (size_tmp%8) {
		size_tmp++;
	}

	tal_blk_split(tid, index, size_tmp);
	blks_table[tid].blk_info_arr[index].used = true;
	blks_table[tid].blk_info_arr[index].next_idx = -1;
	blks_table[tid].blk_info_arr[index].prev_idx = -1;
	// Find prev_idx
	for (int i = index -1; i >= 0; i--)
	{
		if (blks_table[tid].blk_info_arr[i].ptr != NULL)
		{
			blks_table[tid].blk_info_arr[i].next_idx = index;
			blks_table[tid].blk_info_arr[index].prev_idx = i;
			break;
		}
	}
	// Find next_idx
	for (unsigned i = index +1; i < blks_table[tid].nblks; i++)
	{
		if (blks_table[tid].blk_info_arr[i].ptr != NULL)
		{
			blks_table[tid].blk_info_arr[i].prev_idx = index;
			blks_table[tid].blk_info_arr[index].next_idx = i;
			break;
		}
	}
	
	return (void *) blks_table[tid].blk_info_arr[index].ptr;
}

/**
 * Free memory for a given thread.
 * @param tid   thread index
 * @param ptr   pointer to memory allocated by tal_alloc or tal_realloc.
 *              NULL = do nothing.
 */
void tal_free(unsigned tid, void *ptr)
{
	if (ptr == NULL)
	{
		return;
	}

	// Find index for void *ptr from argument
	unsigned freeIdx = 0;
	for (freeIdx = 0; freeIdx < blks_table[tid].nblks; freeIdx++)
	{
		if (blks_table[tid].blk_info_arr[freeIdx].ptr == (const void *) ptr)
		{
			break;
		}
	}
	if (freeIdx >= blks_table[tid].nblks)
	{
		return;
	}
	
	// Merge if possible (right left)
	int tmpVar = blks_table[tid].blk_info_arr[freeIdx].next_idx;
	if (tmpVar != -1 && blks_table[tid].blk_info_arr[tmpVar].used == false)
	{
		tal_blk_merge(tid, freeIdx, tmpVar);
	}
	// (look left)
	tmpVar = blks_table[tid].blk_info_arr[freeIdx].prev_idx;
	if (tmpVar != -1 && blks_table[tid].blk_info_arr[tmpVar].used == false)
	{
		tal_blk_merge(tid, tmpVar, freeIdx);
	}

	// Detach heap holder
/*	if (blks_table[tid].blk_info_arr[freeIdx].prev_idx != -1)
	{
		blks_table[tid].blk_info_arr[blks_table[tid].blk_info_arr[freeIdx].prev_idx].next_idx = blks_table[tid].blk_info_arr[freeIdx].next_idx;
	}
	if (blks_table[tid].blk_info_arr[freeIdx].next_idx != -1)
	{
		blks_table[tid].blk_info_arr[blks_table[tid].blk_info_arr[freeIdx].next_idx].prev_idx = blks_table[tid].blk_info_arr[freeIdx].prev_idx;
	}*/
	blks_table[tid].blk_info_arr[freeIdx].used = false;
}

/**
 * Realloc memory for a given thread.
 * @param tid   thread index
 * @param ptr   pointer to allocated memory, NULL = allocate a new memory.
 * @param size  a new requested size (may be smaller than already allocated),
 *              0 = equivalent to free the allocated memory.
 * @return      pointer to reallocated space, NULL = failed.
 */
void *tal_realloc(unsigned tid, void *ptr, size_t size)
{
	// Try to allocate new space
	void * newPtr = tal_alloc(tid, size);
	if (newPtr == NULL)
	{
		return NULL;
	}

	if (ptr == NULL)
	{
		return newPtr;
	}
	
	// If reallocation was successful, move old array
	memcpy(newPtr, (const void *) ptr, size);
	
	// Release old memory
	tal_free(tid, ptr);
	
	return newPtr;
}
