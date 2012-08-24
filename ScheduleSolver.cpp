#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <stdexcept>

#ifdef __GNUC__
#include <sys/time.h>
#elif defined _WIN32 || defined _WIN64 || defined WIN32 || defined WIN64
#include <Windows.h>
#endif

#include "AdvancedTabuList.h"
#include "ScheduleSolver.h"
#include "SimpleTabuList.h"
#include "SourcesLoadCapacityResolution.h"
#include "SourcesLoadTimeResolution.h"

#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

using namespace std;

ScheduleSolver::ScheduleSolver(const InputReader& rcpspData) : tabu(NULL), totalRunTime(0), numberOfEvaluatedSchedules(0) 	{
	// Copy pointers to data of instance.
	numberOfResources = rcpspData.getNumberOfResources();
	capacityOfResources = rcpspData.getCapacityOfResources();
	numberOfActivities = rcpspData.getNumberOfActivities();
	activitiesDuration = rcpspData.getActivitiesDuration();
	numberOfSuccessors = rcpspData.getActivitiesNumberOfSuccessors();
	activitiesSuccessors = rcpspData.getActivitiesSuccessors();
	activitiesResources = rcpspData.getActivitiesResources();

	// It computes the estimate of the longest duration of the project.
	upperBoundMakespan = accumulate(activitiesDuration, activitiesDuration+numberOfActivities, 0);

	// Create desired type of tabu list.
	if (ConfigureRCPSP::TABU_LIST_TYPE == SIMPLE_TABU)
		tabu = new SimpleTabuList(numberOfActivities, ConfigureRCPSP::SIMPLE_TABU_LIST_SIZE);
	else if (ConfigureRCPSP::TABU_LIST_TYPE == ADVANCED_TABU)
		tabu = new AdvancedTabuList(ConfigureRCPSP::MAXIMAL_NUMBER_OF_ITERATIONS_SINCE_BEST);
	else
		throw invalid_argument("ScheduleSolver::ScheduleSolver: Invalid type of tabu list!");

	// Create initial solution and fill required data structures.
	createInitialSolution();
}

