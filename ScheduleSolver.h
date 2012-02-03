#ifndef HLIDAC_PES_SCHEDULE_SOLVER_H
#define HLIDAC_PES_SCHEDULE_SOLVER_H

#include <iostream>
#include "ConfigureRCPSP.h"
#ifdef SIMPLE_TABU
#include "SimpleTabuList.h"
#endif
#ifdef ADVANCE_TABU
#include "TabuList.h"
#endif

class ScheduleSolver {
	public:
		ScheduleSolver(uint32_t resNum, uint32_t *capRes, uint32_t actNum, uint32_t *actDur, uint32_t **actSuc, uint32_t *actNumSuc, uint32_t **actRes, uint32_t maxIter);

		void solveSchedule(const uint32_t& maxIter = 100);
		void printBestSchedule(bool verbose = true, std::ostream& OUT = std::cout) const;

		~ScheduleSolver();

	protected:

		void createInitialSolution();
		uint32_t evaluateOrder(const uint32_t *order, uint32_t *startTimesWriter = NULL, uint32_t *startTimesWriterById = NULL) const;
		uint32_t computePrecedencePenalty(const uint32_t *startTimesById) const;
		void printSchedule(uint32_t *scheduleOrder, bool verbose = true, std::ostream& OUT = std::cout) const;
		void makeShift(uint32_t *order, int32_t diff, uint32_t baseIdx)	const;

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
		uint32_t **activitesResources;
		

		/* MUTABLE DATA */	

		// Current activities order.
		uint32_t *activitiesOrder;
		// Order that is used when diversification is performed.
		uint32_t *diversificationOrder;
		// Max iter that are allowed than diversification is called.
		uint32_t maxIterToDiversification;
		// Best schedule order.
		uint32_t *bestScheduleOrder;
		// Cost of best schedule.
		uint32_t costOfBestSchedule;
		// Tabu list instance.
		#ifdef SIMPLE_TABU
		SimpleTabuList tabu;
		#endif
		#ifdef ADVANCE_TABU
		TabuList tabu;
		#endif
		double totalRunTime;
};

#endif

