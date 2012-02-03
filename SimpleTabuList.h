#ifndef HLIDAC_PES_SIMPLE_TABU_LIST_H
#define HLIDAC_PES_SIMPLE_TABU_LIST_H

#include <stdint.h>

struct ListRecord {
	int32_t i;
	int32_t j;
};

class SimpleTabuList {
	public:
		SimpleTabuList(const uint32_t numberOfActivities, const uint32_t length);

		bool isPossibleMove(const uint32_t i, const uint32_t j) const;
		void addTurnToTabuList(const uint32_t i, const uint32_t j);

		~SimpleTabuList();

	private:

		SimpleTabuList(const SimpleTabuList&);
		SimpleTabuList& operator=(const SimpleTabuList&);

		uint32_t curIdx;
		ListRecord* tabu;	
		bool** tabuSearch;
		uint32_t tabuLength;
		uint32_t totalNumberOfActivities;
};

#endif