void ScheduleSolver::solveSchedule(const uint32_t& maxIter, const string& graphFilename)	{
	#ifdef __GNUC__
	timeval startTime, endTime, diffTime;
	timeval startTimeIter, endTimeIter, diffTimeIter;
	gettimeofday(&startTime, NULL);
	#elif defined _WIN32 || defined _WIN64 || defined WIN32 || defined WIN64
	LARGE_INTEGER ticksPerSecond;
	LARGE_INTEGER startTimeStamp, stopTimeStamp;
	LARGE_INTEGER startTimeIterStamp, stopTimeIterStamp;
	QueryPerformanceFrequency(&ticksPerSecond);
	QueryPerformanceCounter(&startTimeStamp); 
	#endif

	srand(time(NULL));
	numberOfEvaluatedSchedules = 0;
	uint32_t numberOfIterSinceBest = 0;
	FILE *graphFile = NULL;
	if (ConfigureRCPSP::WRITE_GRAPH == true && !graphFilename.empty())	{
		graphFile = fopen(graphFilename.c_str(), "w");
		if (graphFile == NULL)	{
			cerr<<"ScheduleSolver::solveSchedule: Cannot write csv file! Check permissions."<<endl;
		}	else	{
			fprintf(graphFile, "Iteration number; Current criterion; Current best known makespan;\n");
			fprintf(graphFile, "0; %u; %u;\n", costOfBestSchedule, costOfBestSchedule);
		}
	}

	for (uint32_t iter = 0; iter < maxIter && ((uint32_t) criticalPathMakespan) < costOfBestSchedule; ++iter)	{
		size_t neighborhoodSize = 0;
		MoveType iterBestMove = NONE;
		uint32_t iterBestI = 0, iterBestJ = 0, iterShiftDiff = 0;
		uint32_t iterBestEval = UINT32_MAX;

		if ((iter % 100) == 0 || (iter % 100) == 1)	{
			#ifdef __GNUC__
			gettimeofday(&startTimeIter, NULL);
			#elif defined _WIN32 || defined _WIN64 || defined WIN32 || defined WIN64
			QueryPerformanceCounter(&startTimeIterStamp); 
			#endif
			algo = (iter % 100) == 0 ? CAPACITY_RESOLUTION : TIME_RESOLUTION;
		}

		if ((iter % 100) == 2)
			algo = reqTimePerIterForCapacityResAlg < reqTimePerIterForTimeResAlg ? CAPACITY_RESOLUTION : TIME_RESOLUTION;

		uint64_t evaluatedSchedulesInIteration = 0;
		#pragma omp parallel reduction(+:neighborhoodSize,evaluatedSchedulesInIteration)
		{
			/* PRIVATE DATA FOR EVERY THREAD */
			MoveType threadBestMove = NONE;
			uint32_t threadBestI = 0, threadBestJ = 0, threadShiftDiff = 0;
			uint32_t threadBestEval = UINT32_MAX;;
			size_t threadNeighborhoodCounter = 0;

			// Each thread own copy of current order.
			uint32_t *threadOrder = new uint32_t[numberOfActivities], *outThreadOrder = threadOrder;
			copy(activitiesOrder, activitiesOrder+numberOfActivities, outThreadOrder);
			uint32_t *threadStartTimesById = new uint32_t[numberOfActivities];

			/* HUGE COMPUTING... */
			#pragma omp for schedule(dynamic)
			for (uint32_t i = 1; i < numberOfActivities-1; ++i)	{

				/* SWAP MOVES */
				uint32_t u = min(i+1+ConfigureRCPSP::SWAP_RANGE, numberOfActivities-1);
				for (uint32_t j = i+1; j < u; ++j)	{

					// Check if current selected swap is precedence penalty free.
					bool precedenceFree = checkSwapPrecedencePenalty(i, j);

					if (precedenceFree == true)	{
						swap(threadOrder[i], threadOrder[j]);

						uint32_t totalMoveCost = forwardScheduleEvaluation(threadOrder, threadStartTimesById);

						bool isPossibleMove = tabu->isPossibleMove(i, j, SWAP);

						if ((isPossibleMove == true && threadBestEval > totalMoveCost) || totalMoveCost < costOfBestSchedule)	{
							threadBestI = i; threadBestJ = j; threadBestMove = SWAP;
							threadBestEval = totalMoveCost;
							++threadNeighborhoodCounter;
						}
						++evaluatedSchedulesInIteration;

						swap(threadOrder[i], threadOrder[j]);
					} else if (relationMatrix[activitiesOrder[i]][activitiesOrder[j]] == 1)	{
						break;
					}
				}

				/* SHIFT MOVES */
				uint32_t minStartIdx = max(1, ((int32_t) i)-((int32_t) ConfigureRCPSP::SHIFT_RANGE));
				uint32_t maxStartIdx = min(i+1+ConfigureRCPSP::SHIFT_RANGE, numberOfActivities-1);

				for (uint32_t shift = minStartIdx; shift < maxStartIdx; ++shift)	{
					if (shift > i+1 || shift < i-1)	{

						bool penaltyFree = true;
						if (shift > i+1)	{
							for (uint32_t k = i+1; k < shift+1; ++k)	{
								if (relationMatrix[activitiesOrder[i]][activitiesOrder[k]] == 1)	{
									penaltyFree = false;
									shift = maxStartIdx;
									break;
								}
							}
						} else {
							for (uint32_t k = shift; k < i; ++k)	{
								if (relationMatrix[activitiesOrder[k]][activitiesOrder[i]] == 1)	{
									penaltyFree = false;
									break;
								}
							}
						}

						if (penaltyFree == true)	{
							makeShift(threadOrder, ((int32_t) shift)-((int32_t) i), i);

							uint32_t totalMoveCost = forwardScheduleEvaluation(threadOrder, threadStartTimesById);
							bool isPossibleMove = tabu->isPossibleMove(i, i, SHIFT);

							if ((isPossibleMove == true && threadBestEval > totalMoveCost) || totalMoveCost < costOfBestSchedule)	{
								threadBestI = threadBestJ = i; threadBestMove = SHIFT;
								threadBestEval = totalMoveCost; threadShiftDiff = shift;
								++threadNeighborhoodCounter;
							}
							++evaluatedSchedulesInIteration;

							makeShift(threadOrder, ((int32_t) i)-((int32_t) shift), shift);
						}
					}
				}
			}

			/* MERGE RESULTS */
			if (threadNeighborhoodCounter > 0)	{
				neighborhoodSize += threadNeighborhoodCounter;
				#pragma omp critical
				{
					if (threadBestEval < iterBestEval)	{
						iterBestI = threadBestI;
						iterBestJ = threadBestJ;
						iterBestMove = threadBestMove;
						iterShiftDiff = threadShiftDiff;
						iterBestEval = threadBestEval;
					}
				}
			}

			delete[] threadStartTimesById;
			delete[] threadOrder;
		}

		if ((iter % 100) == 0 || (iter % 100) == 1)	{
			#ifdef __GNUC__
			gettimeofday(&endTimeIter, NULL);
			timersub(&endTimeIter, &startTimeIter, &diffTimeIter);
			double iterRunTime = diffTimeIter.tv_sec+diffTimeIter.tv_usec/1000000.;
			#elif defined _WIN32 || defined _WIN64 || defined WIN32 || defined WIN64
			QueryPerformanceCounter(&stopTimeIterStamp);
			double iterRunTime = (stopTimeIterStamp.QuadPart-startTimeIterStamp.QuadPart)/((double) ticksPerSecond.QuadPart);
			#endif
			if ((iter % 100) == 1)
				reqTimePerIterForTimeResAlg = iterRunTime;
			else
				reqTimePerIterForCapacityResAlg = iterRunTime;
		}

		/* CHECK BEST SOLUTION AND UPDATE TABU LIST */

		if (neighborhoodSize > 0)	{
			if ((iterBestMove == SWAP) && (tabu->isPossibleMove(iterBestI, iterBestJ, SWAP) == true))
				tabu->addTurnToTabuList(iterBestI, iterBestJ, SWAP);
			else if ((iterBestMove == SHIFT) && (tabu->isPossibleMove(iterBestI, iterBestI, SHIFT) == true))
				tabu->addTurnToTabuList(iterBestI, iterBestI, SHIFT);
		} else {
			clog<<"Expanded neighborhood is empty! Prematurely ending..."<<endl;
			break;
		}

		// Apply best move.
		switch (iterBestMove)	{
			case SWAP:
				swap(activitiesOrder[iterBestI], activitiesOrder[iterBestJ]);
				break;
			case SHIFT:
				makeShift(activitiesOrder, ((int32_t) iterShiftDiff)-((int32_t) iterBestI), iterBestI);
				break;
			default:
				throw runtime_error("ScheduleSolver::solveSchedule: Unsupported type of move!");
		}

		if (iterBestEval < costOfBestSchedule)	{
			bool bestSolutionFound = false;
			uint32_t *bestScheduleStartTimesById = new uint32_t[numberOfActivities];
			uint32_t shakedCost = shakingDownEvaluation(activitiesOrder, bestScheduleStartTimesById);
			if (shakedCost < costOfBestSchedule)	{
				convertStartTimesById2ActivitiesOrder(activitiesOrder, bestScheduleStartTimesById);
				costOfBestSchedule = shakedCost;
				bestSolutionFound = true;
			} else if (iterBestEval < costOfBestSchedule)	{
				costOfBestSchedule = iterBestEval;
				bestSolutionFound = true;
			}
			if (bestSolutionFound == true)	{
				uint32_t *outBestSchedule = bestScheduleOrder;
				copy(activitiesOrder, activitiesOrder+numberOfActivities, outBestSchedule);
				tabu->bestSolutionFound();
				numberOfIterSinceBest = 0;
			}
			delete[] bestScheduleStartTimesById;
		} else {
			++numberOfIterSinceBest;
		}

		if (graphFile != NULL)	{
			fprintf(graphFile, "%u; %u; %u;\n", iter+1u, iterBestEval, costOfBestSchedule);
		}

		if (numberOfIterSinceBest > ConfigureRCPSP::MAXIMAL_NUMBER_OF_ITERATIONS_SINCE_BEST)	{
			makeDiversification();
			numberOfIterSinceBest = 0;
		}

		tabu->goToNextIter();
		numberOfEvaluatedSchedules += evaluatedSchedulesInIteration;
	}

	if (graphFile != NULL)	{
		fclose(graphFile);
	}

	#ifdef __GNUC__
	gettimeofday(&endTime, NULL);
	timersub(&endTime, &startTime, &diffTime);
	totalRunTime = diffTime.tv_sec+diffTime.tv_usec/1000000.;
	#elif defined _WIN32 || defined _WIN64 || defined WIN32 || defined WIN64
	QueryPerformanceCounter(&stopTimeStamp);
	totalRunTime = (stopTimeStamp.QuadPart-startTimeStamp.QuadPart)/((double) ticksPerSecond.QuadPart);
	#endif
}

