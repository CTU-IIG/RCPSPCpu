/*
	This file is part of the RCPSPCpu program.

	RCPSPCpu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	RCPSPCpu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with RCPSPCpu. If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdint.h>
#include "ConfigureRCPSP.h"

namespace ConfigureRCPSP {
	/* TABU LIST SETTINGS */
	TabuType TABU_LIST_TYPE = TABU_LIST;
	uint32_t SIMPLE_TABU_LIST_SIZE = TABU_LIST_SIZE;
	double ADVANCED_TABU_RANDOMIZE_ERASE_AMOUNT = RANDOMIZE_ERASE_AMOUNT;
	uint32_t ADVANCED_TABU_SWAP_LIFE = SWAP_LIFE;
	uint32_t ADVANCED_TABU_SHIFT_LIFE = SHIFT_LIFE;
	/* SCHEDULE SOLVER SETTINGS */
	uint32_t NUMBER_OF_ITERATIONS = DEFAULT_NUMBER_OF_ITERATIONS;
	uint32_t MAXIMAL_NUMBER_OF_ITERATIONS_SINCE_BEST = DEFAULT_MAXIMAL_NUMBER_OF_ITERATIONS_SINCE_BEST;
	uint32_t SWAP_RANGE = DEFAULT_SWAP_RANGE;
	uint32_t SHIFT_RANGE = DEFAULT_SHIFT_RANGE;
	uint32_t DIVERSIFICATION_SWAPS = DEFAULT_DIVERSIFICATION_SWAPS;
	bool WRITE_GRAPH = (DEFAULT_WRITE_GRAPH == 1 ? true : false);
	bool WRITE_RESULT_FILE = (DEFAULT_WRITE_RESULT_FILE == 1 ? true : false);
}

