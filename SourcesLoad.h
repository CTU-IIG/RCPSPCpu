#ifndef HLIDAC_PES_SOURCES_LOAD_H
#define HLIDAC_PES_SOURCES_LOAD_H

/*!
 * \file SourcesLoad.h
 * \author Libor Bukata
 * \brief Implementation of SourcesLoad class.
 */

/*!
 * Constant that is used to turn on/off debug mode for SourcesLoad class.
 * Debug mode detect error at runtime - error messages are displayed to console.
 * Two different algorithms compute new sources load vectors and compare results.
 * Debug mode check only addActivity method. Although the most difficult method in this class.
 * \warning Debug mode decrease performance in a nasty way. (more than 10 times)
 */
#define DEBUG_SOURCES 0

#include <iostream>
#include <stdint.h>

#if DEBUG_SOURCES == 1
#include <map>
#include <vector>
#endif

/*!
 * \class SourcesLoad
 * \brief All sources states are stored and updated in this class.
 */
class SourcesLoad {
	public:
		/*!
		 * \param numRes Number of resources.
		 * \param capRes Capacities of resources.
		 * \brief Allocate sources arrays and helper arrays.
		 */
		SourcesLoad(const uint32_t& numRes, const uint32_t * const& capRes);

		/*!
		 * \param activityResourceRequirement Activity requirements.
		 * \return The earliest start time of activity (precedence relations aren't counted).
		 * \brief Compute earliest start time of activity with given sources requirements.
		 */
		uint32_t getEarliestStartTime(const uint32_t * const& activityResourceRequirement) const;
		/*!
		 * \param activityStart Start time of activity.
		 * \param activityStop Stop time of the activity. (= activityStart + activityDuration)
		 * \param activityRequirement Activity resources requirements.
		 * \brief Update sources vectors (arrays). This operation is irreversible.
		 * \note Method can be debugged.
		 */
		void addActivity(const uint32_t& activityStart, const uint32_t& activityStop, const uint32_t * const& activityRequirement);
		/*!
		 * \param OUT Output stream.
		 * \brief Print current state of resources.
		 */
		void printCurrentState(std::ostream& OUT = std::cout) const;

		//! Free all allocated memory.
		~SourcesLoad();

	private:

		//! Copy constructor is forbidden.
		SourcesLoad(const SourcesLoad&);
		//! Asignment operator is forbidden.
		SourcesLoad& operator=(const SourcesLoad&);


		//! Total number of resources.
		const uint32_t numberOfResources;
		//! Capacities of resources.
		const uint32_t * const capacityOfResources;
		//! Current state of resources.
		uint32_t **resourcesLoad;
		//! Helper array that is used at addActivity method.
		uint32_t *startValues;
		//! Helper array that is used at addActivity method.
		uint32_t *reqItems;

		#if DEBUG_SOURCES == 1
		//! Save current state of the resources as a peaks. Only in debug mode.
		std::map<uint32_t,int32_t*> peaks;
		#endif
};

#endif

