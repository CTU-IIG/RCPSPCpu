#ifndef HLIDAC_PES_CONFIGURE_RCPSP_H
#define HLIDAC_PES_CONFIGURE_RCPSP_H

#define SIMPLE_TABU

/* SCHEDULE SOLVER */
#define SWAP_RANGE 20
#define SHIFT_RANGE 0

#define PRECEDENCE_PENALTY 4

/* SIMPLE TABU LIST */
#define SIMPLE_TABU_LIST_SIZE 800

/* TABU LIST */
#define RANDOMIZE_ERASE_AMOUNT 0.5

// Possible moves in space.
enum MoveType {
	SWAP,SHIFT,NONE
};

#define SWAP_ITER 80
#define SHIFT_ITER 120

#endif

