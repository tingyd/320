/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"
#include "helper.h"

// Global statistics tracking
// size_t current_payload = 0;
// size_t max_payload = 0;
// size_t current_heap_size = 0;
static inline void store_requested_size(sf_block *block, size_t requested_size) {
    *(size_t *)block->body.payload = requested_size;
}

static inline size_t get_requested_size(sf_block *block) {
    return *(size_t *)block->body.payload;
}
size_t calculate_block_size(size_t requested_size) {
    size_t block_size = HEADER_SIZE + requested_size + FOOTER_SIZE;
    
    //round up to the nearest multiple of 16
    if (block_size % ALIGN_SIZE != 0) {
        block_size = block_size + (ALIGN_SIZE - (block_size % ALIGN_SIZE));
    }
    
    //ensure minimum block size
    if (block_size < MIN_BLOCK_SIZE) {
        block_size = MIN_BLOCK_SIZE;
    }
    
    return block_size;
}

void set_allocated(sf_block *block) {
    sf_header true_header = block->header ^ MAGIC;
    size_t size = true_header & ~0xF;
    
    block->header = (size | THIS_BLOCK_ALLOCATED) ^ MAGIC;
    sf_footer *footer = (sf_footer *)((char *)block + size - 8);
    *footer = block->header;
}

void setup_prologue(void) {
    char *prologue_start = (char *)sf_mem_start() + 8; //skip first 8 bytes
    sf_block *prologue = (sf_block *)prologue_start;
    
    prologue->header = (32 | 0x1)^MAGIC; 
    sf_footer *prologue_footer = (sf_footer *)(prologue_start + 24);
    *prologue_footer = prologue->header;
}

void setup_initial_free_block(void) {
    char *prologue_start = (char *)sf_mem_start() + 8;
    char *free_block_start = prologue_start + 32; //after prologue
    
    //calculate size (page size minus prologue, unused space, and epilogue)
    size_t free_block_size = PAGE_SZ - 32 - 8 - 8;
    
    sf_block *free_block = (sf_block *)free_block_start;
    free_block->header = free_block_size ^ MAGIC; //not allocated
    sf_footer *free_footer = (sf_footer *)(free_block_start + free_block_size - 8);
    *free_footer = free_block->header;
    
    //add to appropriate free list
    int index = get_free_list_index(free_block_size);
    insert_into_free_list(free_block, index);
}

void setup_epilogue(void) {
    sf_header *epilogue = (sf_header *)((char *)sf_mem_end() - 8);
    *epilogue = 0x1 ^ MAGIC; //size 0, allocated
}

void initialize_free_list(){
    for (int i = 0; i < NUM_FREE_LISTS; i++) {
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }
}

void initialize_heap() {
    void *heap_start = sf_mem_grow();
    if (heap_start == NULL) {
        return;
    }
   
    setup_prologue();
    setup_initial_free_block();
    setup_epilogue();
    current_heap_size = PAGE_SZ;
}

int get_free_list_index(size_t size) {
    if (size <= 32) return 0;
    
    size_t min_size = 32;
    int index = 0;
    
    while (index < NUM_FREE_LISTS - 1) {
        min_size *= 2;
        index++;
        
        if (size <= min_size)
            return index;
    }
    
    return NUM_FREE_LISTS - 1;
}

sf_block *find_free_list_block(size_t size) {
    int start_index = get_free_list_index(size);
    //search each free list starting at the appropriate size class
    for (int i = start_index; i < NUM_FREE_LISTS; i++) {
        sf_block *current = sf_free_list_heads[i].body.links.next;
        //search this free list for first fit
        while (current != &sf_free_list_heads[i]) {
            sf_header true_header = current->header ^ MAGIC;
            size_t current_size = true_header & ~0xF; //mask out flags
            if (current_size >= size) {
                //remove from free list FIRST
                remove_from_free_list(current);
                
                //split if possible - only if the remainder would be at least MIN_BLOCK_SIZE
                if (current_size - size >= MIN_BLOCK_SIZE) {
                    split_block(current, size);
                }
                
                set_allocated(current);
                return current;
            }
            current = current->body.links.next;
        }
    }
    return NULL;
}

