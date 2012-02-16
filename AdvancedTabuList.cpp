#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <stdint.h>
#include "AdvancedTabuList.h"

using namespace std;

AdvancedTabuList::AdvancedTabuList(const uint32_t& maxIter) : maxIterSinceBest(maxIter) {
	baseInit();
}

bool AdvancedTabuList::isPossibleMove(const uint32_t& i, const uint32_t& j, const MoveType& type) const {
	struct BaseElement f1 = { i, j, type };
	if (tabuHash.find(f1) == tabuHash.end())
		return true;
	else
		return false;
}

void AdvancedTabuList::addTurnToTabuList(const uint32_t& i, const uint32_t& j, const MoveType& type)	{
	uint32_t lifeFactor;
	switch (type)	{
		case SWAP:
			lifeFactor = ConfigureRCPSP::ADVANCED_TABU_SWAP_LIFE;
			break;
		case SHIFT:
			lifeFactor = ConfigureRCPSP::ADVANCED_TABU_SHIFT_LIFE;
			break;
		default:
			throw runtime_error("TabuList::addTurnToTabuList: Unsupported move type!");	
	}

	struct BaseElement ab = { i, j, type };
	struct ListElement al;
	al.i = i; al.j = j; al.type = type;
	al.lifeCounter = lifeFactor;

	pair<TabuHash::iterator,bool> ret = tabuHash.insert(ab);
	if (ret.second == false)	{
		string error = "TabuList::addTurnToTabuList: Failed to insert to TabuHash!\n\tInvalid manipulation with tabu list, probably duplicate keys found!";
		throw runtime_error(error);
	}
	
	// Insert record before actual possition.
	tabu.insert(curPos,al);
}

void AdvancedTabuList::bestSolutionFound()	{
	if (!bestTabu.empty())	{
		secondBestTabu = bestTabu;
		secondBestTabuHash = bestTabuHash;
	}
	bestTabu = tabu;
	bestTabuHash = tabuHash;

	iterSinceBest = 0;

	return;
}

uint32_t AdvancedTabuList::goToNextIter()	{

	if (iterSinceBest > maxIterSinceBest)	{
		// Current location in space is not suitable for improving current solution.
		// Change location in space by randomizing tabu list.
		randomizeTabuList();
	}

	uint32_t erasedItems = 0;
	float sizeOfRange = ptl*tabu.size()+ptlRemain;
	size_t nmbrOfElements = (size_t) sizeOfRange;

	for (size_t i = 0; (i < nmbrOfElements) && !(tabu.empty()); ++i)	{
		if (curPos == tabu.end())	{
			curPos = tabu.begin();
		}
		curPos->lifeCounter--;	
		if (curPos->lifeCounter == 0)	{
			struct BaseElement eb = { curPos->i, curPos->j, curPos->type };
			if (tabuHash.erase(eb) != 1)	{
				throw runtime_error("TabuList::goToNextIter: Invalid number of erased element!\n\tInconsistent tabu list or tabu hash.");
			}
			curPos = tabu.erase(curPos);
			++erasedItems;
		} else {
			++curPos;
		}
	}

	computeNextPtl();
	ptlRemain = sizeOfRange-nmbrOfElements;
	++iterSinceBest;

	return erasedItems;
}

void AdvancedTabuList::randomizeTabuList()	{
	if (!secondBestTabu.empty())	{
		tabu = secondBestTabu;
		tabuHash = secondBestTabuHash;
	} else if (!bestTabu.empty())	{
		tabu = bestTabu;
		tabuHash = bestTabuHash;
	}

	size_t erasedElements = tabu.size()*ConfigureRCPSP::ADVANCED_TABU_RANDOMIZE_ERASE_AMOUNT;

	vector<size_t> eraseIdxs;
	for (size_t i = 0; i < tabu.size(); ++i)	{
		eraseIdxs.push_back(i);
	}

	random_shuffle(eraseIdxs.begin(), eraseIdxs.end());
	eraseIdxs.resize(erasedElements);
	sort(eraseIdxs.begin(),eraseIdxs.end());

	size_t listIdx = 0;
	list<ListElement>::iterator it = tabu.begin();
	vector<size_t>::const_iterator eit = eraseIdxs.begin();
	while ((it != tabu.end()) && (eit != eraseIdxs.end()))	{
		if (*eit == listIdx)	{
			struct BaseElement ee = { it->i, it->j, it->type };
			if (tabuHash.erase(ee) != 1)	{
				throw runtime_error("TabuList::randomizeTabuList: Invalid number of erased element!\n\tInconsistent tabu list or tabu hash.");
			}
			it = tabu.erase(it); ++eit;
		} else {
			++it;
		}
		++listIdx;
	}

	baseInit();

	return;
}

void AdvancedTabuList::baseInit()	{
	curPos = tabu.begin();
	ptlRemain = 0.0;
	iterSinceBest = 0;
	computeNextPtl();
}

void AdvancedTabuList::computeNextPtl()	{
	float phase = ((float) iterSinceBest)/((float) maxIterSinceBest);
	// 1/(e^(-8*x+4)+1)
	ptl = 1./(exp(-8*phase+4)+1);
	return;
}

