#ifndef HLIDAC_PES_CONFIGURE_RCPSP_H
#define HLIDAC_PES_CONFIGURE_RCPSP_H

#include <stdint.h>
#include "ConstantsRCPSP.h"
#include "DefaultConfigureRCPSP.h"

namespace ConfigureRCPSP {
	/* TABU LIST SETTINGS */
	extern TabuType TABU_LIST_TYPE;
	extern uint32_t SIMPLE_TABU_LIST_SIZE;
	extern double ADVANCE_TABU_RANDOMIZE_ERASE_AMOUNT;
	extern uint32_t ADVANCE_TABU_SWAP_LIVE;
	extern uint32_t ADVANCE_TABU_SHIFT_LIVE;
	/* SCHEDULE SOLVER SETTINGS */
	extern uint32_t NUMBER_OF_ITERATIONS;
	extern uint32_t MAXIMAL_NUMBER_OF_ITERATIONS_SINCE_BEST;
	extern uint32_t SWAP_RANGE;
	extern uint32_t SHIFT_RANGE;
	extern uint32_t DIVERSIFICATION_SWAPS;
}

#endif

