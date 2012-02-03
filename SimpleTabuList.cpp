#include "SimpleTabuList.h"

using namespace std;

SimpleTabuList::SimpleTabuList(const uint32_t numberOfActivities, const uint32_t length) : curIdx(0), tabuLength(length), totalNumberOfActivities(numberOfActivities)	{
	/* INIT SEARCH ARRAY */
	tabuSearch = new bool*[numberOfActivities];
	for (bool **ptr1 = tabuSearch; ptr1 < tabuSearch+numberOfActivities; ++ptr1)	{
		*ptr1 = new bool[numberOfActivities];
		for (bool *ptr2 = *ptr1; ptr2 < *ptr1+numberOfActivities; ++ptr2)
			*ptr2 = false;
	}

	/* INIT TABU LIST */
	tabu = new ListRecord[tabuLength];
	for (ListRecord* ptr = tabu; ptr < tabu+tabuLength; ++ptr)	{
		ptr->i = ptr->j = -1;
	}
}

bool SimpleTabuList::isPossibleMove(const uint32_t i, const uint32_t j)	const	{
	if (tabuSearch[i][j] == false && tabuSearch[j][i] == false)
		return true;
	else
		return false;
}

void SimpleTabuList::addTurnToTabuList(const uint32_t i, const uint32_t j)	{
	if (tabu[curIdx].i != -1 && tabu[curIdx].j != -1)
		tabuSearch[tabu[curIdx].i][tabu[curIdx].j] = tabuSearch[tabu[curIdx].j][tabu[curIdx].i] = false;

	tabu[curIdx].i = i;
	tabu[curIdx].j = j;
	tabuSearch[i][j] = tabuSearch[j][i] = true;

	curIdx = (curIdx+1) % tabuLength;
}

SimpleTabuList::~SimpleTabuList()	{
	delete[] tabu;
	for (bool **ptr = tabuSearch; ptr < tabuSearch+totalNumberOfActivities; ++ptr)	{
		delete[] *ptr;
	}
	delete[] tabuSearch;
}

