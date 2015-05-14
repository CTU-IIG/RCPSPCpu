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
#include "SourcesLoadTimeResolution.h"

using namespace std;

SourcesLoadTimeResolution::SourcesLoadTimeResolution(const uint32_t& numberOfResources, const uint32_t * const& capacitiesOfResources, const uint32_t& makespanUpperBound)
       	: numberOfResources(numberOfResources), capacitiesOfResources(capacitiesOfResources), makespanUpperBound(makespanUpperBound)	{
	remainingResourcesCapacity = new uint32_t*[numberOfResources];
	for (uint32_t resourceId = 0; resourceId < numberOfResources; ++resourceId)	{
		remainingResourcesCapacity[resourceId] = new uint32_t[makespanUpperBound];
		for (uint32_t t = 0; t < makespanUpperBound; ++t)
			remainingResourcesCapacity[resourceId][t] = capacitiesOfResources[resourceId];
	}
}

uint32_t SourcesLoadTimeResolution::getEarliestStartTime(const uint32_t * const& activityResourceRequirements,
		const uint32_t& earliestPrecedenceStartTime, const uint32_t& activityDuration)	const 	{
	uint32_t loadTime = 0, t = makespanUpperBound;
	for (t = earliestPrecedenceStartTime; t < makespanUpperBound && loadTime < activityDuration; ++t)	{
		bool capacityAvailable = true;
		for (uint32_t resourceId = 0; resourceId < numberOfResources && capacityAvailable; ++resourceId)	{
			if (remainingResourcesCapacity[resourceId][t] < activityResourceRequirements[resourceId])	{
				loadTime = 0;
				capacityAvailable = false;
			}
		}
		if (capacityAvailable)
			++loadTime;
	}
	return t-loadTime;
}

void SourcesLoadTimeResolution::addActivity(const uint32_t& activityStart, const uint32_t& activityStop, const uint32_t * const& activityRequirements)	{
	for (uint32_t resourceId = 0; resourceId < numberOfResources; ++resourceId)	{
		for (uint32_t t = activityStart; t < activityStop; ++t)	{
			remainingResourcesCapacity[resourceId][t] -= activityRequirements[resourceId];
		}
	}
}

SourcesLoadTimeResolution::~SourcesLoadTimeResolution()	{
	for (uint32_t** ptr = remainingResourcesCapacity; ptr < remainingResourcesCapacity+numberOfResources; ++ptr)
		delete[] *ptr;
	delete[] remainingResourcesCapacity;
}

