/**
 * API:
 * - HEAP_VALUE_TYPE: Data type stored in the heap
 * - HEAP_SIZE: Maximum size of the heap
 * - HEAP_COMPARE: Compares two heap members, strongly ordering them.
 * - HEAP_PREFIX: Prefix to add to all names
 */

// One layer of indirection so macro arguments get expanded first.
#define HEAP_CONCAT(a, b) HEAP_CONCAT_IMPL(a, b)
#define HEAP_CONCAT_IMPL(a, b) a##b

struct HEAP_CONCAT(HEAP_PREFIX, _type)
{
    unsigned size;
    HEAP_VALUE_TYPE data[HEAP_SIZE];
};

static struct HEAP_CONCAT(HEAP_PREFIX, _type) HEAP_CONCAT(HEAP_PREFIX, _state);

void HEAP_CONCAT(HEAP_PREFIX, _init)()
{
    HEAP_CONCAT(HEAP_PREFIX, _state).size = 0;
}

enum
{
    HEAP_CONCAT(HEAP_PREFIX, _fail) = ~(unsigned)0,
};

static inline void HEAP_CONCAT(HEAP_PREFIX, _swap)(HEAP_VALUE_TYPE *lhs,
                                                   HEAP_VALUE_TYPE *rhs)
{
    const HEAP_VALUE_TYPE tmp = *lhs;
    *lhs = *rhs;
    *rhs = tmp;
}

static unsigned HEAP_CONCAT(HEAP_PREFIX, _swim)(unsigned idx)
{
    struct HEAP_CONCAT(HEAP_PREFIX, _type) *v =
        &HEAP_CONCAT(HEAP_PREFIX, _state);
    for (unsigned parent = (idx - 1) / 2;
         idx > 0 && HEAP_COMPARE(v->data[idx], v->data[parent]);
         idx = parent, parent = (idx - 1) / 2)
    {
        HEAP_CONCAT(HEAP_PREFIX, _swap)(&v->data[idx], &v->data[parent]);
    }
    return idx;
}

static unsigned HEAP_CONCAT(HEAP_PREFIX, _sink)(unsigned idx)
{
    struct HEAP_CONCAT(HEAP_PREFIX, _type) *v =
        &HEAP_CONCAT(HEAP_PREFIX, _state);
    unsigned last_parent_idx = (v->size - 2) / 2;

    while (idx <= last_parent_idx)
    {
        const unsigned left_child = idx * 2 + 1;
        const unsigned right_child = idx * 2 + 2;
        // Check the right child first to keep the operation "semi-stable".
        // Note that there's a corner case where right_child is not within the
        // heap anymore.
        if (right_child < v->size &&
            HEAP_COMPARE(v->data[right_child], v->data[idx]))
        {
            HEAP_CONCAT(HEAP_PREFIX, _swap)
            (&v->data[right_child], &v->data[idx]);
            idx = right_child;
        }
        else if (HEAP_COMPARE(v->data[left_child], v->data[idx]))
        {
            HEAP_CONCAT(HEAP_PREFIX, _swap)
            (&v->data[left_child], &v->data[idx]);
            idx = left_child;
        }
        else
            break;
    }

    return idx;
}

unsigned HEAP_CONCAT(HEAP_PREFIX, _insert)(HEAP_VALUE_TYPE value)
{
    struct HEAP_CONCAT(HEAP_PREFIX, _type) *v =
        &HEAP_CONCAT(HEAP_PREFIX, _state);
    if (v->size == HEAP_SIZE)
        return HEAP_CONCAT(HEAP_PREFIX, _fail);
    unsigned idx = v->size++;
    v->data[idx] = value;
    return HEAP_CONCAT(HEAP_PREFIX, _swim)(idx);
}

void HEAP_CONCAT(HEAP_PREFIX, _pop)()
{
    struct HEAP_CONCAT(HEAP_PREFIX, _type) *v =
        &HEAP_CONCAT(HEAP_PREFIX, _state);
    v->data[0] = v->data[--v->size];
    HEAP_CONCAT(HEAP_PREFIX, _sink(0));
}
