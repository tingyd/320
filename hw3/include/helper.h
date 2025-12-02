#include "sfmm.h"

/* Constants for the allocator */
#define HEADER_SIZE 8
#define FOOTER_SIZE 8
#define MIN_BLOCK_SIZE 32
#define ALIGN_SIZE 16

/* Bit masks */
#define THIS_BLOCK_ALLOCATED  0x1
#define IN_QUICK_LIST         0x2
#define PREV_BLOCK_ALLOCATED  0x4

#define BLOCK_SIZE_MASK       (~0xF)
#define ALLOC_MASK            0xF
#define REQUESTED_SIZE_SHIFT  32

size_t max_payload = 0;
size_t current_payload = 0;
size_t  current_heap_size = 0;

void initialize_heap();
int get_free_list_index(size_t size) ;
size_t calculate_block_size(size_t requested_size);
size_t total_requested_payload = 0; 
size_t total_allocated_blocks_size = 0;
sf_block *search_free_lists(size_t size);
sf_block *extend_heap(size_t size);
sf_block *coalesce(sf_block *block);
void split_block(sf_block *block, size_t asize);
void insert_into_free_list(sf_block *block, int index);
void remove_from_free_list(sf_block *block);
void add_to_quick_list(sf_block *block,int qlist);
sf_block *find_quick_list_block(size_t size);
int validate_pointer(void *bp);
sf_block *get_block_from_payload(void *bp);
sf_footer *get_footer(sf_block *block);
sf_block *get_next_block(sf_block *block);
sf_block *get_prev_block(sf_block *block);
size_t get_block_size(sf_block *block);