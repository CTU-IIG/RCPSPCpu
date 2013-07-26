#ifndef HLIDAC_PES_ADVANCE_TABU_LIST_H
#define HLIDAC_PES_ADVANCE_TABU_LIST_H

/*!
 * \file AdvancedTabuList.h
 * \author Libor Bukata
 * \brief Advanced tabu list implementation.
 */

#include <iostream>
#include <list>
#include <vector>
#include <unordered_set>
#include <stdint.h>
#include "TabuList.h"
#include "ConfigureRCPSP.h"

/*!
 * \struct BaseElement
 * \brief Base structure of move. Unordered set element.
 */
struct BaseElement {
	//! Index, activity id, or something else.
	uint32_t i;
	//! Index, activity id, or something else.
	uint32_t j;
	//! Type of the performed move.
	MoveType type;
};

/*!
 * \struct BaseElementHash
 * \brief Helper structure for unordered set.
 */
struct BaseElementHash {
	/*!
	 * \param x Description of move.
	 * \return Hash value of move.
	 * \brief Compute hash value of specified move.
	 */
	size_t operator()(const BaseElement& x)	const {
		std::hash<uint64_t> hash_ptr;
		uint64_t val = x.i+(((uint64_t) x.j)<<32);
		return hash_ptr(val)+x.type;
	}
};

/*!
 * \struct BaseElementEqual
 * \brief Helper structure for unordered set.
 */
struct BaseElementEqual {
	/*!
	 * \param x Description of move.
	 * \param y Description of move.
	 * \return True if moves x and y are equal else false.
	 * \brief Equal operator required for unordered set.
	 */
	bool operator()(const BaseElement& x, const BaseElement& y) const {
		if (x.i == y.i && x.j == y.j && x.type == y.type)
			return true;
		else
			return false;
	}
};

/*!
 * \struct ListElement
 * \brief Item of advanced tabu list.
 */
struct ListElement : public BaseElement {
	//! Life expectancy of the move at the tabu list. Greater number - longer life. 
	//! Decreased by time. When counter reach zero then item is erased from tabu list.
	uint32_t lifeCounter;
};

//! Shorter notation of unordered_set class.
typedef std::unordered_set<BaseElement,BaseElementHash,BaseElementEqual> TabuHash;

/*!
 * Advanced tabu list adds randomisation and variable tabu list size. 
 * \class AdvancedTabuList
 * \brief Implementation of more sophisticated tabu list.
 */
class AdvancedTabuList : public TabuList {
	public:
		/*!
		 * \param maxIter Maximal number iterations since last best solution than randomisation of tabu list will be called.
		 * \brief Construct empty tabu list and tabu hash. Initialise base variables.
		 */
		AdvancedTabuList(const uint32_t& maxIter);

		/*!
		 * \param i Index, activity ID, or something else.
		 * \param j Index, activity ID, or something else.
		 * \param type Type of the move. Currently only swap and shift moves are supported.
		 * \return True if move is possible else false.
		 * \brief Check if move is at tabu hash and return negated result.
		 */
		virtual bool isPossibleMove(const uint32_t& i, const uint32_t& j, const MoveType& type) const;
		/*!
		 * \param i Index, activity ID, or something else.
		 * \param j Index, activity ID, or something else.
		 * \param type Type of the move. Currently only swap and shift moves are supported.
		 * \brief Add move to tabu list and tabu hash.
		 */
		virtual void addTurnToTabuList(const uint32_t& i, const uint32_t& j, const MoveType& type);
		//! Advanced tabu list is informed about new best solution. Randomisation purposes.
		virtual void bestSolutionFound();
		//! Inform tabu list about new iteration. Required for update lives of the tabu list elements and for randomisation.
		virtual uint32_t goToNextIter();
		//! Randomly erase some amount of the tabu list elements.
		virtual void prune();

		//! All allocated resources are automatically freed.
		virtual ~AdvancedTabuList() { }

	protected:
		
		//! Helper method that initialise base variables.
		void baseInit();
		//! Compute relative amount of erased tabu list elements at next iteration.
		void computeNextPtl();

	private:

		//! Number of iteration since best solution.	
		uint32_t iterSinceBest;
		//! Maximal number of iteration (without best solution found) until randomizeTabuList method will be called. (computed at constructor)
		const uint32_t maxIterSinceBest;	
		//! Relative amount of erased element per one iteration.	
		float ptl;
		//! Remainder from last erase iteration.
		float ptlRemain;
		//! Tabu list with variable size.
		std::list<ListElement> tabu;
		//! Iterator to current location at tabu list.
		std::list<ListElement>::iterator curPos;
		//! Tabu hash set - fast detection if tabu list item exists. Synchronisation with tabu list is required.
		TabuHash tabuHash;
		//! Tabu list state is saved for the best found solution.
		std::list<ListElement> bestTabu;
		//! Tabu hash state is saved for the best found solution.
		TabuHash bestTabuHash;
		//! Tabu list state is also saved for the second best solution.
		std::list<ListElement> secondBestTabu;
		//! Tabu hash state is also saved for the second best solution.
		TabuHash secondBestTabuHash;
};

#endif

