#ifndef OBJDITHER_H
#define OBJDITHER_H

/* threshold maps */
#define map(x) (x)
static const int map8[8*8] = {
map( 0), map(48), map(12), map(60), map( 3), map(51), map(15), map(63),
map(32), map(16), map(44), map(28), map(35), map(19), map(47), map(31),
map( 8), map(56), map( 4), map(52), map(11), map(59), map( 7), map(55),
map(40), map(24), map(36), map(20), map(43), map(27), map(39), map(23),
map( 2), map(50), map(14), map(62), map( 1), map(49), map(13), map(61),
map(34), map(18), map(46), map(30), map(33), map(17), map(45), map(29),
map(10), map(58), map( 6), map(54), map( 9), map(57), map( 5), map(53),
map(42), map(26), map(38), map(22), map(41), map(25), map(37), map(21)
};

static const int map4[4*4] = {
map( 0), map( 8), map( 2), map(10),
map(12), map( 4), map(14), map( 6),
map( 3), map(11), map( 1), map( 9),
map(15), map( 7), map(13), map( 5)
};

static const int map2[2*2] = {
map( 0), map( 2),
map( 3), map( 1)
};
#undef map

#endif // OBJDITHER_H
