#include <algorithm>
#include <cstring>
#include "SourcesLoad.h"

using namespace std;

SourcesLoad::SourcesLoad(const uint32_t& numRes, const uint32_t * const& capRes) : numberOfResources(numRes), capacityOfResources(capRes)	{
	uint32_t maxCapacity = 0;
	resourcesLoad = new uint32_t*[numberOfResources];

	for (uint32_t resourceId = 0; resourceId < numberOfResources; ++resourceId)	{
		resourcesLoad[resourceId] = new uint32_t[capacityOfResources[resourceId]];
		memset(resourcesLoad[resourceId], 0, sizeof(uint32_t)*capacityOfResources[resourceId]);
		maxCapacity = max(capacityOfResources[resourceId],maxCapacity);
	}

	startValues = new uint32_t[maxCapacity];
	memset(startValues, 0, sizeof(uint32_t)*maxCapacity);
}

uint32_t SourcesLoad::getEarliestStartTime(const uint32_t * const& activityResourceRequirement)	const 	{
	uint32_t bestStart = 0;
	for (uint32_t resourceId = 0; resourceId < numberOfResources; ++resourceId)	{
		uint32_t activityRequirement = activityResourceRequirement[resourceId];
		if (activityRequirement > 0)
			bestStart = max(resourcesLoad[resourceId][capacityOfResources[resourceId]-activityRequirement], bestStart);
	}
	return bestStart;
}

