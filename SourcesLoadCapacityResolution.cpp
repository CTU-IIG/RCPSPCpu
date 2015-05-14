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
#include <algorithm>
#include <cstring>
#include "SourcesLoadCapacityResolution.h"

using namespace std;

SourcesLoadCapacityResolution::SourcesLoadCapacityResolution(const uint32_t& numberOfResources, const uint32_t * const& capacitiesOfResources)
	: numberOfResources(numberOfResources), capacitiesOfResources(capacitiesOfResources)	{
	uint32_t maxCapacity = 0;
	resourcesLoad = new uint32_t*[numberOfResources];

	for (uint32_t resourceId = 0; resourceId < numberOfResources; ++resourceId)	{
		resourcesLoad[resourceId] = new uint32_t[capacitiesOfResources[resourceId]];
		memset(resourcesLoad[resourceId], 0, sizeof(uint32_t)*capacitiesOfResources[resourceId]);
		maxCapacity = max(capacitiesOfResources[resourceId], maxCapacity);
	}

	startValues = new uint32_t[maxCapacity];
	memset(startValues, 0, sizeof(uint32_t)*maxCapacity);
}

uint32_t SourcesLoadCapacityResolution::getEarliestStartTime(const uint32_t * const& activityResourceRequirements, const uint32_t&, const uint32_t&) const {
	uint32_t bestStart = 0;
	for (uint32_t resourceId = 0; resourceId < numberOfResources; ++resourceId)	{
		uint32_t activityRequirement = activityResourceRequirements[resourceId];
		if (activityRequirement > 0)
			bestStart = max(resourcesLoad[resourceId][capacitiesOfResources[resourceId]-activityRequirement], bestStart);
	}
	return bestStart;
}

void SourcesLoadCapacityResolution::addActivity(const uint32_t& activityStart, const uint32_t& activityStop, const uint32_t * const& activityRequirements)	{
	#if DEBUG_SOURCES == 1
	map<uint32_t,int32_t*>::iterator mit;
	if ((mit = peaks.find(activityStart)) == peaks.end())	{
		int32_t *peak = new int32_t[numberOfResources];
		for (uint32_t idx = 0; idx < numberOfResources; ++idx)
			peak[idx] = -((int32_t) activityRequirements[idx]);
		peaks[activityStart] = peak;
	} else {
		int32_t *peak = mit->second;
		for (uint32_t idx = 0; idx < numberOfResources; ++idx)
			peak[idx] -= (int32_t) activityRequirements[idx];
	}

	if ((mit = peaks.find(activityStop)) == peaks.end())	{
		int32_t *peak = new int32_t[numberOfResources];
		for (uint32_t idx = 0; idx < numberOfResources; ++idx)
			peak[idx] = (int32_t) activityRequirements[idx];
		peaks[activityStop] = peak;
	} else {
		int32_t *peak = mit->second;
		for (uint32_t idx = 0; idx < numberOfResources; ++idx)
			peak[idx] += (int32_t) activityRequirements[idx];
	}

	int32_t lastLevel, cumulativeValue;
	for (uint32_t r = 0; r < numberOfResources; ++r)	{
		cumulativeValue = 0;
		bool eraseHoles = false;
		lastLevel = capacitiesOfResources[r];
		for (map<uint32_t,int32_t*>::reverse_iterator rit = peaks.rbegin(); rit != peaks.rend(); ++rit)	{
			int32_t peak = rit->second[r];
			if (peak < 0)	{
				eraseHoles = true;
				lastLevel = cumulativeValue;	
			}

			cumulativeValue += peak;

			if (eraseHoles == true)	{
				if (lastLevel > cumulativeValue)	{
					rit->second[r] = 0;
				} else {
					eraseHoles = false;
					rit->second[r] = cumulativeValue-lastLevel;
				}
			}
		}
	}

	uint32_t **resourcesLoadCopy = new uint32_t*[numberOfResources];
	for (uint32_t i = 0; i < numberOfResources; ++i)	{
		resourcesLoadCopy[i] = new uint32_t[capacitiesOfResources[i]];
		for (uint32_t j = 0; j < capacitiesOfResources[i]; ++j)
			resourcesLoadCopy[i][j] = resourcesLoad[i][j];
	}
	#endif

	int32_t requiredSquares, timeDiff;
	uint32_t k, c, capacityOfResource, resourceRequirement, newStartTime;
	for (uint32_t resourceId = 0; resourceId < numberOfResources; ++resourceId)	{
		capacityOfResource = capacitiesOfResources[resourceId];
		resourceRequirement = activityRequirements[resourceId];
		requiredSquares = resourceRequirement*(activityStop-activityStart);
		if (requiredSquares > 0)	{
			c = 0; newStartTime = activityStop;
			k = upper_bound(resourcesLoad[resourceId], resourcesLoad[resourceId]+capacityOfResource, activityStop, cmpMethod)-resourcesLoad[resourceId]; 
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
		uint32_t capacityOfResource = capacitiesOfResources[resourceId];
		uint32_t *startTimes = new uint32_t[capacityOfResource];
		memset(startTimes, 0, sizeof(uint32_t)*capacityOfResource);

		vector<uint32_t>::const_iterator it1 = tim.begin(), eit1 = tim.end();
		vector<int32_t*>::const_iterator it2 = cum.begin(), eit2 = cum.end();

		while (it1 != eit1 && it2 != eit2)	{
			uint32_t curTime = *it1;
			int32_t curVal = (*it2)[resourceId];
			if (curVal > ((int32_t) capacityOfResource))
				cerr<<"Overload resource "<<resourceId+1<<endl;

			for (uint32_t i = 0; i < (uint32_t) curVal && i < capacityOfResource; ++i)	{
				if (startTimes[i] < curTime)
					startTimes[i] = curTime;
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
			cerr<<"activity requirement: "<<activityRequirements[resourceId]<<endl;
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
			cerr<<"Time of peaks and values:"<<endl;
			for (map<uint32_t,int32_t*>::const_iterator mit = peaks.begin(); mit != peaks.end(); ++mit)	{
				cerr<<mit->first<<" -> "<<mit->second[resourceId]<<"; ";
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

void SourcesLoadCapacityResolution::printCurrentState(ostream& output)	const	{
	for (uint32_t resourceId = 0; resourceId < numberOfResources; ++resourceId)	{
		output<<"Resource "<<resourceId+1<<":";
		for (uint32_t capIdx = 0; capIdx < capacitiesOfResources[resourceId]; ++capIdx)	
			output<<" "<<resourcesLoad[resourceId][capIdx];
		output<<endl;
	}
}

bool SourcesLoadCapacityResolution::cmpMethod(const uint32_t& i, const uint32_t& j)	{
	return i > j ? true : false;
}

SourcesLoadCapacityResolution::~SourcesLoadCapacityResolution()	{
	for (uint32_t** ptr = resourcesLoad; ptr < resourcesLoad+numberOfResources; ++ptr)
		delete[] *ptr;
	delete[] resourcesLoad;
	delete[] startValues;
	#if DEBUG_SOURCES == 1
	for (map<uint32_t,int32_t*>::const_iterator mit = peaks.begin(); mit != peaks.end(); ++mit)
		delete[] mit->second;
	#endif
}

