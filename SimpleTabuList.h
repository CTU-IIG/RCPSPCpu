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
#ifndef HLIDAC_PES_SIMPLE_TABU_LIST_H
#define HLIDAC_PES_SIMPLE_TABU_LIST_H

/*!
 * \file SimpleTabuList.h
 * \author Libor Bukata
 * \brief Simple tabu list class definition.
 */

#include <stdint.h>
#include "TabuList.h"

/*!
 * \struct ListRecord
 * \brief Simple tabu list item.
 */
struct ListRecord {
	//! Index, activity identification or something else. Maximal value is total number of activities-1.
	int32_t i;
	//! Index, activity identification or something else. Maximal value is total number of activities-1.
	int32_t j;
};

/*!
 * Implementation of simple version of tabu list. Implement only required methods.
 * Effectiveness of items search is achieved by tabu hash.
 * Tabu list is implemented as a circular buffer with fixed size.
 * \class SimpleTabuList
 * \brief Simple tabu list implementation. Tabu hash and circular buffer are used.
 */
class SimpleTabuList : public TabuList {
	public:
		/*!
		 * \param numberOfActivities Total number of activities (or jobs).
		 * \param length Tabu list size. (circular buffer size)
		 * \brief Initialize tabu hash and tabu list elements.
		 */
		SimpleTabuList(const uint32_t& numberOfActivities, const uint32_t& length);

		/*!
		 * \param i Index, activity identification or something else.
		 * \param j Index, activity identification or something else.
		 * \return False if move is at the tabu hash else true.
		 * \brief Check if move is possible or not.
		 * \note Third parameter is ignored.
		 */
		virtual bool isPossibleMove(const uint32_t& i, const uint32_t& j, const MoveType&) const;
		/*!
		 * \param i Index, activity identification or something else.
		 * \param j Index, activity identification or something else.
		 * \brief Add move to simple tabu list. Update tabu list and tabu hash.
		 * \note Third parameter is ignored.
		 */
		virtual void addTurnToTabuList(const uint32_t& i, const uint32_t& j, const MoveType&);

		/*!
		 * If the tabu list does not allow to select any solution in the neighbourhood
		 * some random moves are required to be pruned from the tabu list. The method removes 30 % of the tabu list moves.
		 */
		virtual void prune();

		//! Free all allocated resources. (i.e. tabu list and tabu hash)
		virtual ~SimpleTabuList();

	private:

		//! Copy constructor is forbidden.
		SimpleTabuList(const SimpleTabuList&);
		//! Assignment operator is forbidden.
		SimpleTabuList& operator=(const SimpleTabuList&);

		//! Current index at tabu list. (circular buffer)
		uint32_t curIdx;
		//! Array of tabu list items. It is tabu list.
		ListRecord *tabu;	
		//! Tabu hash structure. It's two-dimensional array of boolean (size totalNumberOfActivities x totalNumberOfActivities).
		bool **tabuSearch;
		//! Fixed tabu list size.
		const uint32_t tabuLength;
		//! Number of activities read from instance file. Required for tabuSearch allocation.
		const uint32_t totalNumberOfActivities;
};

#endif