void add_to_free_list(sf_block *block) {
    sf_header true_header = block->header ^ MAGIC;
    size_t block_size = true_header & ~0xF;
    
    block->header = (true_header & ~0x3) ^ MAGIC; //clear both bits
    
    sf_footer *footer = (sf_footer *)((char *)block + block_size - sizeof(sf_footer));
    *footer = block->header;
    
    block = coalesce(block);
    
    true_header = block->header ^ MAGIC;
    block_size = true_header & ~0xF;
    
    int index = get_free_list_index(block_size);
    insert_into_free_list(block, index);
}

void insert_into_free_list(sf_block *block, int index) {
    //insert at the beginning of the list
    block->body.links.next = sf_free_list_heads[index].body.links.next;
    block->body.links.prev = &sf_free_list_heads[index];
    
    sf_free_list_heads[index].body.links.next->body.links.prev = block;
    sf_free_list_heads[index].body.links.next = block;
    
    sf_header true_header = block->header ^ MAGIC;
    block->header = (true_header & ~(THIS_BLOCK_ALLOCATED | IN_QUICK_LIST)) ^ MAGIC;
    size_t size = true_header & ~0xF;
    sf_footer *footer = (sf_footer *)((char *)block + size - 8);
    *footer = block->header;
}

void remove_from_free_list(sf_block *block) {
    block->body.links.prev->body.links.next = block->body.links.next;
    block->body.links.next->body.links.prev = block->body.links.prev;
}

void add_to_quick_list(sf_block *block, int qlist_index) {
    //keep allocation bit set but set quick list bit
    sf_header true_header = block->header ^ MAGIC;
    block->header = (true_header | 0x3) ^ MAGIC; //set both allocated and in-quick-list bits
    
    //add block to front of quick list (LIFO)
    block->body.links.next = sf_quick_lists[qlist_index].first;
    sf_quick_lists[qlist_index].first = block;
    sf_quick_lists[qlist_index].length++;
}

sf_block *find_quick_list_block(size_t size) {
    if (size > MIN_BLOCK_SIZE + (NUM_QUICK_LISTS - 1) * ALIGN_SIZE) {
        return NULL;
    }
    
    int index = (size - MIN_BLOCK_SIZE) / ALIGN_SIZE;
    
    //make sure the index is valid
    if (index >= NUM_QUICK_LISTS || sf_quick_lists[index].length == 0) {
        return NULL;
    }
    
    //get the first block from the quick list
    sf_block *block = sf_quick_lists[index].first;
    sf_quick_lists[index].first = block->body.links.next;
    sf_quick_lists[index].length--;
    
    //clear the "in quick list" bit but keep the "allocated" bit
    block->header = ((block->header ^ sf_magic()) & ~IN_QUICK_LIST) ^ sf_magic();
    
    return block;
}

sf_footer *get_footer(sf_block *block) {
    size_t block_size = (block->header ^ sf_magic()) & BLOCK_SIZE_MASK;
    return (sf_footer *)((char *)block + block_size - FOOTER_SIZE);
}

sf_block *get_next_block(sf_block *block) {
    size_t block_size = (block->header ^ sf_magic()) & BLOCK_SIZE_MASK;
    return (sf_block *)((char *)block + block_size);
}

sf_block *get_prev_block(sf_block *block) {
    sf_footer *prev_footer = (sf_footer *)((char *)block - FOOTER_SIZE);
    
    //check if we're at the beginning of the heap
    if ((char *)prev_footer <= (char *)sf_mem_start() + 8 + MIN_BLOCK_SIZE) {
        return NULL;
    }
    
    size_t prev_size = (*prev_footer ^ sf_magic()) & BLOCK_SIZE_MASK;
    
    return (sf_block *)((char *)block - prev_size);
}

