
#ifndef DEFINE_INCLUDE_SLIDER_PUZZLE
#define DEFINE_INCLUDE_SLIDER_PUZZLE

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

const char asc_seq[SP_SIZE] = SP_ASC_SEQUENCE;

// expects a valid 'positions' in sp_pos_t, will initialize 'ordinal'
// returns number of errors
int sp_pos__fill_in_ordinal(sp_pos_t *obj);
// expects a valid 'ordinal' in sp_pos_t, will initialize 'positions'
// returns number of errors
int sp_pos__fill_in_positions(sp_pos_t *obj);

int sp_pos__score(sp_pos_t *pos);

void sp_pos__pos_to_buffer(sp_pos_t *obj, char* buf_sp);
void sp_pos__buffer_to_pos(sp_pos_t *obj, char* buf_sp);

// returns number of moves, moves_out must point to 4 uninitialized sp_pos_t.
// will return either 2, 3, or 4
int legal_moves(sp_pos_t *moves_out, sp_pos_t *position);

// Only initializes 'positions', doesn't fill in the ordinal.
void randomize_sp(sp_pos_t *obj);

#endif

