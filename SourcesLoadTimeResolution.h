#ifndef HLIDAC_PES_SOURCES_LOAD_TIME_RESOLUTION_H
#define HLIDAC_PES_SOURCES_LOAD_TIME_RESOLUTION_H

/*!
 * \file SourcesLoadTimeResolution.h
 * \author Libor Bukata
 * \brief Implementation of SourcesLoadTimeResolution class.
 */

#include <iostream>
#include <stdint.h>
#include "SourcesLoad.h"

/*!
 * \class SourcesLoadTimeResolution
 * \brief Implementation of resources evaluation. For each time unit available free capacity is remembered.
 */
class SourcesLoadTimeResolution : public SourcesLoad {
	public:
		/*!
		 * \param numberOfResources Number of renewable resources with constant capacity.
		 * \param capacitiesOfResources Maximal capacity of each resource.
		 * \param makespanUpperBound Estimate of maximal project duration.
		 * \brief It allocates required data-structures and fill them with initial values.
		 */
		SourcesLoadTimeResolution(const uint32_t& numberOfResources, const uint32_t * const& capacitiesOfResources, const uint32_t& makespanUpperBound);

		/*!
		 * \param activityResourceRequirements Activity requirement for each resource.
		 * \param earliestPrecedenceStartTime The earliest activity start time without precedence violation.
		 * \param activityDuration Duration of the activity.
		 * \return The earliest activity start time without precedence and resources violation.
		 */
		virtual uint32_t getEarliestStartTime(const uint32_t * const& activityResourceRequirements,
			       	const uint32_t& earliestPrecedenceStartTime, const uint32_t& activityDuration) const;
		/*!
		 * \param activityStart Start time of the scheduled activity.
		 * \param activityStop Finish time of the scheduled activity.
		 * \param activityRequirements Activity requirement for each resource.
		 * \brief Update state of resources with respect to the added activity.
		 */
		virtual void addActivity(const uint32_t& activityStart, const uint32_t& activityStop, const uint32_t * const& activityRequirements);

		//! Free allocated memory.
		virtual ~SourcesLoadTimeResolution();

	private:

		//! Copy constructor is forbidden.
		SourcesLoadTimeResolution(const SourcesLoadTimeResolution&);
		//! Assignment operator is forbidden.
		SourcesLoadTimeResolution& operator=(const SourcesLoadTimeResolution&);

		//! Number of renewable resources with constant capacity.
		const uint32_t numberOfResources;
		//! Capacities of the resources.
		const uint32_t * const capacitiesOfResources;
		//! Upper bound of the project duration.
		const uint32_t makespanUpperBound;
		//! Available capacity for each resource (independent variable is time).
		uint32_t **remainingResourcesCapacity;
};

#endif

