
#ifndef DEFINE_INCLUDE_SP_TABLE
#define DEFINE_INCLUDE SP_TABLE

// would actually like a O(1) insert data structure
// with O(sqrt n) or better lookup time

typedef struct slider_puzzle_table_entry {
    sp_ordinal_t prev_node;
    sp_ordinal_t key;
    int key_hash;
    char is_occupied; // Valid for all tabents
    char is_bottom_node; // Valid for occupied tabents
    char next_node_offset; // Valid for tabents except for bottom nodes
    char bucket_depth, bottom_idx; // Valid for "top node" tabents
} sp_tabent_t;

#define SP_TAB_SIZE (64*1024)
#define SP_TABENT_KEYS_LEN 255

// since this should be useful for doing searches for moves,
// this should hold up to 4 or 16 million positions? load factor 25%?
typedef struct slider_puzzle_table {
    int keys_held, buckets_allocated;
    int load_too_high, soft_maxed_count;
    struct slider_puzzle_table *next_table;
    sp_tabent_t storage[SP_TAB_SIZE];
} sp_table_t;

int sp_hash_ordinal(sp_ordinal_t key);
sp_table_t* sp_table__new();
void sp_table__del(sp_table_t *obj);

int sp_table__insert(sp_table_t *head,
        const sp_ordinal_t key,
        const sp_ordinal_t prev);

sp_ordinal_t* sp_table__find(sp_table_t *head, const sp_ordinal_t data);
int sp_tabent__probe_insert(sp_table_t *head, int idx, sp_ordinal_t key);

#endif

