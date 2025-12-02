#define TEST_TIMEOUT 15
#include "__grading_helpers.h"

/*
 * Do one malloc and check that the prologue and epilogue are correctly initialized
 */
Test(sf_memsuite_grading, initialization, .timeout = TEST_TIMEOUT)
{
	void *p  = sf_malloc(1);
	cr_assert(p != NULL, "The pointer should NOT be null after a malloc");
	_assert_heap_is_valid();
}

/*
 * Single malloc tests, up to the size that forces a non-minimum block size.
 */
Test(sf_memsuite_grading, single_malloc_1, .timeout = TEST_TIMEOUT)
{
	void *x = sf_malloc(1);

	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x-8, 1, 32);
	_assert_heap_is_valid();
	_assert_free_block_count(4016, 1);
	_assert_quick_list_block_count(0, 0);

	_assert_errno_eq(0);
}

Test(sf_memsuite_grading, single_malloc_16, .timeout = TEST_TIMEOUT)
{
	void *x = sf_malloc(16);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x-8, 1, 32);
	_assert_heap_is_valid();

	_assert_free_block_count(4016, 1);
	_assert_quick_list_block_count(0, 0);

	_assert_errno_eq(0);
}

Test(sf_memsuite_grading, single_malloc_32, .timeout = TEST_TIMEOUT)
{
	void *x = sf_malloc(32);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x-8, 1, 48);
	_assert_heap_is_valid();

	_assert_free_block_count(4000, 1);
	_assert_quick_list_block_count(0, 0);

	_assert_errno_eq(0);
}

/*
 * Single malloc test, of a size exactly equal to what is left after initialization.
 * Requesting the exact remaining size (leaving space for the header)
 */
Test(sf_memsuite_grading, single_malloc_exactly_one_page_needed, .timeout = TEST_TIMEOUT)
{
	void *x = sf_malloc(4032);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x-8, 1, 4048);
	_assert_heap_is_valid();

	_assert_free_block_count(0, 0);
	_assert_quick_list_block_count(0, 0);

	_assert_errno_eq(0);
}

/*
 * Single malloc test, of a size just larger than what is left after initialization.
 */
Test(sf_memsuite_grading, single_malloc_more_than_one_page_needed, .timeout = TEST_TIMEOUT)
{
	void *x = sf_malloc(4048);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x-8, 1, 4064);
	_assert_heap_is_valid();

	_assert_free_block_count(4080, 1);
	_assert_quick_list_block_count(0, 0);

	_assert_errno_eq(0);
}

/*
 * Single malloc test, of multiple pages.
 */
Test(sf_memsuite_grading, single_malloc_three_pages_needed, .timeout = TEST_TIMEOUT)
{
	void *x = sf_malloc(12000);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x-8, 1, 12016);
	_assert_heap_is_valid();

	_assert_free_block_count(224, 1);
	_assert_quick_list_block_count(0, 0);

	_assert_errno_eq(0);
}

/*
 * Single malloc test, unsatisfiable.
 * There should be one single large block.
 */
Test(sf_memsuite_grading, single_malloc_max, .timeout = TEST_TIMEOUT)
{
	void *x = sf_malloc(151552);
	_assert_null_payload_pointer(x);
	_assert_heap_is_valid();

	_assert_free_block_count(151504, 1);
	_assert_quick_list_block_count(0, 0);

	_assert_errno_eq(ENOMEM);
}

/*
 * Malloc/free with/without coalescing.
 */
