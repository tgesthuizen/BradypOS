#define HEAP_VALUE_TYPE int
#define HEAP_SIZE 8
#define HEAP_COMPARE(lhs, rhs) (lhs) < (rhs)
#define HEAP_PREFIX test_heap

#include <heap.inc>
#include <stdio.h>
#include <string.h>

static void print_assert(const char *file, int line, const char *cond)
{
    printf("%s: %d: Assertion failed: %d\n", file, line, cond);
}
static void print_cmp_int(const char *file, int line, const char *clhs,
                          const char *crhs, int lhs, int rhs)
{
    printf("%s: %d: Assertion fialed: %s and %s are not the same (%d vs. %d)\n",
           file, line, clhs, crhs, lhs, rhs);
}
static void print_cmp_unsigned(const char *file, int line, const char *clhs,
                               const char *crhs, unsigned lhs, unsigned rhs)
{
    printf("%s: %d: Assertion fialed: %s and %s are not the same (%u vs. %u)\n",
           file, line, clhs, crhs, lhs, rhs);
}
static void print_cmp_generic(const char *file, int line, const char *clhs,
                              const char *crhs)
{
    printf("%s: %d: Assertion fialed: %s and %s are not the same\n", file, line,
           clhs, crhs);
}

#define TEST_ASSERT(cond)                                                      \
    if (!(cond))                                                               \
    {                                                                          \
        print_assert(__FILE__, __LINE__, #cond);                               \
        return 1;                                                              \
    }
#define TEST_CMP(a, b)                                                         \
    if ((a) != (b))                                                            \
    {                                                                          \
        _Generic((a),                                                          \
            int: print_cmp_int(__FILE__, __LINE__, #a, #b, a, b),              \
            unsigned: print_cmp_unsigned(__FILE__, __LINE__, #a, #b, a, b),    \
            default: print_cmp_generic(__FILE__, __LINE__, #a, #b));           \
        return 1;                                                              \
    }

static void reset_heap()
{
    memset(test_heap_state.data, 0xFF, sizeof(test_heap_state.data));
    test_heap_state.size = 0;
}

static int heap_starts_empty()
{
    TEST_CMP(test_heap_state.size, 0);
    return 0;
}

static int basic_order_test()
{
    test_heap_insert(4);
    test_heap_insert(3);
    TEST_CMP(test_heap_state.data[0], 3);
    TEST_CMP(test_heap_state.data[1], 4);
    test_heap_pop();
    TEST_CMP(test_heap_state.size, 1);
    TEST_CMP(test_heap_state.data[0], 4);
    test_heap_pop();
    TEST_CMP(test_heap_state.size, 0);
    return 0;
}

static int full_heap_insert_fails()
{
    for (int i = 0; i < HEAP_SIZE; ++i)
    {
        TEST_ASSERT(test_heap_insert(i) != test_heap_fail);
    }
    // Heap is full
    TEST_CMP(test_heap_insert(42), test_heap_fail);
    // Size is still the same after failed insert
    TEST_CMP(test_heap_state.size, HEAP_SIZE);
    for (int i = 0; i < HEAP_SIZE; ++i)
    {
        test_heap_pop();
    }
    TEST_CMP(test_heap_state.size, 0);
    return 0;
}

static int heap_delete_test()
{
    for (int i = 0; i < 5; ++i)
        TEST_ASSERT(test_heap_insert(i) != test_heap_fail);
    test_heap_delete(3);
    TEST_CMP(test_heap_state.size, 4);
    // Swaps the deleted element with the last
    TEST_CMP(test_heap_state.data[3], 4);
    return 0;
}

int main()
{
    int ret;
    ret = heap_starts_empty();
    if (ret)
        return ret;
    reset_heap();
    ret = basic_order_test();
    if (ret)
        return ret;
    reset_heap();
    ret = full_heap_insert_fails();
    if (ret)
        return ret;
    reset_heap();
    ret = heap_delete_test();
    if (ret)
        return ret;

    puts("SUCCESS!");
    return 0;
}
