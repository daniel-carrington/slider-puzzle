
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "slider_puzzle.h"

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

// Only initializes 'positions', doesn't fill in the ordinal.
void randomize_sp(sp_pos_t *obj) {
    char i;
    obj->positions = SP_ASC_SEQUENCE;
    for (i = 0; i < (SP_SIZE-1); i++) {
        char shuffle, tmp;
        shuffle = rand() % (SP_SIZE - i);
        tmp = obj->positions[i + shuffle];
        obj->positions[i + shuffle] = obj->positions[i];
        obj->positions[i] = tmp;
    }
}

