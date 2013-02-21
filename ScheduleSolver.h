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
	//! A forward declaration of the InstanceData inner class.
	struct InstanceData;
	//! A forward declaration of the InstanceSolution inner class.
	struct InstanceSolution;

	public:
		/*!
		 * \param rcpspData Data of the project instance.
		 * \exception invalid_argument Invalid type of tabu list.
		 * \brief Copy pointers of project data, initialize required structures, create initial activities order, ...
		 */
		ScheduleSolver(const InputReader& rcpspData);

		/*!
		 * \param maxIter Number of iterations that should be performed.
		 * \param graphFilename Filename of generated graph (csv file).
		 * \exception runtime_error Unsupported type of move.
		 * \brief Use tabu search to find a good quality solution.
		 */
		void solveSchedule(const uint32_t& maxIter = ConfigureRCPSP::NUMBER_OF_ITERATIONS, const std::string& graphFilename = "");
		/*!
		 * \param verbose If true then verbose mode is turn on.
		 * \param output Output stream.
		 * \brief Print best found schedule, schedule length, computational time and number of evaluated schedules.
		 */
		void printBestSchedule(bool verbose = true, std::ostream& output = std::cout);
		/*!
		 * \param filename The name of the file where results will be written.
		 * \exception invalid_argument The output file cannot be created, check the permissions.
		 * \brief It writes required data structures and the best schedule to the given file.
		 */
		void writeBestScheduleToFile(const std::string& filename);

		//! Free all allocated resources.
		~ScheduleSolver();

	protected:

		/*!
		 * \param project The data-structure of the read instance.
		 * \param solution The data-structure which stores an initial solution of the project instance.
		 * \brief It initialises auxiliary data-structures of the read instance and creates the initial solution.
		 */
		static void initialiseInstanceDataAndInitialSolution(InstanceData& project, InstanceSolution& solution);


		/*!
		 * \param project The data of the read instance.
		 * \param solution An initial order will be written to this data-structure.
		 * \brief An initial order of activities is created using precedence graph stored in the project data-structure.
		 */
		static void createInitialSolution(const InstanceData& project, InstanceSolution& solution);

		/*!
		 * \param out The output stream where the instance data and the solution will be written.
		 * \param project The project instance data that will be written to the output file.
		 * \param solution The solution of the given project.
		 * \return A reference to the output stream.
		 * \brief The method writes the instance data and solution of the instance.
		 */
		static std::ofstream& writeBestScheduleToFile(std::ofstream& out, const InstanceData& project, const InstanceSolution& solution);
		
		/*!
		 * \param startActivityId The id of the start activity of the project.
		 * \param project The project instance in which the longest paths are computed.
		 * \param energyReasoning The energy requirements are taken into account if energyReasoning variable is set to true.
		 * \return The earliest start time for each activity.
		 * \brief Lower bounds of the earliest start time values are computed for each activity.
		 * \warning The user is responsible for freeing the allocated memory in the returned array.
		 */
		static uint32_t* computeLowerBounds(const uint32_t& startActivityId, const InstanceData& project, const bool& energyReasoning = false);

		/*!
		 * \param project The instance data of the instance.
		 * \return The method returns an estimate of the project makespan.
		 * \brief A lower bound is computed by using the "Extended Node Packing Bound" problem.
		 */
		static uint32_t lowerBoundOfMakespan(const InstanceData& project);

		static void createStaticTreeOfSolutions(const InstanceData& project);
		
		/*!
		 * \param project The data of the instance.
		 * \param solution Current solution of the instance. The best makespan value is required.
		 * \param startTimesById The start time value for each activity.
		 * \return The sum of overhangs, i.e. the penalty.
		 * \brief The penalty is computed as a sum of overhangs. Overhang of the activity is the difference between the latest
		 * activity finish time and the real finish time if real finish time > the latest finish time.
		 */
		static uint32_t computeUpperBoundsOverhangPenalty(const InstanceData& project, const InstanceSolution& solution, const uint32_t * const& startTimesById);

		/*!
		 * \param project The data of the printed instance.
		 * \param solution The solution of the instance.
		 * \param runTime The computation time at seconds.
		 * \param evaluatedSchedules The number of evaluated schedules during execution.
		 * \param verbose If true then verbose mode is turn on.
		 * \param output Output stream.
		 * \brief Print schedule, schedule length, precedence penalty and number of evaluated schedules.
		 */
		static void printSchedule(const InstanceData& project, const InstanceSolution& solution, double runTime, uint64_t evaluatedSchedules, bool verbose = true, std::ostream& output = std::cout);

		/*!
		 * \param project The data of the instance. (activity duration, precedence edges, ...)
		 * \param solution The current solution of the project. The order is evaluated.
		 * \param timeValuesById The earliest start time values for forward evaluation and transformed time values for backward evaluation.
		 * \param forwardEvaluation It determines if forward or backward schedule is evaluated.
		 * \param algorithm The selected evaluation algorithm.
		 * \return Length of the schedule.
		 * \brief Input order is evaluated and the earliest start/transformed time values are computed.
		 * \warning Order of activities is sequence of putting to the schedule, time values don't have to be ordered.
		 */
		static uint32_t evaluateOrder(const InstanceData& project, const InstanceSolution& solution, uint32_t *& timeValuesById, bool forwardEvaluation, EvaluationAlgorithm algorithm);

		/*!
		 * \param project The data of the instance.
		 * \param solution Current solution of the instance.
		 * \param startTimesById The earliest start time values for each scheduled activity.
		 * \param algorithm The selected evaluation algorithm.
		 * \return Project makespan, i.e. the length of the schedule.
		 * \brief It evaluates order of activities and determines the earliest start time values.
		 */
		static uint32_t forwardScheduleEvaluation(const InstanceData& project, const InstanceSolution& solution, uint32_t *& startTimesById, EvaluationAlgorithm algorithm);
		/*!
		 * \param project The data-structure of the instance.
		 * \param solution Current solution of the instance.
		 * \param startTimesById The latest start time values for each scheduled activity.
		 * \param algorithm The selected evaluation algorithm.
		 * \return Project makespan, i.e. the length of the schedule.
		 * \brief It evaluates order (in reverse order) of activities and determines the latest start time values.
		 */
		static uint32_t backwardScheduleEvaluation(const InstanceData& project, const InstanceSolution& solution, uint32_t *& startTimesById, EvaluationAlgorithm algorithm);
		/*!
		 * \param project The data-structure of the instance.
		 * \param solution Current solution of the instance.
		 * \param bestScheduleStartTimesById The earliest start time values for the best found schedule.
		 * \return Project makespan, i.e. the length of the schedule.
		 * \brief Iterative method tries to shake down activities in the schedule to ensure equally loaded resources.
		 * Therefore, the shorter schedule could be found.
		 */
		static uint32_t shakingDownEvaluation(const InstanceData& project, const InstanceSolution& solution, uint32_t *bestScheduleStartTimesById);
		/*!
		 * \param project The data of the instance.
		 * \param startTimesById Start time values of activities ordered by ID's.
		 * \return Precedence penalty of the schedule.
		 * \brief Method compute precedence penalty (= broken relation between two activities) of the schedule.
		 * \note Because precedence free swaps and shifts are currently used, this function is only for debugging purposes.
		 */
		static uint32_t computePrecedencePenalty(const InstanceData& project, const uint32_t * const& startTimesById);
		/*!
		 * \param project The data of the project.
		 * \param solution A solution of the project.
		 * \param i Index at activitiesOrder.
		 * \param j Index at activitiesOrder.
		 * \return True if and only if precedence penalty is zero else false.
		 * \brief Method check if candidate for swap is precedence penalty free.
		 */
		static bool checkSwapPrecedencePenalty(const InstanceData& project, const InstanceSolution& solution, uint32_t i, uint32_t j);

		/*!
		 * \param project The data of the instance.
		 * \param solution Current solution of the instance.
		 * \param startTimesById The earliest start time values in the order W.
		 * \brief It transforms the earliest start time values to the order W. The order W is written to the variable solution.orderOfActivities.
		 */
		static void convertStartTimesById2ActivitiesOrder(const InstanceData& project, InstanceSolution& solution, const uint32_t * const& startTimesById);
		/*!
		 * \param project The data of the instance.
		 * \param solution Current solution of the instance.
		 * \param timeValuesById Assigned time values to activities, it is used for sorting input order.
		 * \brief Input order of activities is sorted in accordance with time values. It's stable sort.
		 */
		static void insertSort(const InstanceData& project, InstanceSolution& solution, const uint32_t * const& timeValuesById);

		/*!
		 * \param order Activities order.
		 * \param diff Direction and norm of shift move.
		 * \param baseIdx Base index from which the shift will be performed.
		 * \brief Change activities order. Activity at index baseIdx is shifted to the left or to the right (diff parameter). 
		 */
		static void makeShift(uint32_t * const& order, const int32_t& diff, const uint32_t& baseIdx);

		/*!
		 * \param project The data of the instance.
		 * \param solution A solution in which a diversification will be performed.
		 * \brief Random swaps are performed when diversification is called..
		 */
		static void makeDiversification(const InstanceData& project, InstanceSolution& solution);

		/*!
		 * \param project The data of the project instance.
		 * \brief The method swaps directions of all precedence edges in the project data-structure.
		 */
		static void changeDirectionOfEdges(InstanceData& project);

		/*!
		 * \param activityId The activity from which all related activities are found.
		 * \param numberOfRelated The number of related activities for each activity.
		 * \param related The related (= successors || predecessors) activities for each activity in the project.
		 * \param numberOfActivities The total number of activities in the project.
		 * \return It returns all activityId's successors or predecessors.
		 */
		static std::vector<uint32_t>* getAllRelatedActivities(uint32_t activityId, uint32_t *numberOfRelated, uint32_t **related, uint32_t numberOfActivities);
		/*!
		 * \param activityId Identification of the activity.
		 * \param project The data of the project.
		 * \return It returns all activityId's successors.
		 */
		static std::vector<uint32_t>* getAllActivitySuccessors(const uint32_t& activityId, const InstanceData& project);
		/*!
		 * \param activityId Identification of the activity.
		 * \param project The data of the project.
		 * \return It returns all activityId's predecessors.
		 */
		static std::vector<uint32_t>* getAllActivityPredecessors(const uint32_t& activityId, const InstanceData& project);


	private:

		//! Copy constructor is forbidden.
		ScheduleSolver(const ScheduleSolver&);
		//! Assignment operator is forbidden.
		ScheduleSolver& operator=(const ScheduleSolver&);


		static uint32_t *copyAndPush(uint32_t* array, uint32_t size, uint32_t value);

		/* IMMUTABLE DATA */

		//! A static parameters of a RCPSP project.
		struct InstanceData	{
			//! Number of renewable sources.
			uint32_t numberOfResources;
			//! The capacity of the resources.
			uint32_t *capacityOfResources;
			//! Total number of activities.
			uint32_t numberOfActivities;
			//! Duration of activities.
			uint32_t *durationOfActivities;
			//! Activities successors;
			uint32_t **successorsOfActivity;
			//! Number of successors that activities.
			uint32_t *numberOfSuccessors;
			//! Precomputed predecessors.
			uint32_t **predecessorsOfActivity;
			//! Number of predecessors.
			uint32_t *numberOfPredecessors;
			//! Sources that are required by activities.
			uint32_t **requiredResourcesOfActivities;
			//! Matrix of successors. (if matrix(i,j) == 1 then "Exist precedence edge between activities i and j")
			int8_t **matrixOfSuccessors;
			//! Critical Path Makespan. (Critical Path Method)
			int32_t criticalPathMakespan;
			//! The longest paths from the end activity in the transformed graph.
			uint32_t *rightLeftLongestPaths;
			//! Upper bound of Cmax (sum of all activity durations).
			uint32_t upperBoundMakespan;
			//! All successors of an activity. Cache purposes.
			std::vector<std::vector<uint32_t>*> allSuccessorsCache;
			//! All predecessors of an activity. Cache purposes.
			std::vector<std::vector<uint32_t>*> allPredecessorsCache;
			//! The matrix of disjunctive activities.
			bool **disjunctiveActivities;
			//! An artificially added directed edge to the problem.
			struct Edge {
				//! The start node.
				uint32_t i;
				//! The end node.
				uint32_t j;
				//! A weight of the edge.
				int32_t weight;
			};
			//! A list of added edges to the problem.
			std::vector<Edge> addedEdges;
		};

		//! The data of the read instance.
		InstanceData instance;


		/* MUTABLE DATA */	

		//! A solution of a project is stored in this structure.
		struct InstanceSolution	{
			//! Current activities order.
			uint32_t *orderOfActivities;
			//! Best schedule order.
			uint32_t *bestScheduleOrder;
			//! Cost of the best schedule.
			uint32_t costOfBestSchedule;
		};

		//! The current solution of the read instance.
		InstanceSolution instanceSolution;

		//! Tabu list instance.
		TabuList *tabu;
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

