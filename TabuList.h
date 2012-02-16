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
		//! Prunning tabu list.
		virtual void randomizeTabuList() { };
		//! Virtual destructor of TabuList class.
		virtual ~TabuList() { };
};

#endif