Test(sf_memsuite_grading, malloc_free_no_coalesce, .timeout = TEST_TIMEOUT)
{
    size_t sz1 = 200;
	size_t sz2 = 300;
	size_t sz3 = 400;

	void *x = sf_malloc(sz1);
	_assert_nonnull_payload_pointer(x);

	void *y = sf_malloc(sz2);
	_assert_nonnull_payload_pointer(y);

	void *z = sf_malloc(sz3);
	_assert_nonnull_payload_pointer(z);

	sf_free(y);

	_assert_block_info(x-8, 1, 224);
	_assert_block_info(y-8, 0, 320);
	_assert_block_info(z-8, 1, 416);
	_assert_heap_is_valid();

	_assert_free_block_count(320, 1);
	_assert_quick_list_block_count(0, 0);

	_assert_free_block_count(3088, 1);

	_assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_coalesce_lower, .timeout = TEST_TIMEOUT)
{
	size_t sz1 = 200;
	size_t sz2 = 300;
	size_t sz3 = 400;
	size_t sz4 = 500;

	void *x = sf_malloc(sz1);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x-8, 1, 224);

	void *y = sf_malloc(sz2);
	_assert_nonnull_payload_pointer(y);
	_assert_block_info(y-8, 1, 320);

	void *z = sf_malloc(sz3);
	_assert_nonnull_payload_pointer(z);
	_assert_block_info(z-8, 1, 416);

	void *w = sf_malloc(sz4);
	_assert_nonnull_payload_pointer(w);
	_assert_block_info(w-8, 1, 528);

	sf_free(y);
	sf_free(z);

	_assert_block_info(x-8, 1, 224);
	_assert_block_info(y-8, 0, 736);
	_assert_block_info(w-8, 1, 528);
	_assert_heap_is_valid();

	_assert_free_block_count(736, 1);
	_assert_quick_list_block_count(0, 0);
	_assert_free_block_count(2560, 1);

	_assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_coalesce_upper, .timeout = TEST_TIMEOUT)
{
	size_t sz1 = 200;
	size_t sz2 = 300;
	size_t sz3 = 400;
	size_t sz4 = 500;

	void *x = sf_malloc(sz1);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x-8, 1, 224);

	void *y = sf_malloc(sz2);
	_assert_nonnull_payload_pointer(y);
	_assert_block_info(y-8, 1, 320);

	void *z = sf_malloc(sz3);
	_assert_nonnull_payload_pointer(z);
	_assert_block_info(z-8, 1, 416);

	void *w = sf_malloc(sz4);
	_assert_nonnull_payload_pointer(w);
	_assert_block_info(w-8, 1, 528);

	sf_free(z);
	sf_free(y);

	_assert_block_info(x-8, 1, 224);
	_assert_block_info(y-8, 0, 736);
	_assert_block_info(w-8, 1, 528);
	_assert_heap_is_valid();

	_assert_free_block_count(320 + 416, 1);
	_assert_quick_list_block_count(0, 0);
	_assert_free_block_count(2560, 1);

	_assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_coalesce_both, .timeout = TEST_TIMEOUT)
{
	size_t sz1 = 200;
	size_t sz2 = 300;
	size_t sz3 = 400;
	size_t sz4 = 500;

	void *x = sf_malloc(sz1);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x-8, 1, 224);

	void *y = sf_malloc(sz2);
	_assert_nonnull_payload_pointer(y);
	_assert_block_info(y-8, 1, 320);

	void *z = sf_malloc(sz3);
	_assert_nonnull_payload_pointer(z);
	_assert_block_info(z-8, 1, 416);

	void *w = sf_malloc(sz4);
	_assert_nonnull_payload_pointer(w);
	_assert_block_info(w-8, 1, 528);

	sf_free(x);
	sf_free(z);
	sf_free(y);

	_assert_block_info(x-8, 0, 960);
	_assert_heap_is_valid();

	_assert_free_block_count(960, 1);
	_assert_quick_list_block_count(0, 0);
	_assert_free_block_count(2560, 1);

	_assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_first_block, .timeout = TEST_TIMEOUT)
{
	size_t sz1 = 200;
	size_t sz2 = 300;

	void *x = sf_malloc(sz1);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x-8, 1, 224);

	void *y = sf_malloc(sz2);
	_assert_nonnull_payload_pointer(y);
	_assert_block_info(y-8, 1, 320);

	sf_free(x);

	_assert_block_info(x-8, 0, 224);
	_assert_block_info(y-8, 1, 320);
	_assert_heap_is_valid();

	_assert_free_block_count(224, 1);
	_assert_quick_list_block_count(0, 0);
	_assert_free_block_count(3504, 1);

	_assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_last_block, .timeout = TEST_TIMEOUT)
{
	size_t sz1 = 200;
	size_t sz2 = 3808;

	void *x = sf_malloc(sz1);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x-8, 1, 224);

	void *y = sf_malloc(sz2);
	_assert_nonnull_payload_pointer(y);
	_assert_block_info(y-8, 1, 3824);

	sf_free(y);

	size_t exp_free_size = 3824;
	_assert_block_info(x-8, 1, 224);
	_assert_block_info(y-8, 0, exp_free_size);
	_assert_free_block_count(exp_free_size, 1);
	_assert_quick_list_block_count(0, 0);

	_assert_heap_is_valid();

	_assert_errno_eq(0);
}