sf_block *coalesce(sf_block *block) {
    size_t block_size = (block->header ^ sf_magic()) & BLOCK_SIZE_MASK;
    sf_block *next_block = get_next_block(block);
    sf_block *prev_block = get_prev_block(block);
    size_t next_alloc = (next_block->header ^ sf_magic()) & THIS_BLOCK_ALLOCATED;
    size_t prev_alloc = 1;  //default to allocated if at start of heap
    
    if (prev_block != NULL) {
        prev_alloc = (prev_block->header ^ sf_magic()) & THIS_BLOCK_ALLOCATED;
    }
    
    //case 1: both previous and next blocks are allocated
    if (prev_alloc && next_alloc) {
        return block;
    }
    
    //case 2: previous block is allocated, next block is free
    else if (prev_alloc && !next_alloc) {
        //remove next block from its free list
        remove_from_free_list(next_block);
        
        //update the current block size to include next block
        size_t next_size = (next_block->header ^ sf_magic()) & BLOCK_SIZE_MASK;
        block_size += next_size;
        block->header = (block_size) ^ sf_magic();
        
        //update the footer
        sf_footer *footer = get_footer(block);
        *footer = block->header;
    }
    
    //case 3: previous block is free, next block is allocated
    else if (!prev_alloc && next_alloc) {
        //remove previous block from its free list
        remove_from_free_list(prev_block);
        
        //update the previous block size to include current block
        size_t prev_size = (prev_block->header ^ sf_magic()) & BLOCK_SIZE_MASK;
        block_size += prev_size;
        prev_block->header = (block_size) ^ sf_magic();
        
        //update the footer
        sf_footer *footer = get_footer(prev_block);
        *footer = prev_block->header;
        
        //the coalesced block is now the previous block
        block = prev_block;
    }
    
    //case 4: both previous and next blocks are free
    else {
        //remove both blocks from their free lists
        remove_from_free_list(prev_block);
        remove_from_free_list(next_block);
        
        //update the previous block size to include current and next blocks
        size_t prev_size = (prev_block->header ^ sf_magic()) & BLOCK_SIZE_MASK;
        size_t next_size = (next_block->header ^ sf_magic()) & BLOCK_SIZE_MASK;
        block_size = prev_size + block_size + next_size;
        prev_block->header = (block_size) ^ sf_magic();
        
        //update the footer
        sf_footer *footer = get_footer(prev_block);
        *footer = prev_block->header;
        
        //the coalesced block is now the previous block
        block = prev_block;
    }
    return block;
}

sf_block *extend_heap(size_t size) {
    void *new_page = sf_mem_grow();
    if (new_page == NULL) {
        sf_errno = ENOMEM;
        return NULL;
    }

    sf_block *new_block = (sf_block *)((char *)new_page - 8);
    size_t new_size = PAGE_SZ;

    //if the block before the epilogue was free, coalesce
    sf_block *prev_block = NULL;
    sf_footer *prev_footer = (sf_footer *)((char *)new_page - 16);
    sf_header prev_footer_val = *prev_footer ^ MAGIC;

    if (!(prev_footer_val & ALLOC_MASK)) {
        //previous block is free, coalesce
        size_t prev_size = prev_footer_val & ~0xF;
        prev_block = (sf_block *)((char *)new_page - 8 - prev_size);
        
        //remove previous block from its free list
        prev_block->body.links.prev->body.links.next = prev_block->body.links.next;
        prev_block->body.links.next->body.links.prev = prev_block->body.links.prev;
        
        new_block = prev_block;
        new_size += prev_size;
    }

    //set up new block header
    new_block->header = new_size;
    new_block->header ^= MAGIC;

    //create new epilogue
    sf_header *new_epilogue = (sf_header *)((char *)sf_mem_end() - 8);
    *new_epilogue = ALLOC_MASK; //size 0, allocated
    *new_epilogue ^= MAGIC;

    //set up footer for new block
    sf_footer *new_footer = (sf_footer *)((char *)new_block + new_size - 8);
    *new_footer = new_block->header;

    //add new block to appropriate free list
    int new_index = get_free_list_index(new_size);
    insert_into_free_list(new_block, new_index);
    return new_page;
}