void SourcesLoad::addActivity(const uint32_t& activityStart, const uint32_t& activityStop, const uint32_t * const& activityRequirement)	{
	#if DEBUG_SOURCES == 1
	map<uint32_t,int32_t*>::iterator mit;
	if ((mit = peaks.find(activityStart)) == peaks.end())	{
		int32_t *peak = new int32_t[numberOfResources];
		for (uint32_t idx = 0; idx < numberOfResources; ++idx)
			peak[idx] = -((int32_t) activityRequirement[idx]);
		peaks[activityStart] = peak;
	} else {
		int32_t *peak = mit->second;
		for (uint32_t idx = 0; idx < numberOfResources; ++idx)
			peak[idx] -= activityRequirement[idx];
	}

	if ((mit = peaks.find(activityStop)) == peaks.end())	{
		int32_t *peak = new int32_t[numberOfResources];
		for (uint32_t idx = 0; idx < numberOfResources; ++idx)
			peak[idx] = ((int32_t) activityRequirement[idx]);
		peaks[activityStop] = peak;
	} else {
		int32_t *peak = mit->second;
		for (uint32_t idx = 0; idx < numberOfResources; ++idx)
			peak[idx] += activityRequirement[idx];
	}
	uint32_t **resourcesLoadCopy = new uint32_t*[numberOfResources];
	for (uint32_t i = 0; i < numberOfResources; ++i)	{
		resourcesLoadCopy[i] = new uint32_t[capacityOfResources[i]];
		for (uint32_t j = 0; j < capacityOfResources[i]; ++j)
			resourcesLoadCopy[i][j] = resourcesLoad[i][j];
	}
	#endif

	int32_t requiredSquares, timeDiff;
	uint32_t k, c, capacityOfResource, resourceRequirement, baseResourceIdx, startTimePreviousUnit, newStartTime;
	for (uint32_t resourceId = 0; resourceId < numberOfResources; ++resourceId)	{
		capacityOfResource = capacityOfResources[resourceId];
		resourceRequirement = activityRequirement[resourceId];
		requiredSquares = resourceRequirement*(activityStop-activityStart);
		if (requiredSquares > 0)	{
			baseResourceIdx = capacityOfResource-resourceRequirement;
			startTimePreviousUnit = ((resourceRequirement < capacityOfResource) ? resourcesLoad[resourceId][baseResourceIdx-1] : activityStop);
			newStartTime = min(activityStop, startTimePreviousUnit);
			if (activityStart < startTimePreviousUnit)	{
				for (k = baseResourceIdx; k < capacityOfResource; ++k)	{
					resourcesLoad[resourceId][k] = newStartTime; 
				}
				requiredSquares -= resourceRequirement*(newStartTime-activityStart); 
			}
			c = 0; k = 0;
			newStartTime = activityStop;
			while (requiredSquares > 0 && k < capacityOfResource)	{
				if (resourcesLoad[resourceId][k] < newStartTime)    {
					if (c >= resourceRequirement)
						newStartTime = startValues[c-resourceRequirement];
					timeDiff = newStartTime-max(resourcesLoad[resourceId][k],activityStart);
					if (requiredSquares-timeDiff > 0)	{
						requiredSquares -= timeDiff;
						startValues[c++] = resourcesLoad[resourceId][k];
						resourcesLoad[resourceId][k] = newStartTime; 
					} else {
						resourcesLoad[resourceId][k] = newStartTime-timeDiff+requiredSquares; 
						break;
					}
				}
				++k;
			}
		}
	}

	#if DEBUG_SOURCES == 1
	vector<uint32_t> tim;	
	vector<int32_t*> cum;	

	int32_t *currentLoad = new int32_t[numberOfResources];
	memset(currentLoad,0,sizeof(int32_t)*numberOfResources);

	for (map<uint32_t,int32_t*>::const_reverse_iterator it = peaks.rbegin(); it != peaks.rend(); ++it)	{
		int32_t *peak = it->second;	
		int32_t* cumVal = new int32_t[numberOfResources];
		for (uint32_t idx = 0; idx < numberOfResources; ++idx)	{
			currentLoad[idx] += peak[idx];
			cumVal[idx] = currentLoad[idx];
		}
		tim.push_back(it->first);
		cum.push_back(cumVal);
	}

	for (uint32_t resourceId = 0; resourceId < numberOfResources; ++resourceId)	{
		uint32_t pos = 0;
		uint32_t capacityOfResource = capacityOfResources[resourceId];
		uint32_t *startTimes = new uint32_t[capacityOfResource];
		memset(startTimes, 0, sizeof(uint32_t)*capacityOfResource);

		vector<uint32_t>::const_iterator it1 = tim.begin(), eit1 = tim.end();
		vector<int32_t*>::const_iterator it2 = cum.begin(), eit2 = cum.end();

		while (it1 != eit1 && it2 != eit2)	{
			uint32_t curTime = *it1;
			int32_t curVal = (*it2)[resourceId];
			if (curVal > ((int32_t) capacityOfResource))
				cerr<<"Overload resource "<<resourceId+1<<endl;
			while (curVal > ((int32_t) pos) && ((int32_t) pos) < capacityOfResource)	{
				startTimes[pos++] = curTime;
			}

			++it1; ++it2;
		}

		bool correct = true;
		for (uint32_t j = 0; j < capacityOfResource; ++j)	{
			if (startTimes[j] != resourcesLoad[resourceId][j])	{
				cerr<<"SourcesLoad::addActivity: Please fix computation of vector."<<endl;
				correct = false; break;
			}
		}

		if (!correct)	{
			cerr<<"Resource id: "<<resourceId<<endl;
			cerr<<"activity times: "<<activityStart<<" "<<activityStop<<endl;
			cerr<<"activity requirement: "<<activityRequirement[resourceId]<<endl;
			cerr<<"Original start times vector: "<<endl;
			for (uint32_t i = 0; i < capacityOfResource; ++i)	{
				cerr<<" "<<resourcesLoadCopy[resourceId][i];
			}
			cerr<<endl;
			cerr<<"Probably correct result: "<<endl;
			for (uint32_t i = 0; i < capacityOfResource; ++i)	{
				cerr<<" "<<startTimes[i];
			}
			cerr<<endl;
			cerr<<"Probably incorrect result: "<<endl;
			for (uint32_t i = 0; i < capacityOfResource; ++i)	{
				cerr<<" "<<resourcesLoad[resourceId][i];
			}
			cerr<<endl;
		}
		
		delete[] startTimes;
	}

	for (vector<int32_t*>::const_iterator it = cum.begin(); it != cum.end(); ++it)
		delete[] *it;
	delete[] currentLoad;

	for (uint32_t i = 0; i < numberOfResources; ++i)
		delete[] resourcesLoadCopy[i];
	delete[] resourcesLoadCopy;
	#endif
}

void SourcesLoad::printCurrentState(ostream& OUT)	const	{
	for (uint32_t resourceId = 0; resourceId < numberOfResources; ++resourceId)	{
		OUT<<"Resource "<<resourceId+1<<":";
		for (uint32_t capIdx = 0; capIdx < capacityOfResources[resourceId]; ++capIdx)	
			OUT<<" "<<resourcesLoad[resourceId][capIdx];
		OUT<<endl;
	}
}

SourcesLoad::~SourcesLoad()	{
	for (uint32_t** ptr = resourcesLoad; ptr < resourcesLoad+numberOfResources; ++ptr)
		delete[] *ptr;
	delete[] resourcesLoad;
	delete[] startValues;
	#if DEBUG_SOURCES == 1
	for (map<uint32_t,int32_t*>::const_iterator mit = peaks.begin(); mit != peaks.end(); ++mit)
		delete[] mit->second;
	#endif
}