/*
 * Check that malloc leaves no splinter.
 */
Test(sf_memsuite_grading, malloc_with_splinter, .timeout = TEST_TIMEOUT)
{
	size_t sz = 4001;
	void *x = sf_malloc(sz);

	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x-8, 1, 4048);
	_assert_heap_is_valid();

	_assert_free_block_count(0, 0);
	_assert_quick_list_block_count(0, 0);

	_assert_errno_eq(0);
}

/*
 * Determine if the existing heap can satisfy an allocation request
 * of a specified size.  The heap blocks are examined directly;
 * freelists are ignored.
 */
static int can_satisfy_request(size_t size) {
    size_t asize = ((size + 31) >> 4) << 4;
    sf_block *bp = (sf_block *)(sf_mem_start() + 40);

    while(bp < (sf_block *)(sf_mem_end() - 8)) {
	if(!((bp->header ^ MAGIC) & 0x1) && asize <= ((bp->header ^ MAGIC) & 0xFFFFFFF0LU))
	    return 1;  // Suitable free block found.
	bp = (sf_block *)((char *)(bp) + ((bp->header ^ MAGIC) & 0xFFFFFFF0LU));
    }
    return 0;  // No suitable free block.
}

/*
 *  Allocate small blocks until memory exhausted.
 */
Test(sf_memsuite_grading, malloc_to_exhaustion, .timeout = TEST_TIMEOUT)
{
    size_t size = 90;  // Arbitrarily chosen small size.
    size_t asize = 112;
    // To prevent looping, set an arbitrary limit on the number of allocations.
    // There isn't any way to check here how much the heap might be able to grow.
    int limit = LOOP_LIMIT;

    void *x;
    size_t bsize = 0;
    while(limit > 0 && (x = sf_malloc(size)) != NULL) {
	sf_block *bp = x - 8;
	bsize = ((bp->header ^ MAGIC) & 0xFFFFFFF0LU);
	// The allocated block could be slightly larger than the adjusted size,
	// due to splitting restrictions.  In this test, this can occur when the
        // last block in the current heap is big enough, but would leave a splinter
	// when split.  However, we don't check to verify that it is the last block.
	cr_assert(!bsize || bsize - asize < 32,
		  "Block has incorrect size (was: %lu, exp range: [%lu, %lu))",
		  bsize, asize, asize + 32);
	limit--;
    }
    cr_assert_null(x, "Allocation succeeded, but heap should be exhausted.");
    _assert_errno_eq(ENOMEM);
    cr_assert_null(sf_mem_grow(), "Allocation failed, but heap can still grow.");
    cr_assert(!can_satisfy_request(size),
	      "Allocation failed, but there is still a suitable free block.");
}

// Quick list tests
Test(sf_memsuite_grading, simple_quick_list, .timeout = TEST_TIMEOUT)
{
	void *x = sf_malloc(32);

	sf_free(x);  // Goes to quick list

	_assert_quick_list_block_count(0, 1);
	_assert_errno_eq(0);
}

Test(sf_memsuite_grading, multiple_quick_lists, .timeout = TEST_TIMEOUT)
{

	void *a = sf_malloc(32);
	void *b = sf_malloc(48);
	void *c = sf_malloc(96);
	void *d = sf_malloc(112);
	void *e = sf_malloc(128);

	sf_free(a);  // Goes to quick list
	sf_free(b);
	sf_free(c);
	sf_free(d);
	sf_free(e);

	_assert_quick_list_block_count(0, 5);

	_assert_errno_eq(0);
}
