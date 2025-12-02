#define TEST_TIMEOUT 15
#include "__grading_helpers.h"
#include "debug.h"

/*
 * Check LIFO discipline on free list
 */
Test(sf_memsuite_grading, malloc_free_lifo, .timeout=TEST_TIMEOUT)
{
    size_t sz = 200;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(x-8, 1, 224);
    void * u = sf_malloc(sz);
    _assert_nonnull_payload_pointer(u);
    _assert_block_info(u-8, 1, 224);
    void * y = sf_malloc(sz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(y-8, 1, 224);
    void * v = sf_malloc(sz);
    _assert_nonnull_payload_pointer(v);
    _assert_block_info(v-8, 1, 224);
    void * z = sf_malloc(sz);
    _assert_nonnull_payload_pointer(z);
    _assert_block_info(z-8, 1, 224);
    void * w = sf_malloc(sz);
    _assert_nonnull_payload_pointer(w);
    _assert_block_info(w-8, 1, 224);

    sf_free(x);
    sf_free(y);
    sf_free(z);

    void * z1 = sf_malloc(sz);
    _assert_nonnull_payload_pointer(z1);
    _assert_block_info(z1-8, 1, 224);
    void * y1 = sf_malloc(sz);
    _assert_nonnull_payload_pointer(y1);
    _assert_block_info(y1-8, 1, 224);
    void * x1 = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x1);
    _assert_block_info(x1-8, 1, 224);

    cr_assert(x == x1 && y == y1 && z == z1,
      "malloc/free does not follow LIFO discipline");

    _assert_free_block_count(2704, 1);
    _assert_quick_list_block_count(0, 0);

    _assert_errno_eq(0);
}

/*
 * Realloc tests.
 */