void split_block(sf_block *block, size_t size) {
    sf_header true_header = block->header ^ MAGIC;
    size_t old_size = true_header & BLOCK_SIZE_MASK;
    size_t alloc_bits = true_header & ALLOC_MASK;
    
    size_t remainder_size = old_size - size;
    
    //split if remainder is at least MIN_BLOCK_SIZE
    if (remainder_size < MIN_BLOCK_SIZE) {
        return;  
    }
    
    //remainder block
    sf_block *remainder = (sf_block *)((char *)block + size);
    remainder->header = (remainder_size | PREV_BLOCK_ALLOCATED) ^ MAGIC;
    sf_footer *remainder_footer = (sf_footer *)((char *)remainder + remainder_size - 8);
    *remainder_footer = remainder->header;
    
    int index = get_free_list_index(remainder_size);
    insert_into_free_list(remainder, index);
     
    //update original block header, preserve the flags and allocate the block
    block->header = (size | (alloc_bits & ~THIS_BLOCK_ALLOCATED) | THIS_BLOCK_ALLOCATED) ^ MAGIC;
    sf_footer *original_footer = (sf_footer *)((char *)block + size - 8);
    *original_footer = block->header;
}

sf_block *search_free_lists(size_t size) {
    int index = get_free_list_index(size);
    
    //search starting from the appropriate size class
    for (int i = index; i < NUM_FREE_LISTS; i++) {
        sf_block *block = sf_free_list_heads[i].body.links.next;
        
        //traverse the free list
        while (block != &sf_free_list_heads[i]) {
            size_t block_size = (block->header ^ sf_magic()) & BLOCK_SIZE_MASK;
            
            if (block_size >= size) {
                //remove from free list
                remove_from_free_list(block);
                return block;
            }
            block = block->body.links.next;
        }
    }
    
    return NULL;  //no suitable block found
}

inline sf_block *get_block_from_payload(void *bp) {
    return (sf_block *)((char *)bp - HEADER_SIZE);
}

int validate_pointer(void *bp) {
    if (bp == NULL) {
        return 0;
    }
    
    if ((uintptr_t)bp % ALIGN_SIZE != 0) {
        return 0;
    }
    
    sf_block *block = get_block_from_payload(bp);
    
    if ((void *)block < sf_mem_start() || (void *)((char *)block + HEADER_SIZE) > sf_mem_end()) {
        return 0;
    }
    
    size_t header = block->header ^ sf_magic();
    size_t block_size = header & BLOCK_SIZE_MASK;
    
    if (block_size < MIN_BLOCK_SIZE || block_size % ALIGN_SIZE != 0) {
        return 0;
    }
    
    if (!(header & THIS_BLOCK_ALLOCATED) || (header & IN_QUICK_LIST)) {
        return 0;
    }
    
    sf_footer *footer = get_footer(block);
    if (*footer != block->header) {
        return 0;
    }
    return 1; 
}

sf_block *coalesce_with_preceding(void *new_page) {
    sf_footer *prev_footer = (sf_footer *)((char *)new_page - 16);
    sf_header prev_footer_val = *prev_footer ^ MAGIC;
    
    if (!(prev_footer_val & 0x1)) {
        size_t prev_size = prev_footer_val & ~0xF;
        sf_block *prev_block = (sf_block *)((char *)new_page - 8 - prev_size);
        
        remove_from_free_list(prev_block);
        
        size_t new_size = prev_size + PAGE_SZ;
        
        prev_block->header = new_size;
        prev_block->header ^= MAGIC;
        
        sf_footer *new_footer = (sf_footer *)((char *)prev_block + new_size - 8);
        *new_footer = prev_block->header;
        
        int index = get_free_list_index(new_size);
        insert_into_free_list(prev_block, index);
        
        return prev_block;
    } else {
        //sets up new block
        sf_block *new_block = (sf_block *)((char *)new_page - 8);
        size_t new_size = PAGE_SZ;
        new_block->header = new_size;
        new_block->header ^= MAGIC;
        sf_footer *new_footer = (sf_footer *)((char *)new_block + new_size - 8);
        *new_footer = new_block->header;
        
        int index = get_free_list_index(new_size);
        insert_into_free_list(new_block, index);
        
        return new_block;
    }
}

