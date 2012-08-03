#ifndef HLIDAC_PES_SCHEDULE_SOLVER_H
#define HLIDAC_PES_SCHEDULE_SOLVER_H

/*!
 * \file ScheduleSolver.h
 * \author Libor Bukata
 * \brief RCPSP solver class.
 */

#include <iostream>
#include "ConfigureRCPSP.h"
#include "InputReader.h"
#include "TabuList.h"

/*!
 * Tabu search meta heuristic is used to solve RCPSP. Multiprocessors are effectively exploited by OpenMP library.
 * \class ScheduleSolver
 * \brief Instance of this class is able to solve resource constrained project scheduling problem.
 */
class ScheduleSolver {
	public:
		/*!
		 * \param rcpspData Data of project instance.
		 * \exception invalid_argument Invalid type of tabu list.
		 * \brief Copy pointers of project data, initialize required structures, create initial activities order, ...
		 */
		ScheduleSolver(const InputReader& rcpspData);

		/*!
		 * \param maxIter Number of iterations that should be performed.
		 * \param graphFilename Filename of generated graph (csv file).
		 * \exception runtime_error Unsupported type of move.
		 * \brief Use tabu search to find good quality solution.
		 */
		void solveSchedule(const uint32_t& maxIter = ConfigureRCPSP::NUMBER_OF_ITERATIONS, const std::string& graphFilename = "");
		/*!
		 * \param verbose If true then verbose mode is turn on.
		 * \param OUT Output stream.
		 * \brief Print best found schedule, schedule length and computational time.
		 */
		void printBestSchedule(bool verbose = true, std::ostream& OUT = std::cout) const;

		//! Free all allocated resources.
		~ScheduleSolver();

	protected:

		//! Compute predecessors, matrix of successors and create init activities order.
		void createInitialSolution();
		/*!
		 * \param order Activities order.
		 * \param startTimesWriter Start times of the activities can be written to this array. Order is the same as the order parameter.
		 * \param startTimesWriterById Start times of the activities can be written to this array. Order is defined by activities ID's.
		 * \return Length of the schedule.
		 * \brief Input order is evaluated and start times are determined. Total schedule length is returned.
		 * \warning Order of activities is sequence of putting to the schedule, start time values don't have to be ordered ascendly.
		 */
		uint32_t evaluateOrder(const uint32_t * const& order, uint32_t *startTimesWriter = NULL, uint32_t *startTimesWriterById = NULL) const;
		/*!
		 * \param startTimesById Start times of activities ordered by ID's.
		 * \return Precedence penalty of the schedule.
		 * \brief Method compute precedence penalty (= broken relation between two activities) of the schedule.
		 * \note Because precedence free swaps and shifts are currently used, this function is only for debugging purposes.
		 */
		uint32_t computePrecedencePenalty(const uint32_t * const& startTimesById) const;
		/*!
		 * \param scheduleOrder Order of activities that should be evaluated.
		 * \param verbose If true then verbose mode is turn on.
		 * \param OUT Output stream.
		 * \brief Print schedule and schedule length. 
		 */
		void printSchedule(const uint32_t * const& scheduleOrder, bool verbose = true, std::ostream& OUT = std::cout) const;
		/*!
		 * \param i Index at activitiesOrder.
		 * \param j Index at activitiesOrder.
		 * \return True if and only if precedence penalty is zero else false.
		 * \brief Method check if candidate for swap is precedence penalty free.
		 */
		bool checkSwapPrecedencePenalty(uint32_t i, uint32_t j) const;
		/*!
		 * \param order Activities order.
		 * \param diff Direction and norm of shift move.
		 * \param baseIdx Base index from which the shift will be performed.
		 * \brief Change activities order. Activity at index baseIdx is shifted to the left or to the right (diff parameter). 
		 */
		void makeShift(uint32_t * const& order, const int32_t& diff, const uint32_t& baseIdx)	const;
		//! Random swaps are performed when diversification is called.
		void makeDiversification();

	private:

		//! Copy constructor is forbidden.
		ScheduleSolver(const ScheduleSolver&);
		//! Assignment operator is forbidden.
		ScheduleSolver& operator=(const ScheduleSolver&);


		/* IMMUTABLE DATA */

		//! Number of renewable sources.
		uint32_t numberOfResources;
		//! Capacity of resources;
		uint32_t *capacityOfResources;
		//! Total number of activities.
		uint32_t numberOfActivities;
		//! Duration of activities.
		uint32_t *activitiesDuration;
		//! Activities successors;
		uint32_t **activitiesSuccessors;
		//! Number of successors that activities.
		uint32_t *numberOfSuccessors;
		//! Precomputed predecessors.
		uint32_t **activitiesPredecessors;
		//! Number of predecessors.
		uint32_t *numberOfPredecessors;
		//! Sources that are required by activities.
		uint32_t **activitiesResources;
		//! Matrix of successors. (if matrix(i,j) == 1 then "Exist precedence edge between activities i and j")
		int8_t **relationMatrix;
		//! Critical Path Makespan. (Critical Path Method)
		int32_t criticalPathMakespan;

		/* MUTABLE DATA */	

		//! Current activities order.
		uint32_t *activitiesOrder;
		//! Best schedule order.
		uint32_t *bestScheduleOrder;
		//! Cost of the best schedule.
		uint32_t costOfBestSchedule;
		//! Tabu list instance.
		TabuList *tabu;
		//! Purpose of this variable is to remember total time.
		double totalRunTime;
};

#endif

