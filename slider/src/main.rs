
fn main() {
    let thing = Board::start();
    println!("A board is like {:?}", thing);
    let SomeBoard::Complete { bh, ba } = thing.myboard;
        println!("A boardhash is like {:?}", Board::array_to_hash(&ba));
        println!("A boardarray is like {:?}", Board::hash_to_array(&bh));
}

type BoardHash = u64;
type BoardArray = [i8; 16];

// fn board_array_eq (&salf: &BoardArray, &other: &BoardArray) -> bool {
//     (salf[0] == other[0]) &&
//     (salf[1] == other[1]) &&
//     (salf[2] == other[2]) &&
//     (salf[3] == other[3]) &&
//     (salf[4] == other[4]) &&
//     (salf[5] == other[5]) &&
//     (salf[6] == other[6]) &&
//     (salf[7] == other[7]) &&
//     (salf[8] == other[8]) &&
//     (salf[9] == other[9]) &&
//     (salf[10] == other[10]) &&
//     (salf[11] == other[11]) &&
//     (salf[12] == other[12]) &&
//     (salf[13] == other[13]) &&
//     (salf[14] == other[14]) &&
//     (salf[15] == other[15])
// }

#[derive(Debug, Copy, Clone)]
enum SomeBoard {
    // HashOnly { bh : BoardHash, },
    // ArrayOnly { ba : BoardArray, },
    Complete { bh : BoardHash, ba : BoardArray, },
}

#[derive(Debug)]
struct Board {
    myboard : SomeBoard,
    // previous : &'a Option<Board>,
}

const  START_BOARD    : [i8; 16] = [ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15];
static INT_POS_WIDTHS : [i8; 16] = [15, 15, 15, 15, 15, 15, 15, 15,  7,  7,  7,  7,  3,  3,  1,  0];
static INT_POS_SHIFTS : [i8; 16] = [ 0,  4,  8, 12, 16, 20, 24, 28, 32, 35, 38, 41, 44, 46, 48, 49];

#[test]
fn board_conversions_0() {
    let start_board = Board::start();
    {
        let SomeBoard::Complete { bh, ba } = start_board.myboard;
        assert_eq!(bh, 0);
        assert_eq!(Board::array_to_hash(&Board::hash_to_array(&bh)), bh);
        assert_eq!(Board::hash_to_array(&Board::array_to_hash(&ba)), ba);
        assert_eq!(Board::array_to_hash(&ba), bh);
        assert_eq!(Board::hash_to_array(&bh), ba);
    }
}

#[test]
fn board_conversions_1() {
    let end_ba : [i8; 16] = [15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0];
    let end_bh : u64 = 0x0001_b977_89ab_cdefu64;
        //          1     b     9     7     7    8    9    a    b    c    d    e    f
        // 0b000 0001 10_11 100_1 01_11 0_111 1000 1001 1010 1011 1100 1101 1110 1111
        //          1  2  3   4    5    6   7    8    9    a    b    c    d    e    f
    {
        let rt_bd = Board::hash_to_array(&Board::array_to_hash(&end_ba));
        assert_eq!(rt_bd, end_ba);

        assert_eq!(format!("0x{:013x}", Board::array_to_hash(&Board::hash_to_array(&end_bh))),
            format!("0x{:013x}", &end_bh));
    }
    {
        let eh = Board::array_to_hash(&end_ba);
        assert_eq!(format!("0x{:013x}", eh), format!("0x{:013x}", end_bh));
    }
    //assert_eq!();
}

fn num_previous_vals_used(i : u32, val_used : u32) -> i8 {
    (val_used & ((1u32 << i) - 1u32)).count_ones() as i8
}

fn get_nth_unused_val(n : i8, val_used : u32) -> i8 {
    let lo = (val_used + 1).trailing_zeros() as i8;
    let skipped_val_used = !((1u32 << lo) - 1u32) & val_used;
    let rv = _get_nth_unused_val(n, skipped_val_used, lo);
    // println!("get_nth_unused_val rv = {}", rv);
    rv
}

fn _get_nth_unused_val(n : i8, val_used : u32, previous : i8) -> i8 {
    // println!("get_nth_unused_val(n={}, val_used=0x{:04x}, previous={})", n, val_used, previous);
    let tz : i8 = (val_used.trailing_zeros() as i8) - previous;
    if tz >= n {
        n + previous
    }
    else {
        println!("n={} val_used=0x{:04x} tz={} previous={}", n, val_used, tz, previous);
        let next_val_used = (!(1u32 << tz as u32)) & val_used;
        _get_nth_unused_val(n, next_val_used, previous + 1)
    }
}

impl Board {
    fn start() -> Board {
        Board { myboard: SomeBoard::Complete { bh : 0, ba : START_BOARD } }
    }

    fn hash_to_array(&bh : &BoardHash) -> BoardArray {
        let mut rv : BoardArray = [0; 16];
        let mut val_used : u32 = 0;
        for i in 0 .. 16 {
            let hf : i8 =  INT_POS_WIDTHS.get(i).unwrap() &
                  ((&bh >> INT_POS_SHIFTS.get(i).unwrap()) as i8);
            let code : i8 = get_nth_unused_val(hf, val_used);

            println!("h2a: i = {:2}, hf = {:2}, code = {:2}, val_used = 0x{:04x}",
                  i, hf, code, val_used);

            debug_assert!((code >= 0) && (code <= 15));
            debug_assert!(((val_used & (1 << code)) == 0), "h2a value seen before;\n\
                    \tbh=0x{:013x} code={}, i={}, hf={}, val_used=0x{:04x}\n\
                    \trv={:?}",
                bh, code, i, hf, val_used,
                rv);
            rv[i] = code;
            val_used |= 1 << code;
        }
        rv
    }

    fn array_to_hash(&ba : &BoardArray) -> BoardHash {
        let mut rv : BoardHash = 0;
        let mut val_used : u32 = 0;
        for i in 0 .. 16 {
            debug_assert!(((ba[i] >= 0) && (ba[i] <= 15)),
                "i = {}, ba[i] = {}, should be in range 0..16", i, ba[i]);
            debug_assert!(((val_used & (1 << ba[i])) == 0),
                "value seen before: i={}, val_used=0x{:04x}, ba={:?}", i, val_used, ba);
            let code : i8 = (ba[i] as i8) - num_previous_vals_used(get_nth_unused_val(0, val_used) as u32, val_used);
            println!("rv = 0x{:013x}, ba[i] = {:2}, code = {:2}, val_used = 0x{:04x}", rv, ba[i], code, val_used);
            rv |= (code as u64).wrapping_shl(*INT_POS_SHIFTS.get(i).unwrap() as u32);
            val_used |= 1 << ba[i];
        }
        rv
    }
}