void *expand_heap(void) {
    void *new_page = sf_mem_grow();
    if (new_page == NULL) {
        return NULL;
    }
    
    coalesce_with_preceding(new_page);
    
    sf_header *new_epilogue = (sf_header *)((char *)sf_mem_end() - 8);
    *new_epilogue = 0x1 ^ MAGIC;
    
    current_heap_size += PAGE_SZ;
    return new_page;
}

void *sf_malloc(size_t size) {
    sf_set_magic(1231);

    if (size <= 0) {
        return NULL;
    }

    size_t block_size = calculate_block_size(size);
    
    if (sf_mem_start() == sf_mem_end()) {
        initialize_free_list();
        initialize_heap();
    }
    
    sf_block *block = find_quick_list_block(block_size);
    if (block != NULL) {        
        current_payload += size;
        if (current_payload > max_payload) {
            max_payload = current_payload;
        }
        return block->body.payload;
    }
    
    block = find_free_list_block(block_size);
    if (block != NULL) {    
        current_payload += size;
        if (current_payload > max_payload) {
            max_payload = current_payload;
        }
        return block->body.payload;
    }
    
    if (expand_heap() == NULL) {
        sf_errno = ENOMEM;
        return NULL;
    }
    return sf_malloc(size);
}

size_t get_block_size(sf_block *block) {
    sf_header true_header = block->header ^ MAGIC;
    return true_header & ~0xF; //mask out flags
}

void flush_quick_list(int qlist_index) {
    sf_block *current = sf_quick_lists[qlist_index].first;
    
    while (current != NULL) {
        sf_block *next = current->body.links.next;
        
        //clear the quick list bit but keep allocation bit set
        sf_header true_header = current->header ^ MAGIC;
        current->header = (true_header & ~0x2) ^ MAGIC;
        
        //add to free list and coalesce
        add_to_free_list(current);
        
        current = next;
    }
    
    //reset quick list
    sf_quick_lists[qlist_index].first = NULL;
    sf_quick_lists[qlist_index].length = 0;
}

bool validate_block(sf_block *block) {
    sf_header true_header = block->header ^ MAGIC;
    size_t block_size = true_header & ~0xf; 
    
    if (block_size < 32 || block_size % 16 != 0) {
        return false;
    }
    
    //check allocation bit is set and quick list bit is not set
    if (!(true_header & 0x1) || (true_header & 0x2)) {
        return false;
    }
    
    char *block_start = (char *)block;
    char *block_end = block_start + block_size;
    //check bounds
    if (block_start < (char *)sf_mem_start() + 8 || 
        block_end > (char *)sf_mem_end() - 8) {
        return false;
    }
    
    sf_footer *footer = (sf_footer *)(block_start + block_size - sizeof(sf_footer));
    sf_footer true_footer = *footer ^ MAGIC;
    
    if (true_footer != true_header) {
        return false;
    }
    
    return true;
}

void sf_free(void *bp) {
    sf_set_magic(1231);

    if (bp == NULL) {
        abort();
    }

    if ((uintptr_t)bp % 16 != 0) {
        abort();
    }
    
    sf_block *block = (sf_block *)((char *)bp - sizeof(sf_header));

    if (!validate_block(block)) {
        abort();
    }
    
    size_t block_size = get_block_size(block);
    
    // Update current payload - subtract the user's requested payload size
    // This is tricky - we need to figure out what the original requested size was
    // The payload size is block_size - header - footer = block_size - 16
    size_t payload_size = block_size - 16;
    
    if (current_payload >= payload_size) {
        current_payload -= payload_size;
    } else {
        current_payload = 0;  
    }
    
    //for small blocks, try to add to quick list
    if (block_size <= 32 + (NUM_QUICK_LISTS - 1) * 16) {
        int qlist_index = (block_size - 32) / 16;
        
        //check if quick list is full
        if (sf_quick_lists[qlist_index].length >= QUICK_LIST_MAX) {
            //flush the quick list before adding new block
            flush_quick_list(qlist_index);
        }
        
        //add block to quick list
        add_to_quick_list(block, qlist_index);
        
    } else {
        //for larger blocks, add to free list with coalescing
        add_to_free_list(block);
    }
}

