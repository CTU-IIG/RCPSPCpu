#ifndef HLIDAC_PES_DEFAULT_CONFIGURE_RCPSP_H
#define HLIDAC_PES_DEFAULT_CONFIGURE_RCPSP_H

/*!
 * \file DefaultConfigureRCPSP.h
 * \author Libor Bukata
 * \brief Default settings for RCPSP.
 */

#include "ConstantsRCPSP.h"

/* DEFAULT PARAMETERS FOR RCPSP - YOU CAN CHANGE. */

/* TABU LIST */

//! Type of the tabu list: SIMPLE_TABU or ADVANCED_TABU
#define TABU_LIST SIMPLE_TABU
//! Simple tabu list size. Advanced tabu list isn't affected.
#define TABU_LIST_SIZE 800	// Good choice: 120 activities - 800; 30 activities - 60
//! Relative amount of the tabu list elements that are erased after maximal number of iterations since best solution. Only advanced tabu list. (value 0-1)
#define RANDOMIZE_ERASE_AMOUNT 0.3
//! Swap life factor. Larger number -> longer life of swap moves at a tabu list and vice versa. Supported by advanced tabu list.
#define SWAP_LIFE 80
//! Shift life factor. Larger number -> longer life of shift moves at a tabu list and vice versa. Supported by advanced tabu list.
#define SHIFT_LIFE 120

/* SCHEDULE SOLVER */

//! Tabu search iterations.
#define DEFAULT_NUMBER_OF_ITERATIONS 1000
//! Maximal number of iterations since best solution found. (diversification purposes)
#define DEFAULT_MAXIMAL_NUMBER_OF_ITERATIONS_SINCE_BEST 300
//! Define maximal distance of swapped activities.
#define DEFAULT_SWAP_RANGE 60
//! Define maximal shift range of any activity.
#define DEFAULT_SHIFT_RANGE 0
//! Number of diversification swaps.
#define DEFAULT_DIVERSIFICATION_SWAPS 10
//! Do you want to write makespan criterion graph (independent variable is iteration number). As a result you obtain csv file. (1 == true, 0 == false)
#define DEFAULT_WRITE_GRAPH 0

#endif

