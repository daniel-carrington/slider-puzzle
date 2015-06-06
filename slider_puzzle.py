
import collections as cl
import itertools as it
import logging as lgg

log = lgg.getLogger('slider_puzzle')

# slider puzzle model
# note that a 4x4 slider puzzle has 16! possible board positions (which I
# am not sure beforehand are fully-connected by legal moves), which is
# a rough total of 2e13 states -- too big to fit into 2014 era memory.
# Therefore, let's see what we can do WITHOUT storing the entire graph!

# This is an unconvential "solved" position. The '0' is the empty space.
finish_pos = (0,  1,  2,  3,
              4,  5,  6,  7,
              8,  9, 10, 11,
             12, 13, 14, 15,)

#                       up, down, left, right
legal_move_displacements = (-4, 4, -1, 1)

def legal_moves(position):
    '''Iterator for the legal moves which you can make starting at
    the tuple position given.'''
    hole = position.index(0)

    # legal to move: up, down, left, right
    dirs = [(hole > 3), (hole < 12), (hole % 4 != 0), (hole % 4 != 3)]
    new_pos = ((hole + displacement) % 16 for legal, displacement
                in zip(dirs, legal_move_displacements) if legal)
    for idx, pos in enumerate(new_pos):
        tmp = list(position)
        tmp[pos], tmp[hole] = tmp[hole], tmp[pos]
        yield (position, tuple(tmp))

def legal_moves2(position):
    '''Iterator which returns more legal moves than legal_moves'''
    hole = position.index(0)

    for init_dir, first_2dir, second_2dir in \
                       ((try_move_up, try_move_left, try_move_right),
                        (try_move_right, try_move_up, try_move_down),
                        (try_move_down, try_move_right, try_move_left),
                        (try_move_left, try_move_up, try_move_down)):
        init_moves = list(move_until_fail(init_dir, position, hole))
        last_mainpos = position
        for newpos, newhole in init_moves:
            yield (last_mainpos, newpos)
            last_mainpos = newpos
            last_1_2dir_pos = last_mainpos
            last_2_2dir_pos = last_mainpos
            for nextpos, ignore in move_until_fail(first_2dir, newpos, newhole):
                yield (last_1_2dir_pos, nextpos)
                last_1_2dir_pos = nextpos
            for nextpos, ignore in move_until_fail(second_2dir, newpos, newhole):
                yield (last_2_2dir_pos, nextpos)
                last_2_2dir_pos = nextpos

def move_until_fail(move_fnc, position, hole_idx):
    while True:
        tmp = move_fnc(position, hole_idx)
        if tmp == None:
            raise StopIteration
        else:
            yield tmp
        (position, hole_idx) = tmp

def try_move_up(position, hole_idx):
    '''tries to move up or returns None; translates by -4'''
    if hole_idx <= 3:
        return None
    else:
        tmp = list(position)
        tmp[hole_idx - 4], tmp[hole_idx] = tmp[hole_idx], tmp[hole_idx - 4]
        return [tuple(tmp), hole_idx - 4]

def try_move_down(position, hole_idx):
    '''tries to move down or returns None; translates by +4'''
    if hole_idx >= 12:
        return None
    else:
        tmp = list(position)
        tmp[hole_idx + 4], tmp[hole_idx] = tmp[hole_idx], tmp[hole_idx + 4]
        return [tuple(tmp), hole_idx + 4]

def try_move_right(position, hole_idx):
    '''tries to move right or returns None; translates by +1'''
    if hole_idx % 4 == 3:
        return None
    else:
        tmp = list(position)
        tmp[hole_idx + 1], tmp[hole_idx] = tmp[hole_idx], tmp[hole_idx + 1]
        return [tuple(tmp), hole_idx + 1]

def try_move_left(position, hole_idx):
    '''tries to move up or returns None; translates by -1'''
    if hole_idx % 4 == 0:
        return None
    else:
        tmp = list(position)
        tmp[hole_idx - 1], tmp[hole_idx] = tmp[hole_idx], tmp[hole_idx - 1]
        return [tuple(tmp), hole_idx - 1]


