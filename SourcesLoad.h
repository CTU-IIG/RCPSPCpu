#ifndef HLIDAC_PES_SOURCES_LOAD_H
#define HLIDAC_PES_SOURCES_LOAD_H

/*!
 * \file SourcesLoad.h
 * \author Libor Bukata
 * \brief Abstract class for the evaluation algorithms.
 */

/*!
 * Activities are put to the schedule one after another (sequence is determined by activities order)
 * and state of the resources has to be updated correctly. This state is stored in derived classes from the SourcesLoad.
 * \class SourcesLoad
 * \brief The SourcesLoad abstract class that defines required operations for resources evaluation.
 */
class SourcesLoad {
	public:
		//! Implicit constructor.
		SourcesLoad() { };

		/*!
		 * \param activityResourceRequirements Activity requirement for each available resource.
		 * \param earliestPrecedenceStartTime The earliest start time of the activity without precedence violation.
		 * \param activityDuration Duration of the activity.
		 * \return The earliest activity start time without resource overload.
		 * \brief It finds out the earliest possible activity start time without resource overload.
		 */
		virtual uint32_t getEarliestStartTime(const uint32_t * const& activityResourceRequirements,
			       const uint32_t& earliestPrecedenceStartTime, const uint32_t& activityDuration) const = 0;
		/*!
		 * \param activityStart Scheduled start time of the activity.
		 * \param activityStop Finish time of the scheduled activity.
		 * \param activityRequirements Activity requirement for each available resource.
		 * \brief It updates state of resources with respect to the added activity.
		 */
		virtual void addActivity(const uint32_t& activityStart, const uint32_t& activityStop, const uint32_t * const& activityRequirements) = 0;

		//! Virtual implicit destructor.
		virtual ~SourcesLoad() { };
};

#endif