void ScheduleSolver::printBestSchedule(bool verbose, ostream& output)	{
	printSchedule(bestScheduleOrder,verbose, output);
}

ScheduleSolver::~ScheduleSolver()	{
	for (uint32_t actId = 0; actId < numberOfActivities; ++actId)	{
		delete[] activitiesPredecessors[actId];
	}
	delete[] activitiesPredecessors;

	delete[] numberOfPredecessors;
	delete[] activitiesOrder;
	delete[] bestScheduleOrder;

	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)
		delete[] relationMatrix[activityId];
	delete[] relationMatrix;

	delete tabu;
}

void ScheduleSolver::createInitialSolution()	{

	/* PRECOMPUTE ACTIVITIES PREDECESSORS */

	activitiesPredecessors = new uint32_t*[numberOfActivities];
	numberOfPredecessors = new uint32_t[numberOfActivities];
	memset(numberOfPredecessors, 0, sizeof(uint32_t)*numberOfActivities);

	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		for (uint32_t successorIdx = 0; successorIdx < numberOfSuccessors[activityId]; ++successorIdx)	{
			uint32_t successorId = activitiesSuccessors[activityId][successorIdx];
			++numberOfPredecessors[successorId];
		}
	}

	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		activitiesPredecessors[activityId] = new uint32_t[numberOfPredecessors[activityId]];
	}

	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		for (uint32_t successorIdx = 0; successorIdx < numberOfSuccessors[activityId]; ++successorIdx)	{
			uint32_t successorId = activitiesSuccessors[activityId][successorIdx];
			*(activitiesPredecessors[successorId]) = activityId;	
			++activitiesPredecessors[successorId];
		}
	}

	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		activitiesPredecessors[activityId] -= numberOfPredecessors[activityId];
	}


	/* CREATE INIT ORDER OF ACTIVITIES */

	uint32_t deep = 0;
	uint32_t *levels = new uint32_t[numberOfActivities];
	memset(levels, 0, sizeof(uint32_t)*numberOfActivities);

	uint8_t *currentLevel = new uint8_t[numberOfActivities];
	uint8_t *newCurrentLevel = new uint8_t[numberOfActivities];
	memset(currentLevel, 0, sizeof(uint8_t)*numberOfActivities);

	// Add first task with id 0. (currentLevel contain ID's)
	currentLevel[0] = 1;
	bool anyActivity = true;

	while (anyActivity == true)	{
		anyActivity = false;
		memset(newCurrentLevel, 0, sizeof(uint8_t)*numberOfActivities);
		for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
			if (currentLevel[activityId] == 1)	{
				for (uint32_t nextLevelIdx = 0; nextLevelIdx < numberOfSuccessors[activityId]; ++nextLevelIdx)	{
					newCurrentLevel[activitiesSuccessors[activityId][nextLevelIdx]] = 1;
					anyActivity = true;
				}
				levels[activityId] = deep;
			}
		}

		swap(currentLevel, newCurrentLevel);
		++deep;
	}

	activitiesOrder = new uint32_t[numberOfActivities];
	bestScheduleOrder = new uint32_t[numberOfActivities];

	uint32_t schedIdx = 0;
	for (uint32_t curDeep = 0; curDeep < deep; ++curDeep)	{
		for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
			if (levels[activityId] == curDeep)
				activitiesOrder[schedIdx++] = activityId;
		}
	}

	delete[] levels;
	delete[] currentLevel;
	delete[] newCurrentLevel;


	/* PRECOMPUTE MATRIX OF SUCCESSORS AND CRITICAL PATH MAKESPAN */

	relationMatrix = new int8_t*[numberOfActivities];
	int32_t **distanceMatrix = new int32_t*[numberOfActivities];
	for (uint32_t i = 0; i < numberOfActivities; ++i)	{
		relationMatrix[i] = new int8_t[numberOfActivities];
		memset(relationMatrix[i], 0, sizeof(int8_t)*numberOfActivities);
		distanceMatrix[i] = new int32_t[numberOfActivities];
		memset(distanceMatrix[i], -1, sizeof(int32_t)*numberOfActivities);
	}

	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		for (uint32_t j = 0; j < numberOfSuccessors[activityId]; ++j)	{
			relationMatrix[activityId][activitiesSuccessors[activityId][j]] = 1;
			distanceMatrix[activityId][activitiesSuccessors[activityId][j]] = activitiesDuration[activityId];
		}
	}

	for (uint32_t k = 0; k < numberOfActivities; ++k)	{
		for (uint32_t i = 0; i < numberOfActivities; ++i)	{
			for (uint32_t j = 0; j < numberOfActivities; ++j)	{
				if (distanceMatrix[i][k] != -1 && distanceMatrix[k][j] != -1)	{
					if (distanceMatrix[i][j] != -1)	{
						if (distanceMatrix[i][j] < distanceMatrix[i][k]+distanceMatrix[k][j])
							distanceMatrix[i][j] = distanceMatrix[i][k]+distanceMatrix[k][j];
					} else {
						distanceMatrix[i][j] = distanceMatrix[i][k]+distanceMatrix[k][j];
					}
				}
			}
		}
	}

	if (numberOfActivities > 1)
		criticalPathMakespan = distanceMatrix[0][numberOfActivities-1];
	else
		criticalPathMakespan = -1;

	for (uint32_t i = 0; i < numberOfActivities; ++i)
		delete[] distanceMatrix[i];
	delete[] distanceMatrix;


	/* COPY INITIAL SCHEDULE TO THE BEST SCHEDULE */

	uint32_t *bestScheduleStartTimesById = new uint32_t[numberOfActivities];
	costOfBestSchedule = shakingDownEvaluation(activitiesOrder, bestScheduleStartTimesById);
	convertStartTimesById2ActivitiesOrder(activitiesOrder, bestScheduleStartTimesById);
	uint32_t *outBestSchedule = bestScheduleOrder;
	copy(activitiesOrder, activitiesOrder+numberOfActivities, outBestSchedule);

	delete[] bestScheduleStartTimesById;
}

