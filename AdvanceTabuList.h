#ifndef HLIDAC_PES_ADVANCE_TABU_LIST_H
#define HLIDAC_PES_ADVANCE_TABU_LIST_H

#include <iostream>
#include <list>
#include <vector>
#include <unordered_set>
#include <stdint.h>
#include "TabuList.h"
#include "ConfigureRCPSP.h"

struct BaseElement {
	uint32_t i,j;
	MoveType type;
};

struct BaseElementHash {
	size_t operator()(const BaseElement& x)	const {
		std::hash<uint64_t> hash_ptr;
		uint64_t val = x.i+(((uint64_t) x.j)<<32);
		return hash_ptr(val)+x.type;
	}
};

struct BaseElementEqual {
	bool operator()(const BaseElement& x, const BaseElement& y) const {
		if (x.i == y.i && x.j == y.j && x.type == y.type)
			return true;
		else
			return false;
	}
};

struct ListElement : public BaseElement {
	uint32_t liveCounter;
};

typedef std::unordered_set<BaseElement,BaseElementHash,BaseElementEqual> TabuHash;

class AdvanceTabuList : public TabuList {
	public:
		AdvanceTabuList(const uint32_t& maxIter);

		virtual bool isPossibleMove(const uint32_t& i, const uint32_t& j, const MoveType& type) const;
		virtual void addTurnToTabuList(const uint32_t& i, const uint32_t& j, const MoveType& type);
		virtual void bestSolutionFound();
		virtual uint32_t goToNextIter();
		virtual void randomizeTabuList();

		virtual ~AdvanceTabuList() { }

	protected:
		
		void baseInit();
		void computeNextPtl();

	private:

		// Number of iteration since best solution.	
		uint32_t iterSinceBest;
		// Maximal number of iteration (without best solution found) until randomizeTabuList method will be called. (computed at constructor)
		const uint32_t maxIterSinceBest;	
		// Percentage amount of erased element per one iteration.	
		float ptl;
		// Remain from last erase iteration.
		float ptlRemain;
		// Tabu list with variable size.
		std::list<ListElement> tabu;
		// Pointer to current location in tabu list.
		std::list<ListElement>::iterator curPos;
		// Tabu hash set - fast locate if tabu element exists. Need to be synced with tabu list.
		TabuHash tabuHash;
		// Tabu list state is remebered for 2 best solution.
		std::list<ListElement> bestTabu, secondBestTabu;
		TabuHash bestTabuHash, secondBestTabuHash;
};

#endif

