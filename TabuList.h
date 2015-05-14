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
#ifndef HLIDAC_PES_TABU_LIST_H
#define HLIDAC_PES_TABU_LIST_H

/*!
 * \file TabuList.h
 * \author Libor Bukata
 * \brief Abstract class definition of tabu list.
 */

#include <stdint.h>
#include "ConstantsRCPSP.h"

/*!
 * Tabu list abstract class. Define necessary tabu list methods as isPossibleMove and addTurnToTabuList.
 * Optional methods are bestSolutionFound, goToNextIter, randomizeTabuList. If optional method is unsupported
 * by concrete tabu list implementation, than default empty implementation is used.
 * \class TabuList
 * \brief Tabu list abstract class.
 */
class TabuList	{
	public:
		//! Implicit constructor of TabuList class.
		TabuList() { };
		/*!
		 * \param i Index, activity identification, etc.
		 * \param j Index, activity identification, etc.
		 * \param type Type of the move. Currently are supported swap or shift moves.
		 * \return True if move is possible (not in tabu list) else false.
		 * \brief Check if move is permited. 
		 */
		virtual bool isPossibleMove(const uint32_t& i, const uint32_t& j, const MoveType& type) const = 0;	
		/*!
		 * \param i Index, activity identification, etc.
		 * \param j Index, activity identification, etc.
		 * \param type Type of the move. Currently are supported swap or shift moves.
		 * \brief Add move (specified by i,j,type) to tabu list.
		 */
		virtual void addTurnToTabuList(const uint32_t& i, const uint32_t& j, const MoveType& type) = 0; 
		//! Inform tabu list about new best solution.
		virtual void bestSolutionFound() { }; 
		/*!
		 * Tell tabu list about end of iteration.
		 * \return Number of erased tabu list items.
		 */
		virtual uint32_t goToNextIter() { return 0; }; 
		//! The method removes some tabu moves randomly since all solutions in neighbourhood were tabu.
		virtual void prune() = 0;
		//! Virtual destructor of TabuList class.
		virtual ~TabuList() { };
};

#endif