void ScheduleSolver::printSchedule(const uint32_t * const& scheduleOrder, bool verbose, ostream& output)	{
	uint32_t *startTimesById = new uint32_t[numberOfActivities];
	uint32_t scheduleLength = shakingDownEvaluation(scheduleOrder, startTimesById);
	uint32_t precedencePenalty = computePrecedencePenalty(startTimesById);
	
	if (verbose == true)	{
		output<<"start\tactivities"<<endl;
		for (uint32_t c = 0; c <= scheduleLength; ++c)	{
			bool first = true;
			for (uint32_t id = 0; id < numberOfActivities; ++id)	{
				if (startTimesById[id] == c)	{
					if (first == true)	{
						output<<c<<":\t"<<id+1;
						first = false;
					} else {
						output<<" "<<id+1;
					}
				}
			}
			if (!first)	output<<endl;
		}
		output<<"Schedule length: "<<scheduleLength<<endl;
		output<<"Precedence penalty: "<<precedencePenalty<<endl;
		output<<"Critical path makespan: "<<criticalPathMakespan<<endl;
		output<<"Schedule solve time: "<<totalRunTime<<" s"<<endl;
		output<<"Total number of evaluated schedules: "<<numberOfEvaluatedSchedules<<endl;
	}	else	{
		output<<scheduleLength<<"+"<<precedencePenalty<<" "<<criticalPathMakespan<<"\t["<<totalRunTime<<" s]\t"<<numberOfEvaluatedSchedules<<endl;
	}

	delete[] startTimesById;
}

