#ifndef HLIDAC_PES_SCHEDULE_SOLVER_H
#define HLIDAC_PES_SCHEDULE_SOLVER_H

#include <iostream>
#include "ConfigureRCPSP.h"
#include "InputReader.h"
#include "TabuList.h"

class ScheduleSolver {
	public:
		ScheduleSolver(const InputReader& rcpspData);

		void solveSchedule(const uint32_t& maxIter = ConfigureRCPSP::NUMBER_OF_ITERATIONS);
		void printBestSchedule(bool verbose = true, std::ostream& OUT = std::cout) const;

		~ScheduleSolver();

	protected:

		void createInitialSolution();
		uint32_t evaluateOrder(const uint32_t * const& order, uint32_t *startTimesWriter = NULL, uint32_t *startTimesWriterById = NULL) const;
		uint32_t computePrecedencePenalty(const uint32_t * const& startTimesById) const;
		void printSchedule(const uint32_t * const& scheduleOrder, bool verbose = true, std::ostream& OUT = std::cout) const;
		bool checkSwapPrecedencePenalty(uint32_t i, uint32_t j) const;
		void makeShift(uint32_t * const& order, const int32_t& diff, const uint32_t& baseIdx)	const;
		void makeDiversification();

	private:

		/* COPY OBJECT IS FORBIDDEN */

		ScheduleSolver(const ScheduleSolver&);
		ScheduleSolver& operator=(const ScheduleSolver&);


		/* IMMUTABLE DATA */

		// Number of renewable sources.
		uint32_t numberOfResources;
		// Capacity of resources;
		uint32_t *capacityOfResources;
		// Total number of activities.
		uint32_t numberOfActivities;
		// Duration of activities.
		uint32_t *activitiesDuration;
		// Activities successors;
		uint32_t **activitiesSuccessors;
		// Number of successors that activities.
		uint32_t *numberOfSuccessors;
		// Precomputed predecessors;
		uint32_t **activitiesPredecessors;
		// Number of predecessors;
		uint32_t *numberOfPredecessors;
		// Activities required sources.
		uint32_t **activitiesResources;
		// Matrix of successors. (if matrix(i,j) == 1 then "Exist precedence edge between activities i and j")
		int8_t **relationMatrix;

		/* MUTABLE DATA */	

		// Current activities order.
		uint32_t *activitiesOrder;
		// Best schedule order.
		uint32_t *bestScheduleOrder;
		// Cost of best schedule.
		uint32_t costOfBestSchedule;
		// Tabu list instance.
		TabuList *tabu;
		// Purpose of this variable is to remeber total time.
		double totalRunTime;
};

#endif

