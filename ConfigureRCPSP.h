#ifndef HLIDAC_PES_CONFIGURE_RCPSP_H
#define HLIDAC_PES_CONFIGURE_RCPSP_H

/*!
 * \file ConfigureRCPSP.h
 * \author Libor Bukata
 * \brief Represents a setting of RCPSP.
 */

#include <stdint.h>
#include "ConstantsRCPSP.h"
#include "DefaultConfigureRCPSP.h"

/*!
 * \namespace ConfigureRCPSP
 * \brief Configurable extern global variables are defined at this namespace.
 */
namespace ConfigureRCPSP {

	/* TABU LIST SETTINGS */

	//! Tabu list type.
	extern TabuType TABU_LIST_TYPE;
	//! Simple tabu list size.
	extern uint32_t SIMPLE_TABU_LIST_SIZE;
	//! Relative amount of erased elements of advanced tabu list when diversification take place.
	extern double ADVANCED_TABU_RANDOMIZE_ERASE_AMOUNT;
	//! Life expectancy of the swap move at the advanced tabu list.
	extern uint32_t ADVANCED_TABU_SWAP_LIFE;
	//! Life expectancy of the shift move at the advanced tabu list.
	extern uint32_t ADVANCED_TABU_SHIFT_LIFE;

	/* SCHEDULE SOLVER SETTINGS */

	//! Number of search iterations.
	extern uint32_t NUMBER_OF_ITERATIONS;
	//! Maximal number of iterations without improving of the best solution.
	extern uint32_t MAXIMAL_NUMBER_OF_ITERATIONS_SINCE_BEST;
	//! Maximal distance between swapped activities.
	extern uint32_t SWAP_RANGE;
	//! Maximal number of activities that could be skipped by other activity when shift move is performed.
	extern uint32_t SHIFT_RANGE;
	//! Number of diversification swaps.
	extern uint32_t DIVERSIFICATION_SWAPS;
	//! Do you want a write csv file? Makespan criterion dependent on number of iterations.
	extern bool WRITE_GRAPH;
}

#endif