uint32_t ScheduleSolver::evaluateOrder(const uint32_t * const& order, const uint32_t * const * const& relatedActivities,
	       	const uint32_t * const& numberOfRelatedActivities, uint32_t *& timeValuesById, bool forwardEvaluation) const {
	SourcesLoad *sourcesLoad;
	if (algo == CAPACITY_RESOLUTION)
		sourcesLoad = new SourcesLoadCapacityResolution(numberOfResources, capacityOfResources);
	else
		sourcesLoad = new SourcesLoadTimeResolution(numberOfResources, capacityOfResources, upperBoundMakespan);

	uint32_t scheduleLength = 0;
	for (uint32_t i = 0; i < numberOfActivities; ++i)	{
		uint32_t start = 0;
		uint32_t activityId = order[forwardEvaluation == true ? i : numberOfActivities-i-1];
		for (uint32_t j = 0; j < numberOfRelatedActivities[activityId]; ++j)	{
			uint32_t relatedActivityId = relatedActivities[activityId][j];
			start = max(timeValuesById[relatedActivityId]+activitiesDuration[relatedActivityId], start);
		}

		start = max(sourcesLoad->getEarliestStartTime(activitiesResources[activityId], start, activitiesDuration[activityId]), start);
		sourcesLoad->addActivity(start, start+activitiesDuration[activityId], activitiesResources[activityId]);
		scheduleLength = max(scheduleLength, start+activitiesDuration[activityId]);

		timeValuesById[activityId] = start;
	}

	delete sourcesLoad;
	return scheduleLength;
}

