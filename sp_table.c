
#include "sp_table.h"

#define SP_HASH_BUCKET_SOFT_MDEPTH 20
#define SP_HASH_BUCKET_SOFT_MCOUNT 40

// SP_HASH hash function params

#define SP_HASH_SALT_1 0x000000000013935Dull
#define SP_HASH_SALT_2 0x000DB28700000000ull
#define SP_HASH_SALT_3 0x000000B477F00000ull
#define SP_HASH_SALT_4 0x00CB71000008A500ull

#define SP_HASH_PRIME_1_1 (56909 * 54617)
#define SP_HASH_PRIME_1_2 595939
#define SP_HASH_PRIME_2_1 (65033 * 52957)
#define SP_HASH_PRIME_2_2 600091
#define SP_HASH_PRIME_3_1 (53639 * 68687)
#define SP_HASH_PRIME_3_2 617717
#define SP_HASH_PRIME_4_1 (78989 * 56893)
#define SP_HASH_PRIME_4_2 626861

int sp_hash_ordinal(sp_ordinal_t key) {
    unsigned long long partial1 =
            SP_HASH_PRIME_1_1 * (key ^ SP_HASH_SALT_1) / SP_HASH_PRIME_1_2;
    unsigned long long partial2 =
            SP_HASH_PRIME_2_1 * (key ^ SP_HASH_SALT_2) / SP_HASH_PRIME_2_2;
    unsigned long long partial3 =
            SP_HASH_PRIME_3_1 * (key ^ SP_HASH_SALT_3) / SP_HASH_PRIME_3_2;
    unsigned long long partial4 =
            SP_HASH_PRIME_4_1 * (key ^ SP_HASH_SALT_4) / SP_HASH_PRIME_4_2;

    return ( ((partial1 + partial2 + partial3 + partial4) / 1222219)
            % SP_TAB_SIZE);
}

sp_table_t* sp_table__new() {
    // Yay, "free" zero-allocation
    sp_table_t *rv = calloc(1, sizeof(sp_table_t));
    return rv;
}

void sp_table__del(sp_table_t *obj) {
    while (obj != NULL) {
        sp_table_t *tmp = obj->next_table;
        int i;
        for (i = 0; i < SP_TAB_SIZE; i++) {
            if (obj->storage[i].keys != NULL) {
                free(obj->storage[i].keys);
            }
        }
        free(obj);
        obj = tmp;
    }
}

// return -1 on error, -2 on 'already found', 0 success
int sp_table__insert(sp_table_t *head,
        const sp_ordinal_t key,
        const sp_ordinal_t prev) {
    assert(head != NULL);
    int idx = sp_hash_ordinal(key);
    sp_tabent_t *cursor;
    while(head->load_too_high) {
        cursor = head->storage[idx];
        if (!(cursor->is_occupied)) {
            break;
        }
        else if (cursor->key_hash == idx) {
            int target = sp_table__probe_insert(head, idx, key);
            if (target == -2) {
                return -2; // already exists
            }
            else if (target >= 0) {
                head->storage[target].prev_node = prev;
                head->storage[target].key = key;
                return 0;
            } // falls thru: table is full (-1)
        }
        if (head->next_table == NULL) {
        head->next_table = sp_table__new();
            if (head->next_table == NULL) {
                return -1;
            }
        }
        head = head->next_table;
    }
    sp_tabent_t *cursor = &(head->storage[idx]);
    if (cursor->keys == NULL) {
        sp_ordinal_t *bucket =
                malloc(sizeof(sp_ordinal_t) * SP_TABENT_KEYS_LEN);
        if (bucket == NULL) {
            return -1;
        }
        cursor->keys = bucket;
        cursor->keys[0] = key;
        head->buckets_allocated += 1;
    }
    else {
        cursor->keys[cursor->bucket_len++] = key;
        if(cursor->bucket_len == SP_HASH_BUCKET_SOFT_MAX) {
            head->soft_maxed_count += 1;
            if ((head->soft_maxed_count * 100) >
                (head->buckets_allocated * SP_HASH_BUCKET_SOFT_PCT)) {
                head->load_too_high = 1;
            }
        }
        else if (cursor->bucket_len == SP_TABENT_KEYS_LEN) {
            head->load_too_high = 1;
        }
    }
    head->keys_held += 1;
    return 0;
}

int sp_table__len(sp_table_t *obj) {
    int total = 0;
    while (obj != NULL) {
        total += obj->keys_held;
        obj = obj->next_table;
    }
    return total;
}

// Returns pointer to previous node, or NULL if not found.
// This pointer is borrowed and its lifetime is that of *head.
sp_ordinal_t* sp_table__find(sp_table_t *head, const sp_ordinal_t data) {
    int idx = sp_hash_ordinal(data);
    while (head != NULL) {
        sp_tabent_t *cursor = &(head->storage[idx]);
        while (cursor->is_occupied && (cursor->key_hash != idx)) {
            0; // 
        }
        if (!(cursor->is_occupied)) {
            return NULL; // Cannot exist in this or any further table
        }
        while (!(cursor->is_bottom_node)) {
            if (cursor->key == data) {
                return &(cursor->prev_node);
            }
            cursor += next_node_offset;
        }
        int i;
        int undec_data;

        for (i = 0; i < len; i++) {
            undec_data = cursor[i];
            undec_data &= 0x0000ffffffffffffull;
            if (undec_data == data) {
                sp_tabent_t* rv = {
                return cursor+i;
            }
        }

        head = head->next_table;
    }
    return NULL; // == head
}

int sp_tabent__probe_insert(sp_table_t *head, int idx, sp_ordinal_t key) {
    // TODO: impl
}

