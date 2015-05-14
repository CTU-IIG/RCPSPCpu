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
#ifndef HLIDAC_PES_SOURCES_LOAD_CAPACITY_RESOLUTION_H
#define HLIDAC_PES_SOURCES_LOAD_CAPACITY_RESOLUTION_H

/*!
 * \file SourcesLoadCapacityResolution.h
 * \author Libor Bukata
 * \brief Implementation of SourcesLoadCapacityResolution class.
 */

/*!
 * Constant that is used to turn on/off debug mode for SourcesLoadCapacityResolution class.
 * Debug mode detect error at runtime - error messages are displayed to console.
 * Two different algorithms compute new sources load vectors and compare results.
 * Debug mode check only addActivity method. Although the most difficult method in this class.
 * \warning Debug mode decrease performance in a nasty way. (more than 10 times)
 */
#define DEBUG_SOURCES 0

#include <iostream>
#include <stdint.h>
#include "SourcesLoad.h"

#if DEBUG_SOURCES == 1
#include <map>
#include <vector>
#endif

/*!
 * \class SourcesLoadCapacityResolution
 * \brief All sources states are stored and updated in this class.
 */
class SourcesLoadCapacityResolution : public SourcesLoad {
	public:
		/*!
		 * \param numberOfResources Number of resources.
		 * \param capacitiesOfResources Capacities of resources.
		 * \brief Allocate sources arrays and helper arrays.
		 */
		SourcesLoadCapacityResolution(const uint32_t& numberOfResources, const uint32_t * const& capacitiesOfResources);

		/*!
		 * \param activityResourceRequirements Activity requirements.
		 * \return The earliest start time of activity (precedence relations aren't counted).
		 * \brief Compute earliest start time of the activity with given sources requirements.
		 * \note The last two parameters are unused in the method.
		 */
		virtual uint32_t getEarliestStartTime(const uint32_t * const& activityResourceRequirements, const uint32_t&, const uint32_t&) const;
		/*!
		 * \param activityStart Start time of activity.
		 * \param activityStop Stop time of the activity. (= activityStart + activityDuration)
		 * \param activityRequirements Activity resources requirements.
		 * \brief Update sources vectors (arrays). This operation is irreversible.
		 * \note Method can be debugged.
		 */
		virtual void addActivity(const uint32_t& activityStart, const uint32_t& activityStop, const uint32_t * const& activityRequirements);
		/*!
		 * \param output Output stream.
		 * \brief Print current state of resources.
		 */
		void printCurrentState(std::ostream& output = std::cout) const;
		/*!
		 * \param i The first compared start time.
		 * \param j The second compared start time.
		 * \return i > j ? true : false;
		 * \brief Comparator function is used in upper_bound function that is used in the addActivity method.
		 */
		static bool cmpMethod(const uint32_t& i, const uint32_t& j);

		//! Free all allocated memory.
		virtual ~SourcesLoadCapacityResolution();

	private:

		//! Copy constructor is forbidden.
		SourcesLoadCapacityResolution(const SourcesLoadCapacityResolution&);
		//! Asignment operator is forbidden.
		SourcesLoadCapacityResolution& operator=(const SourcesLoadCapacityResolution&);


		//! Total number of resources.
		const uint32_t numberOfResources;
		//! Capacities of resources.
		const uint32_t * const capacitiesOfResources;
		//! Current state of resources.
		uint32_t **resourcesLoad;
		//! Helper array that is used at addActivity method.
		uint32_t *startValues;

		#if DEBUG_SOURCES == 1
		//! Save current state of the resources as a peaks. Only in debug mode.
		std::map<uint32_t,int32_t*> peaks;
		#endif
};

#endif

