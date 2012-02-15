#ifndef HLIDAC_PES_DEFAULT_CONFIGURE_RCPSP_H
#define HLIDAC_PES_DEFAULT_CONFIGURE_RCPSP_H

#include "ConstantsRCPSP.h"

/* DEFAULT PARAMETERS FOR RCPSP - YOU CAN CHANGE. */

/* TABU LIST */
// Type of the tabu list: SIMPLE_TABU or ADVANCE_TABU
#define TABU_LIST SIMPLE_TABU
// Simple tabu list size. Advance tabu list isn't affected.
#define TABU_LIST_SIZE 800	// Good choice: 120 activities - 800; 30 activities - 60
// Amount of the tabu list elements that are erased after maximal number of iterations since best solution. Only advance tabu list.	(value 0-1)
#define RANDOMIZE_ERASE_AMOUNT 0.3
// Swap live factor. Larger number -> longer live of swap moves at a tabu list and vice versa. Supported by advance tabu list.
#define SWAP_LIVE 80
// Shift live factor. Larger number -> longer live of shift moves at a tabu list and vice versa. Supported by advance tabu list.
#define SHIFT_LIVE 120

/* SCHEDULE SOLVER */
// Tabu search iterations.
#define DEFAULT_NUMBER_OF_ITERATIONS 1000
// Maximal number of iterations since best solution found. (diversification purposes)
#define DEFAULT_MAXIMAL_NUMBER_OF_ITERATIONS_SINCE_BEST 300
// Define maximal distance of swapped activities.
#define DEFAULT_SWAP_RANGE 60
// Define maximal shift range of any activity.
#define DEFAULT_SHIFT_RANGE 0
// Number of diversification swaps.
#define DEFAULT_DIVERSIFICATION_SWAPS 10


#endif