uint32_t ScheduleSolver::forwardScheduleEvaluation(const uint32_t * const& order, uint32_t *& startTimesById) const {
	return evaluateOrder(order, activitiesPredecessors, numberOfPredecessors, startTimesById, true);
}

uint32_t ScheduleSolver::backwardScheduleEvaluation(const uint32_t * const& order, uint32_t *& startTimesById) const {
	uint32_t makespan = evaluateOrder(order, activitiesSuccessors, numberOfSuccessors, startTimesById, false);
	// It computes the latest start time value for each activity.
	for (uint32_t id = 0; id < numberOfActivities; ++id)
		startTimesById[id] = makespan-startTimesById[id]-activitiesDuration[id];
	return makespan;
}

uint32_t ScheduleSolver::shakingDownEvaluation(const uint32_t * const& order, uint32_t *bestScheduleStartTimesById)	{
	// Select time resolution algorithm to evaluate resources. It works better than capacity resolution algorithm.
	EvaluationAlgorithm mainEvaluationAlgorithm = algo;
	algo = TIME_RESOLUTION;

	uint32_t scheduleLength = 0;
	uint32_t bestScheduleLength = UINT32_MAX;
	uint32_t *currentOrder = new uint32_t[numberOfActivities];
	uint32_t *timeValuesById = new uint32_t[numberOfActivities];

	for (uint32_t i = 0; i < numberOfActivities; ++i)
		currentOrder[i] = order[i];

	while (true)	{
		// Forward schedule...
		scheduleLength = forwardScheduleEvaluation(currentOrder, timeValuesById);
		if (scheduleLength < bestScheduleLength)	{
			bestScheduleLength = scheduleLength;
			if (bestScheduleStartTimesById != NULL)	{
				for (uint32_t id = 0; id < numberOfActivities; ++id)
					bestScheduleStartTimesById[id] = timeValuesById[id];
			}
		} else	{
			// No additional improvement can be found...
			break;
		}

		// It computes the earliest activities finish time.
		for (uint32_t id = 0; id < numberOfActivities; ++id)
			timeValuesById[id] += activitiesDuration[id];

		// Sort for backward phase..
		insertSort(currentOrder, timeValuesById, numberOfActivities);

		// Backward phase.
		uint32_t scheduleLengthBackward = backwardScheduleEvaluation(currentOrder, timeValuesById);
		int32_t diffCmax = scheduleLength-scheduleLengthBackward;

		// It computes the latest start time of activities.
		for (uint32_t id = 0; id < numberOfActivities; ++id)	{
			if (((int32_t) timeValuesById[id])+diffCmax > 0)
				timeValuesById[id] += diffCmax;
			else
				timeValuesById[id] = 0;
		}

		// Sort for forward phase..
		insertSort(currentOrder, timeValuesById, numberOfActivities);
	}

	delete[] currentOrder;
	delete[] timeValuesById;

	// The original evaluation algorithm is selected again.
	algo = mainEvaluationAlgorithm;

	return bestScheduleLength;
}