Test(sf_memsuite_grading, realloc_larger, .timeout=TEST_TIMEOUT)
{
    size_t sz = 200;
    size_t nsz = 1024;

    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(x-8, 1, 224);

    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(y-8, 1, 1040);

    _assert_free_block_count(224, 1);
    _assert_quick_list_block_count(0, 0);

    _assert_free_block_count(2784, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_smaller, .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    size_t nsz = 200;

    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(x-8, 1, 1040);

    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(y-8, 1, 224);

    cr_assert_eq(x, y, "realloc to smaller size did not return same payload pointer");

    _assert_free_block_count(3824, 1);
    _assert_quick_list_block_count(0, 0);
    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_same, .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    size_t nsz = 1024;

    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(x-8, 1, 1040);

    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(y-8, 1, 1040);

    cr_assert_eq(x, y, "realloc to same size did not return same payload pointer");

    _assert_free_block_count(3008, 1);

    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_splinter, .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    size_t nsz = 1020;

    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(x-8, 1, 1040);

    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(y-8, 1, 1040);

    cr_assert_eq(x, y, "realloc to smaller size did not return same payload pointer");

    _assert_free_block_count(3008, 1);
    _assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_size_0, .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(x-8, 1, 1040);

    void * y = sf_malloc(sz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(y-8, 1, 1040);

    void * z = sf_realloc(x, 0);
    _assert_null_payload_pointer(z);
    _assert_block_info(x-8, 0, 1040);

    // after realloc x to (2) z, x is now a free block
    _assert_free_block_count(1040, 1);

    // the size of the remaining free block
    _assert_free_block_count(1968, 1);

    _assert_errno_eq(0);
}

/*
 * Illegal pointer tests.
 */
Test(sf_memsuite_grading, free_null, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    (void) sf_malloc(sz);
    sf_free(NULL);
    cr_assert_fail("SIGABRT should have been received");
}

//This test tests: Freeing a memory that was free-ed already
Test(sf_memsuite_grading, free_unallocated, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    void *x = sf_malloc(sz);
    sf_free(x);
    sf_free(x);
    cr_assert_fail("SIGABRT should have been received");
}

Test(sf_memsuite_grading, free_block_too_small, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    void * x = sf_malloc(sz);
    ((sf_block *)(x - 8))->header = 0x11UL ^ MAGIC;

    sf_free(x);
    cr_assert_fail("SIGABRT should have been received");
}

#define NTRACKED 1000
#define ORDER_RANGE 13

static void run_stress_test(int final_free)
{
    errno = 0;

    int order_range = ORDER_RANGE;
    int nullcount = 0;

    void * tracked[NTRACKED];

    for (int i = 0; i < NTRACKED; i++)
    {
        int order = (rand() % order_range);
        size_t extra = (rand() % (1 << order));
        size_t req_sz = (1 << order) + extra;

        tracked[i] = sf_malloc(req_sz);
        // if there is no free to malloc
        if (tracked[i] == NULL)
        {
            order--;
            while (order >= 0)
            {
                req_sz = (1 << order) + (extra % (1 << order));
                tracked[i] = sf_malloc(req_sz);
                if (tracked[i] != NULL)
                {
                    break;
                }
                else
                {
                    order--;
                }
            }
        }

        // tracked[i] can still be NULL
        if (tracked[i] == NULL)
        {
            nullcount++;
            // It seems like there is not enough space in the heap.
            // Try to halve the size of each existing allocated block in the heap,
            // so that next mallocs possibly get free blocks.
            for (int j = 0; j < i; j++)
            {
                if (tracked[j] == NULL)
                {
                    continue;
                }
                sf_block * bp = ((sf_block *)(tracked[j] - 8));
                req_sz = ((bp->header ^ MAGIC) & 0xFFFFFFF0LU) >> 1;
                tracked[j] = sf_realloc(tracked[j], req_sz);
            }
        }
        errno = 0;
    }

    for (int i = 0; i < NTRACKED; i++)
    {
        if (tracked[i] != NULL)
        {
            sf_free(tracked[i]);
        }
    }

    _assert_heap_is_valid();
}


// random block assigments. Tried to give equal opportunity for each possible order to appear.
// But if the heap gets populated too quickly, try to make some space by realloc(half) existing
// allocated blocks.
Test(sf_memsuite_grading, stress_test, .timeout = TEST_TIMEOUT)
{
    run_stress_test(1);
    _assert_heap_is_valid();
}

// Statistics tests.

Test(sf_memsuite_stats, peak_utilization, .timeout = TEST_TIMEOUT)
{
    double actual;
    double expected;
    void * w = sf_malloc(10);
    void * x = sf_malloc(300);
    void * y = sf_malloc(500);
    sf_free(x);

    actual = sf_utilization();
    expected = 0.197754;

    cr_assert_float_eq(actual, expected, 0.0001, "peak utilization_1 did not match (exp=%f, found=%f)", expected, actual);

    x = sf_malloc(400);
    void * z = sf_malloc(1024);
    sf_free(w);
    sf_free(x);
    x = sf_malloc(2048);

    actual = sf_utilization();
    expected = 0.436035;

    cr_assert_float_eq(actual, expected, 0.0001, "peak utilization_2 did not match (exp=%f, found=%f)", expected, actual);

    sf_free(x);
    x = sf_malloc(7400);
    sf_free(y);
    sf_free(z);

    actual = sf_utilization();
    expected = 0.726237;

    cr_assert_float_eq(actual, expected, 0.0001, "peak utilization_3 did not match (exp=%f, found=%f)", expected, actual);
}

Test(sf_memsuite_stats, internal_fragmentation, .timeout = TEST_TIMEOUT)
{
    double actual;
    double expected;
    void * w = sf_malloc(22);
    void * x = sf_malloc(305);
    actual = sf_fragmentation();
    expected = 0.851562;

    cr_assert_float_eq(actual, expected, 0.0001, "internal fragmentation_1 did not match (exp=%f, found=%f)", expected, actual);

    void * y = sf_malloc(1500);
    sf_free(x);
    x = sf_malloc(2048);

    void * z = sf_malloc(526);
    actual = sf_fragmentation();
    expected = 0.980843;

    cr_assert_float_eq(actual, expected, 0.0001, "internal fragmentation_2 did not match (exp=%f, found=%f)", expected, actual);

    sf_free(z);
    z = sf_malloc(700);
    sf_free(x);
    x = sf_malloc(4096);
    sf_free(w);
    sf_free(x);

    {
        int *arr[50];
        int i;

        for(i=1; i < 50;i++)
            arr[i] = sf_malloc(10 * i + i);

        actual = sf_fragmentation();
	expected = 0.929495;

	cr_assert_float_eq(actual, expected, 0.0001, "internal fragmentation_3 did not match (exp=%f, found=%f)", expected, actual);

        for(i=1; i < 50;i++)
            sf_free(arr[i]);
    }

    sf_free(y);
    sf_free(z);
}

Test(sf_memsuite_stats, realloc_internal_fragmentation, .timeout = TEST_TIMEOUT)
{
    double actual;
    double expected;
    void * y = sf_malloc(22);
    void * x = sf_malloc(305);

    {
        int *arr[50];
        int i;

        for(i=1; i < 50;i++)
            arr[i] = sf_malloc(10 * i + i);

        for(i=1; i < 50;i++)
            arr[i] = sf_realloc(arr[i], 10 * i);

        for(i=1; i < 50;i++)
            sf_free(arr[i]);
    }

    void * w = sf_malloc(100);
    sf_realloc(w, 40);

    actual = sf_fragmentation();
    expected = 0.819196;

    cr_assert_float_eq(actual, expected, 0.0001, "internal fragmentation did not match (exp=%f, found=%f)", expected, actual);

    sf_free(w);
    sf_free(x);
    sf_free(y);
}

Test(sf_memsuite_stats, stress_test, .timeout = TEST_TIMEOUT)
{
    double actual;
    double expected;

    run_stress_test(0);

    actual = sf_utilization();
    expected = 0.966454;

    cr_assert_float_eq(actual, expected, 0.0001, "peak utilization did not match (exp=%f, found=%f)",
        expected, actual);

    _assert_heap_is_valid();
}
