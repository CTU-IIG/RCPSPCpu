#ifndef HLIDAC_PES_SCHEDULE_SOLVER_H
#define HLIDAC_PES_SCHEDULE_SOLVER_H

/*!
 * \file ScheduleSolver.h
 * \author Libor Bukata
 * \brief RCPSP solver class.
 */

#include <iostream>
#include <string>
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
		 * \param output Output stream.
		 * \brief Print best found schedule, schedule length, computational time and number of evaluated schedules.
		 */
		void printBestSchedule(bool verbose = true, std::ostream& output = std::cout);
		/*!
		 * \param fileName The name of the file where results will be written.
		 * \brief It writes required data structures and the best schedule to the given file.
		 */
		void writeBestScheduleToFile(const std::string& fileName);

		//! Free all allocated resources.
		~ScheduleSolver();

	protected:

		//! Compute predecessors, matrix of successors and create init activities order.
		void createInitialSolution();
		
		/*!
		 * \param startActivityId The id of the start activity of the project.
		 * \param energyReasoning The energy requirements are taken into account if energyReasoning variable is set to true.
		 * \return The earliest start time for each activity.
		 * \brief Lower bounds of the earliest start time values are computed for each activity.
		 */
		uint32_t* computeLowerBounds(const uint32_t& startActivityId, const bool& energyReasoning = false) const;
		
		/*!
		 * \param makespan The considered project duration is taken as a reference to upper bound computations.
		 * \param startTimesById The start time value for each activity.
		 * \return The sum of overhangs, i.e. the penalty.
		 * \brief The penalty is computed as a sum of overhangs. Overhang of the activity is the difference between the latest
		 * activity finish time and the real finish time if real finish time > the latest finish time.
		 */
		uint32_t computeUpperBoundsOverhangPenalty(const uint32_t& makespan, const uint32_t * const& startTimesById) const;

		/*!
		 * \param scheduleOrder Order of activities that should be evaluated.
		 * \param verbose If true then verbose mode is turn on.
		 * \param output Output stream.
		 * \brief Print schedule, schedule length, precedence penalty and number of evaluated schedules.
		 */
		void printSchedule(const uint32_t * const& scheduleOrder, bool verbose = true, std::ostream& output = std::cout);

		/*!
		 * \param order Activities order.
		 * \param relatedActivities It's array of successors or predecessors. It depends on a way of evaluation (forward, backward).
		 * \param numberOfRelatedActivities Number of successors/predecessors for each activity.
		 * \param timeValuesById The earliest start time values for forward evaluation
		 * and transformed time values for backward evaluation.
		 * \param forwardEvaluation It determines if forward or backward schedule is evaluated.
		 * \return Length of the schedule.
		 * \brief Input order is evaluated and the earliest start/transformed time values are computed.
		 * \warning Order of activities is sequence of putting to the schedule, time values don't have to be ordered.
		 */
		uint32_t evaluateOrder(const uint32_t * const& order, const uint32_t * const * const& relatedActivities,
			       const uint32_t * const& numberOfRelatedActivities, uint32_t *& timeValuesById, bool forwardEvaluation) const;
		/*!
		 * \param order The sequence of putting to the schedule. It's activity order.
		 * \param startTimesById The earliest start time values for each scheduled activity.
		 * \return Project makespan, i.e. the length of the schedule.
		 * \brief It evaluates order of activities and determines the earliest start time values.
		 */
		uint32_t forwardScheduleEvaluation(const uint32_t * const& order, uint32_t *& startTimesById) const;
		/*!
		 * \param order The sequence of putting to the schedule. It's activity order.
		 * \param startTimesById The latest start time values for each scheduled activity.
		 * \return Project makespan, i.e. the length of the schedule.
		 * \brief It evaluates order (in reverse order) of activities and determines the latest start time values.
		 */
		uint32_t backwardScheduleEvaluation(const uint32_t * const& order, uint32_t *& startTimesById) const;
		/*!
		 * \param order Order of activities. It determines the order of putting to the schedule.
		 * \param bestScheduleStartTimesById The earliest start time values for the best found schedule.
		 * \return Project makespan, i.e. the length of the schedule.
		 * \brief Iterative method tries to shake down activities in the schedule to ensure equally loaded resources.
		 * Therefore, the shorter schedule could be found.
		 */
		uint32_t shakingDownEvaluation(const uint32_t * const& order, uint32_t *bestScheduleStartTimesById);
		/*!
		 * \param startTimesById Start time values of activities ordered by ID's.
		 * \return Precedence penalty of the schedule.
		 * \brief Method compute precedence penalty (= broken relation between two activities) of the schedule.
		 * \note Because precedence free swaps and shifts are currently used, this function is only for debugging purposes.
		 */
		uint32_t computePrecedencePenalty(const uint32_t * const& startTimesById) const;
		/*!
		 * \param i Index at activitiesOrder.
		 * \param j Index at activitiesOrder.
		 * \return True if and only if precedence penalty is zero else false.
		 * \brief Method check if candidate for swap is precedence penalty free.
		 */
		bool checkSwapPrecedencePenalty(uint32_t i, uint32_t j) const;

		/*!
		 * \param order Original activities order.
		 * \param startTimesById The earliest start time values in the order W.
		 * \brief It transforms the earliest start time values to the order W. The order W is written to the variable order.
		 */
		void convertStartTimesById2ActivitiesOrder(uint32_t *order, const uint32_t * const& startTimesById) const;
		/*!
		 * \param order Order of activities.
		 * \param timeValuesById Assigned time values to activities, it is used for sorting input order.
		 * \param size It's size of the arrays, i.e. number of project activities.
		 * \brief Input order of activities is sorted in accordance with time values. It's stable sort.
		 */
		static void insertSort(uint32_t* order, const uint32_t * const& timeValuesById, const int32_t& size);

		/*!
		 * \param order Activities order.
		 * \param diff Direction and norm of shift move.
		 * \param baseIdx Base index from which the shift will be performed.
		 * \brief Change activities order. Activity at index baseIdx is shifted to the left or to the right (diff parameter). 
		 */
		void makeShift(uint32_t * const& order, const int32_t& diff, const uint32_t& baseIdx)	const;

		//! Random swaps are performed when diversification is called.
		void makeDiversification();

		/*!
		 * \param activityId The activity from which all related activities are found.
		 * \param numberOfRelated The number of related activities for each activity.
		 * \param related The related (= successors || predecessors) activities for each activity in the project.
		 * \return It returns all activityId's successors or predecessors.
		 */
		std::vector<uint32_t> getAllRelatedActivities(uint32_t activityId, uint32_t *numberOfRelated, uint32_t **related) const;
		/*!
		 * \param activityId Identification of the activity.
		 * \return It returns all activityId's successors.
		 */
		std::vector<uint32_t> getAllActivitySuccessors(const uint32_t& activityId) const;
		/*!
		 * \param activityId Identification of the activity.
		 * \return It returns all activityId's predecessors.
		 */
		std::vector<uint32_t> getAllActivityPredecessors(const uint32_t& activityId) const;


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
		//! The longest paths from the end activity in the transformed graph.
		uint32_t *rightLeftLongestPaths;
		//! Upper bound of Cmax (sum of all activity durations).
		uint32_t upperBoundMakespan;
		//! Current selected version of resources evaluation algorithm.
		EvaluationAlgorithm algo;
		//! Required evaluation time per iteration for evaluation algorithm TIME_RESOLUTION.
		double reqTimePerIterForTimeResAlg;
		//! Required evaluation time per iteration for evaluation algorithm CAPACITY_RESOLUTION.
		double reqTimePerIterForCapacityResAlg;
		//! Purpose of this variable is to remember total time.
		double totalRunTime;
		//! Total number of evaluaded schedules on the CPU.
		uint64_t numberOfEvaluatedSchedules;
};

#endif

