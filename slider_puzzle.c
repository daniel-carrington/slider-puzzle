
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SP_SIZE 16  // number of spaces in the 4x4 puzzle, including the hole
// This
#define SP_ASC_SEQUENCE {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}
typedef unsigned long long sp_ordinal_t;
// Space left over in this cacheline. Structure size: 48 bytes
typedef struct slider_puzzle_position {
    unsigned long long ordinal;
    char positions[SP_SIZE];
} sp_pos_t;

const sp_pos_t start_position = {
    0ull,
    SP_ASC_SEQUENCE,
    // Represents this board:
    //     0; 1; 2; 3;
    //     4; 5; 6; 7;
    //     8; 9;10;11;
    //    12;13;14;15;
};

const char pos_masks[SP_SIZE] = {
    0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, // 32 bits
    0x7, 0x7, 0x7, 0x7, // 4 * 3 = 12 bits
    0x3, 0x3, // 4 bits
    0x1,
    0,
};

const char pos_shifts[SP_SIZE]  = {
     0,
     4,  8, 12, 16, 20, 24, 28, 32,
    35, 38, 41, 44,
    46, 48,
    49, // No significant bits for the last number
};

const char asc_seq[SP_SIZE] = SP_ASC_SEQUENCE;

// expects a valid 'positions' in sp_pos_t, will initialize 'ordinal'
// returns number of errors
int sp_pos__fill_in_ordinal(sp_pos_t *obj) {
    char remaining[SP_SIZE] = SP_ASC_SEQUENCE;
    int bad_codings = 0;
    int i, j;
    obj->ordinal = 0;
    for (i = 0; i < SP_SIZE; i++) {
        unsigned long long code;
        for (j = 0; j < SP_SIZE-i; j++) {
            if (remaining[j] == obj->positions[j]) {
                code = j;
                break;
            }
        }
        if (j == SP_SIZE-i) {
            j = 0;
            bad_codings++;
        }
        obj->ordinal |= code << pos_shifts[i];
        for (; j < SP_SIZE - i; j++) {
            remaining[j] = remaining[j+1];
        }
    }
    return bad_codings;
}

// expects a valid 'ordinal' in sp_pos_t, will initialize 'positions'
// returns number of errors
int sp_pos__fill_in_positions(sp_pos_t *obj) {
    char remaining[SP_SIZE] = SP_ASC_SEQUENCE;
    int bad_codings = 0;
    int i, j;

    for (i = 0; i < SP_SIZE; i++) {
        sp_ordinal_t next;
        next = pos_masks[i] & (obj->ordinal >> pos_shifts[i]);
        if (next >= SP_SIZE - i) {
            next = 0;
            bad_codings++;
        }
        obj->positions[i] = remaining[next];
        for (j = next; j < SP_SIZE - i; j++) {
            remaining[j] = remaining[j+1];
        }
    }

    return bad_codings;
}

// returns the score
int score_a_position(sp_pos_t *pos) {
    int score = 0;
    int i;
    for (i = 0; i < SP_SIZE; i++) {
        if (pos->positions[i] == asc_seq[i]) score++;
    }
    return score;
}

void sp_pos__pos_to_buffer(sp_pos_t *obj, char* buf_sp) {
    memcpy(obj->positions, buf_sp, SP_SIZE);
}

void sp_pos__buffer_to_pos(sp_pos_t *obj, char* buf_sp) {
    memcpy(buf_sp, obj->positions, SP_SIZE);
}

// returns number of moves, moves_out must point to 4 uninitialized sp_pos_t.
// will return either 2, 3, or 4
int legal_moves(sp_pos_t *moves_out, sp_pos_t *position) {
    char *scratch;
    int hole, swap;
    int num_moves = 0;
    int i;

    for (i = 0; i < SP_SIZE; i++) {
        if (position->positions[i] == 0) { hole = i; break; }
    }

    if (hole > 3) { // can move up
        scratch = moves_out[num_moves].positions;
        swap = (hole - 4) % SP_SIZE;
        sp_pos__pos_to_buffer(position, scratch);
        scratch[hole] = scratch[swap];
        scratch[swap] = 0;
        sp_pos__fill_in_ordinal(&moves_out[num_moves]);
        num_moves++;
    }

    if (hole < 12) { // can move down
        scratch = moves_out[num_moves].positions;
        swap = (hole + 4) % SP_SIZE;
        sp_pos__pos_to_buffer(position, scratch);
        scratch[hole] = scratch[swap];
        scratch[swap] = 0;
        sp_pos__fill_in_ordinal(&moves_out[num_moves]);
        num_moves++;
    }

    if ((hole % 4) != 0) { // can move left
        scratch = moves_out[num_moves].positions;
        swap = (hole - 1) % SP_SIZE;
        sp_pos__pos_to_buffer(position, scratch);
        scratch[hole] = scratch[swap];
        scratch[swap] = 0;
        sp_pos__fill_in_ordinal(&moves_out[num_moves]);
        num_moves++;
    }

    if ((hole % 4) != 3) { // can move right
        scratch = moves_out[num_moves].positions;
        swap = (hole + 1) % SP_SIZE;
        sp_pos__pos_to_buffer(position, scratch);
        scratch[hole] = scratch[swap];
        scratch[swap] = 0;
        sp_pos__fill_in_ordinal(&moves_out[num_moves]);
        num_moves++;
    }

    return num_moves;
}

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

sp_ordinal_t sp_tabent__decorate_ordinal(const sp_ordinal_t data) {
    sp_pos_t position;
    sp_ordinal_t rv, score;

    position.ordinal = data;
    sp_pos__fill_in_positions(&position);
    score = score_a_position(&position);
    score <<= pos_shifts[SP_SIZE-1];

    rv = data | score;
    return rv;
}


