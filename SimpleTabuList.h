#ifndef HLIDAC_PES_SIMPLE_TABU_LIST_H
#define HLIDAC_PES_SIMPLE_TABU_LIST_H

#include <stdint.h>
#include "TabuList.h"

struct ListRecord {
	int32_t i;
	int32_t j;
};

class SimpleTabuList : public TabuList {
	public:
		SimpleTabuList(const uint32_t& numberOfActivities, const uint32_t& length);

		virtual bool isPossibleMove(const uint32_t& i, const uint32_t& j, const MoveType&) const;
		virtual void addTurnToTabuList(const uint32_t& i, const uint32_t& j, const MoveType&);

		virtual ~SimpleTabuList();

	private:

		SimpleTabuList(const SimpleTabuList&);
		SimpleTabuList& operator=(const SimpleTabuList&);

		uint32_t curIdx;
		ListRecord * tabu;	
		bool **tabuSearch;
		const uint32_t tabuLength;
		const uint32_t totalNumberOfActivities;
};

#endif