uint32_t ScheduleSolver::computePrecedencePenalty(const uint32_t * const& startTimesById)	const	{
	uint32_t penalty = 0;
	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		for (uint32_t j = 0; j < numberOfSuccessors[activityId]; ++j)	{
			uint32_t successorId = activitiesSuccessors[activityId][j];	
			if (startTimesById[activityId]+activitiesDuration[activityId] > startTimesById[successorId])
				penalty += startTimesById[activityId]+activitiesDuration[activityId]-startTimesById[successorId];
		}
	}
	return penalty;
}

bool ScheduleSolver::checkSwapPrecedencePenalty(uint32_t i, uint32_t j) const	{
	if (i > j) swap(i,j);
	for (uint32_t k = i; k < j; ++k)	{
		if (relationMatrix[activitiesOrder[k]][activitiesOrder[j]] == 1)	{
			return false;
		}
	}
	for (uint32_t k = i+1; k <= j; ++k)	{
		if (relationMatrix[activitiesOrder[i]][activitiesOrder[k]] == 1)	{
			return false;
		}
	}
	return true;
}

void ScheduleSolver::convertStartTimesById2ActivitiesOrder(uint32_t *order, const uint32_t * const& startTimesById) const {
	insertSort(order, startTimesById, numberOfActivities);
}

void ScheduleSolver::insertSort(uint32_t* order, const uint32_t * const& timeValuesById, const int32_t& size) {
	for (int32_t i = 1; i < size; ++i)	{
		for (int32_t j = i; (j > 0) && ((timeValuesById[order[j]] < timeValuesById[order[j-1]]) == true); --j)	{
			swap(order[j], order[j-1]);
		}
	}
}

void ScheduleSolver::makeShift(uint32_t * const& order, const int32_t& diff, const uint32_t& baseIdx)	const	{
	if (diff > 0)	{
		for (uint32_t i = baseIdx; i < baseIdx+diff; ++i)	{
			swap(order[i], order[i+1]);	
		}
	} else {
		for (int32_t i = baseIdx; i > ((int32_t) baseIdx)+diff; --i)	{
			swap(order[i], order[i-1]);
		}
	}
	return;
}

void ScheduleSolver::makeDiversification()	{
	uint32_t performedSwaps = 0;
	while (performedSwaps < ConfigureRCPSP::DIVERSIFICATION_SWAPS)	{
		uint32_t i = (rand() % (numberOfActivities-2)) + 1;
		uint32_t j = (rand() % (numberOfActivities-2)) + 1;

		if ((i != j) && (checkSwapPrecedencePenalty(i, j) == true))	{
			swap(activitiesOrder[i], activitiesOrder[j]);
			++performedSwaps;
		}
	}
}

