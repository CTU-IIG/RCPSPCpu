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
#ifndef HLIDAC_PES_CONSTANTS_RCPSP_H
#define HLIDAC_PES_CONSTANTS_RCPSP_H

/*!
 * \file ConstantsRCPSP.h
 * \author Libor Bukata
 * \brief Declare important constants for RCPSP.
 */

/* DEFINE BASIC RCPSP CONSTANTS - DON'T EDIT! */

//! Possible moves in a space.
enum MoveType {
	SWAP, SHIFT, NONE
};

//! Types of the tabu list.
enum TabuType {
	SIMPLE_TABU, ADVANCED_TABU 
};

//! Algorithm selection constants.
enum EvaluationAlgorithm {
	CAPACITY_RESOLUTION, TIME_RESOLUTION
};

#endif

