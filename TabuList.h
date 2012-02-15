#ifndef HLIDAC_PES_TABU_LIST_H
#define HLIDAC_PES_TABU_LIST_H

#include <stdint.h>
#include "ConstantsRCPSP.h"

class TabuList	{
	public:
		TabuList() { };
		virtual bool isPossibleMove(const uint32_t& i, const uint32_t& j, const MoveType& type) const = 0;	
		virtual void addTurnToTabuList(const uint32_t& i, const uint32_t& j, const MoveType& type) = 0; 
		virtual void bestSolutionFound() { }; 
		virtual uint32_t goToNextIter() { return 0; }; 
		virtual void randomizeTabuList() { };
		virtual ~TabuList() { };
};

#endif