int_pos_shifts = list((4*x         for x in range(8))) +\
                 list((4*8+3*x     for x in range(4))) +\
                 list((4*8+3*4+2*x for x in range(2))) +\
                 [4*8+3*4+2*2, 0]

# list of bitmasks!
int_pos_widths = [0b1111] * 8 + [0b111] * 4 + [0b11] * 2 + [0b1, 0]

def int_from_pos(position):
    '''Convert a tuple position into an integer representing the unique
    permutation represented by the position.'''
    remaining = list(range(16))
    out = 0
    for idx, num in enumerate(position):
        code = remaining.index(num)
        out |= code << int_pos_shifts[idx]
        del remaining[code]
    return out

def pos_from_int(i):
    '''blah blah opposite of int_from_pos()'''
    remaining = list(range(16))
    ls = []
    for num in range(16):
        pos = int_pos_widths[num] & (i >> int_pos_shifts[num])
        ls.append(remaining[pos])
        del remaining[pos]
    return tuple(ls)

def tuple_position(pos):
    if type(pos) == tuple: return pos
    if type(pos) in [int, long]:
        return pos_from_int(pos)
    raise TypeError("pos must be tuple or int/long")

def int_position(pos):
    if type(pos) in [int, long]: return pos
    if type(pos) == tuple:
        return int_from_pos(pos)
    raise TypeError("pos must be tuple or int/long")

def score_a_position(pos_, target_pos=None):
    pos = tuple_position(pos_)
    if target_pos is None:
        target_pos = finish_pos
    score = 0
    for p1, p2 in zip(pos, target_pos):
        score += 1 if p1 == p2 else 0

    if score == 16: score = 20000 # effectively infinite
    return score

class SearchCursor(object):
    '''A SearchCursor can explore states of a game according to a minmax
    strategy given an initial position and crucial information about the types
    used. The exact type requirements are:

        A function which converts a position to a key object or other hashable
            object, 'hs_pos' (hashable position)

        A function which converts a position to a type which is suitable for
            passing into an iterator, 'it_pos' (iterable position)

        You also need a scoring function 'score'. Higher is better.

        You also need a moves function 'moves', which must be an iterator
        returning positions which are legal moves from another position.

        Then you can use the "search" function to get destination positions,
        and get the moves you need using get_paths_back(). Those destination
        positions can be sent into a new SearchCursor to continue...
    '''

    # This class assumes that:
    # Calling moves is more expensive than
    #    (calling 'hs_pos' + doing a dict lookup)
    # Calling score is more expensive than
    #    (
    performance_assumptions = True

    def __init__(self, root_pos_, hs_pos=None, it_pos=None):
        self.hs_pos = hs_pos if hs_pos is not None else int_position
        self.it_pos = it_pos if it_pos is not None else tuple_position
        self.root_pos = self.it_pos(root_pos_)

    def define_connectivity(self, score=None, moves=None):
        self.score = score if score is not None else score_a_position
        self.moves = moves if moves is not None else legal_moves

    def adjacencies(self, mapping, iter_keys):
        for key in iter_keys:
            for derived in mapping[iter_keys]:
                yield derived

    def search(self, mindepth, candidates):
        '''Given mindepth, searches that number of steps into the game and
        returns a dictionary of positions mapping to the steps taken
        to get to that position. The number 'candidates' is the number of
        positions returned.
        '''
        if mindepth < 1:
            raise ValueError("mindepth must be 1 or larger")
        hs_root = self.hs_pos(self.root_pos)
        next_gen = [self.it_pos(self.root_pos)]
        nodes_seen = set()
        paths_back = dict()
        # O(mindepth * len(avg moves(node)) ** n)
        for i in range(mindepth):
            # let's do one 'step' into the game by calling .moves() once on all
            # of the positions we're currently considering
            log.debug('search depth %d of %d', i, mindepth)
            current_gen = next_gen
            next_gen = []
            for prev_node_pos, next_node_pos in \
                        it.chain.from_iterable(map(self.moves, current_gen)):
                        # i.e. for each in current_gen, use moves to get an iterable,
                        # and chain all those iterables together to see all of the moves
                        # that can be seen from the element in current_gen
                hs_next = self.hs_pos(next_node_pos)
                hs_prev = self.hs_pos(prev_node_pos)
                if hs_next not in nodes_seen:
                    nodes_seen.add(hs_next)
                    paths_back[hs_next] = hs_prev
                    next_gen.append(self.it_pos(next_node_pos))
                pass # for next_node
            pass # for i
        # figure out which one(s) are the best, O(n * (y + log n))
        # for y = mindepth, n = candidates
        best_ones = sorted(paths_back.keys(), key=self.score)
        best_ones = best_ones[-candidates:]
        log.debug('best_ones = %s', str(best_ones))
        rv = dict()
        for key in map(self.hs_pos, best_ones):
            path_there = cl.deque()
            rv[key] = path_there
            step = self.hs_pos(key)
            while step != hs_root:
                next = paths_back[step]
                path_there.appendleft(next)
                step = next
        return rv