void *sf_realloc(void *pp, size_t rsize) {
    sf_set_magic(1231);

    if (!validate_pointer(pp)) {
        sf_errno = EINVAL;
        return NULL;
    }
    if (rsize < 0){
        return NULL;
    }
    
    if (rsize == 0) {
        sf_free(pp);
        return NULL;
    }
    
    sf_block *block = get_block_from_payload(pp);
    size_t old_size = get_block_size(block);
    size_t old_payload_size = old_size - 16; // block size minus header and footer
    size_t new_block_size = calculate_block_size(rsize);
    
    //if new size is larger, allocate new block
    if (new_block_size > old_size) {
        void *new_pp = sf_malloc(rsize);
        if (new_pp == NULL) {
            return NULL;
        }
        
        // Copy the smaller of old payload or new requested size
        size_t copy_size = (old_payload_size < rsize) ? old_payload_size : rsize;
        memcpy(new_pp, pp, copy_size);
                
        sf_free(pp);
        return new_pp;
    }
    else {
        // Update payload tracking for realloc
        size_t old_tracked_payload = old_payload_size;
        current_payload = current_payload - old_tracked_payload + rsize;
        if (current_payload > max_payload) {
            max_payload = current_payload;
        }
        
        if (old_size - new_block_size >= MIN_BLOCK_SIZE) {
            //check next block
            sf_block *next_block = get_next_block(block);
            sf_header next_header = next_block->header ^ MAGIC;
            bool next_is_free = !(next_header & THIS_BLOCK_ALLOCATED);
            
            // Split the block
            split_block(block, new_block_size);
            
            //if next block free, coalesce with new remainder
            if (next_is_free) {
                sf_block *remainder = get_next_block(block);
                //remove both from free lists
                remove_from_free_list(remainder);
                remove_from_free_list(next_block);
                
                //combine them
                size_t remainder_size = (remainder->header ^ MAGIC) & BLOCK_SIZE_MASK;
                size_t next_size = next_header & BLOCK_SIZE_MASK;
                size_t combined_size = remainder_size + next_size;
                
                //set new size for the combined block
                remainder->header = combined_size | ((remainder->header ^ MAGIC) & PREV_BLOCK_ALLOCATED);
                remainder->header ^= MAGIC;
                sf_footer *new_footer = (sf_footer *)((char *)remainder + combined_size - FOOTER_SIZE);
                *new_footer = remainder->header;
                
                int index = get_free_list_index(combined_size);
                insert_into_free_list(remainder, index);
            }
        }  
        return pp;
    }
}

double sf_fragmentation() {
    if (sf_mem_start() == sf_mem_end()) {
        return 0.0;
    }
    
    size_t total_allocated_size = 0;
    
    char *current = (char *)sf_mem_start() + 8 + 32; //skip unused space and prologue
    char *heap_end = (char *)sf_mem_end() - 8;       //stop before epilogue
    
    //traverse all blocks in the heap
    while (current < heap_end) {
        sf_block *block = (sf_block *)current;
        sf_header true_header = block->header ^ MAGIC;
        size_t block_size = true_header & BLOCK_SIZE_MASK;
        bool allocated = (true_header & THIS_BLOCK_ALLOCATED) != 0;
        
        //if block is allocated (including those in quick lists)
        if (allocated) {
            total_allocated_size += block_size;
        }
        
        current += block_size;  //go to next block
    }
    
    if (total_allocated_size == 0) {
        return 0.0;
    }
    
    // Fragmentation = current_payload / total_allocated_size
    // current_payload represents the sum of all requested sizes
    // total_allocated_size represents the sum of all allocated block sizes
    return (double)current_payload / (double)total_allocated_size;
}

double sf_utilization() {
    if(sf_mem_start() == sf_mem_end()){
        return 0.0;
    }
    size_t heap_size = (char *)sf_mem_end() - (char *)sf_mem_start();
    
    return (double)max_payload / (double)heap_size;
}