class NestedLoopStats:
    def __init__(self):
        self.spin_count = 0
        self.last_found_spin = 0
        self.num_found = 0
        self.est_found = 0
        self.last_num_found = 0

    def find(self, count=1):
        self.last_found_spin = self.spin_count
        self.num_found += count

    def spin(self):
        self.spin_count += 1

def estimate(explosion, moves=legal_moves, runtime__s=15):
    import signal as s
    import datetime as dt
    global time_left
    time_left = True
    def time_is_expired(signum, frame):
        print "Timer expired."
        global time_left
        time_left = False
    st = dt.datetime.now() # START
    s.signal(s.SIGALRM, time_is_expired)
    s.alarm(runtime__s)  # runtime in seconds before alarm
    start = (0, 1, 2, 3,
             4, 5, 6, 7,
             8, 9,10,11,
            12,13,14,15)

    unused = cl.deque([[start]])
    seen = set()
    last_found_spin = 0
    exact_count = 0
    last_exact = 0
    inexact = 0
    while time_left:
        next_list = unused.pop()
        for node in next_list:
            hs_node = int_position(node)
            if not time_left:
                seen.add(int_position(node))
                continue
            if hs_node not in seen:
                seen.add(hs_node)
                more_nodes = [(node, node)]
                added_nodes = set()
                for i in range(explosion):
                    new_nodes = ()
                    for o, n in more_nodes:
                        new_nodes = it.chain(new_nodes, moves(n))
                        added_nodes.add(n)
                    more_nodes = new_nodes
                    pass # end range(explosion)
                unused.append(list(added_nodes))
                unused.append([n for o, n in more_nodes])
                seen.update(added_nodes)
                inexact += 1
        last_exact = exact_count
        exact_count = len(seen)
        tmp = len(unused)
        tmp_inexact = exact_count + tmp * len(unused[(tmp+1)/2])
        inexact = max(tmp_inexact, inexact - tmp)
        #print 'cleared a next_list found=%14d' % exact_count
    mt = dt.datetime.now()  # MIDDLE
    print "seen thus far: %d" % len(seen)
    for rest in unused:
        for node in rest:
            seen.add(int_position(node))
    exact_count = len(seen)
    et = dt.datetime.now()  # END
    print "time elapsed: start->mid=%d mid->end=%d start->end=%d" % (
        (mt - st).seconds, (et - mt).seconds, (et - st).seconds)
    return exact_count

def pp_tuple(tup):
    if len(tup) != 16:
        raise ValueError("tup must have len == 16")
    for i in range(4):
        print "[ %2d %2d %2d %2d ]" % tuple(tup[i*4:i*4+4])

def main(depth, candidates):
    search = SearchCursor((3, 4, 5, 6,
                           0, 1, 2, 7,
                           8,13,10,15,
                          12, 9,14,11)) # random input for now
    search.define_connectivity()

    print search.search(depth, candidates)

if __name__ == '__main__':
    main(8, 10)
