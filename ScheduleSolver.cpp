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

#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

#include "ScheduleSolver.h"
#include "SourcesLoad.h"
#include "SimpleTabuList.h"
#include "AdvancedTabuList.h"

using namespace std;

ScheduleSolver::ScheduleSolver(const InputReader& rcpspData) : tabu(NULL), totalRunTime(0) 	{
	// Copy pointers to data of instance.
	numberOfResources = rcpspData.getNumberOfResources();
	capacityOfResources = rcpspData.getCapacityOfResources();
	numberOfActivities = rcpspData.getNumberOfActivities();
	activitiesDuration = rcpspData.getActivitiesDuration();
	numberOfSuccessors = rcpspData.getActivitiesNumberOfSuccessors();
	activitiesSuccessors = rcpspData.getActivitiesSuccessors();
	activitiesResources = rcpspData.getActivitiesResources();

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

	// Add first task with id 0. (currentLevel contain ID's)
	uint8_t *currentLevel = new uint8_t[numberOfActivities];
	uint8_t *newCurrentLevel = new uint8_t[numberOfActivities];
	memset(currentLevel, 0, sizeof(uint8_t)*numberOfActivities);

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

	// Current schedule index.
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

	for (uint32_t i = 0; i < numberOfActivities; ++i)	{
		for (uint32_t j = 0; j < numberOfActivities; ++j)	{
			for (uint32_t k = 0; k < numberOfActivities; ++k)	{
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


	/* IMPROVE COST OF INIT SOLUTION */

	for (uint32_t i = 1; i < numberOfActivities-2; ++i)	{
		uint32_t bestI = i, bestJ = i;
		uint32_t bestCost = evaluateOrder(activitiesOrder);
		for (uint32_t j = i+1; j < numberOfActivities-1; ++j)	{
			if (checkSwapPrecedencePenalty(i,j))	{
				swap(activitiesOrder[i], activitiesOrder[j]);
				uint32_t cost = evaluateOrder(activitiesOrder);
				if (cost < bestCost)	{
					bestI = i; bestJ = j;
					bestCost = cost;
				}
				swap(activitiesOrder[i], activitiesOrder[j]);
			}
		}
		if (bestI != bestJ)
			swap(activitiesOrder[bestI], activitiesOrder[bestJ]);
	}

	uint32_t *outBestSchedule = bestScheduleOrder;
	copy(activitiesOrder, activitiesOrder+numberOfActivities, outBestSchedule);
	costOfBestSchedule = evaluateOrder(activitiesOrder);
}

uint32_t ScheduleSolver::evaluateOrder(const uint32_t * const& order, uint32_t *startTimesWriter, uint32_t *startTimesWriterById)	const	{
	bool freeMem = false;
	uint32_t start = 0, scheduleLength = 0;
	SourcesLoad load(numberOfResources,capacityOfResources);
	if (startTimesWriterById == NULL)	{
		startTimesWriterById = new uint32_t[numberOfActivities];
		freeMem = true;
	}
	memset(startTimesWriterById, 0, sizeof(uint32_t)*numberOfActivities);

	for (uint32_t i = 0; i < numberOfActivities; ++i)	{
		uint32_t activityId = order[i];
		for (uint32_t j = 0; j < numberOfPredecessors[activityId]; ++j)	{
			uint32_t predecessorId = activitiesPredecessors[activityId][j];
			start = max(startTimesWriterById[predecessorId]+activitiesDuration[predecessorId], start);
		}

		start = max(load.getEarliestStartTime(activitiesResources[activityId]), start);
		load.addActivity(start, start+activitiesDuration[activityId], activitiesResources[activityId]);
		scheduleLength = max(scheduleLength, start+activitiesDuration[activityId]);

		if (startTimesWriter != NULL)
			*(startTimesWriter++) = start;

		startTimesWriterById[activityId] = start;
	}

	if (freeMem == true)
		delete[] startTimesWriterById;

	return scheduleLength;
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

void ScheduleSolver::printSchedule(const uint32_t * const& scheduleOrder, bool verbose, ostream& OUT)	const	{
	uint32_t *startTimes = new uint32_t[numberOfActivities];
	uint32_t *startTimesById = new uint32_t[numberOfActivities];

	size_t scheduleLength = evaluateOrder(scheduleOrder, startTimes, startTimesById);
	size_t precedencePenalty = computePrecedencePenalty(startTimesById);
	
	if (verbose == true)	{
		int32_t startTime = -1;
		OUT<<"start\tactivities"<<endl;
		for (uint32_t i = 0; i < numberOfActivities; ++i)	{
			if (startTime != ((int32_t) startTimes[i]))	{
				if (i != 0) OUT<<endl;
				OUT<<startTimes[i]<<":\t"<<(scheduleOrder[i]+1);
				startTime = startTimes[i];
			} else {
				OUT<<" "<<(scheduleOrder[i]+1);
			}
		}
		OUT<<endl;
		OUT<<"Schedule length: "<<scheduleLength<<endl;
		OUT<<"Precedence penalty: "<<precedencePenalty<<endl;
		OUT<<"Critical path makespan: "<<criticalPathMakespan<<endl;
		OUT<<"Schedule solve time: "<<totalRunTime<<" s"<<endl;
	}	else	{
		OUT<<scheduleLength<<"+"<<precedencePenalty<<" "<<criticalPathMakespan<<"\t["<<totalRunTime<<" s]"<<endl;
	}

	delete[] startTimesById;
	delete[] startTimes;
}

void ScheduleSolver::printBestSchedule(bool verbose, ostream& OUT)	const	{
	printSchedule(bestScheduleOrder,verbose, OUT);
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


void ScheduleSolver::solveSchedule(const uint32_t& maxIter, const string& graphFilename)	{
	#ifdef __GNUC__
	timeval startTime, endTime, diffTime;
	gettimeofday(&startTime, NULL);
	#elif defined _WIN32 || defined _WIN64 || defined WIN32 || defined WIN64
	LARGE_INTEGER ticksPerSecond;
	LARGE_INTEGER startTimeStamp, stopTimeStamp;
	QueryPerformanceFrequency(&ticksPerSecond);
	QueryPerformanceCounter(&startTimeStamp); 
	#endif

	srand(time(NULL));
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

	for (uint32_t iter = 0; iter < maxIter; ++iter)	{
		size_t neighborhoodSize = 0;
		MoveType iterBestMove = NONE;
		uint32_t iterBestI = 0, iterBestJ = 0, iterShiftDiff = 0;
		uint32_t iterBestEval = UINT32_MAX;

		uint32_t *currentScheduleStartTimes = new uint32_t[numberOfActivities];
		evaluateOrder(activitiesOrder, currentScheduleStartTimes);

		#pragma omp parallel reduction(+:neighborhoodSize)
		{
			/* PRIVATE DATA FOR EVERY THREAD */
			MoveType threadBestMove = NONE;
			uint32_t threadBestI = 0, threadBestJ = 0, threadShiftDiff = 0;
			size_t threadBestEval = UINT32_MAX;
			size_t threadNeighborhoodCounter = 0;

			// Each thread own copy of current order.
			uint32_t *threadOrder = new uint32_t[numberOfActivities], *outThreadOrder = threadOrder;
			copy(activitiesOrder, activitiesOrder+numberOfActivities, outThreadOrder);

			/* HUGE COMPUTING... */
			#pragma omp for schedule(dynamic)
			for (uint32_t i = 1; i < numberOfActivities-1; ++i)	{

				/* SWAP MOVES */
				uint32_t u = min(i+1+ConfigureRCPSP::SWAP_RANGE, numberOfActivities-1);
				for (uint32_t j = i+1; j < u; ++j)	{

					// Check if current selected swap is precedence penalty free.
					bool precedenceFree = checkSwapPrecedencePenalty(i, j);

					if (((currentScheduleStartTimes[i] != currentScheduleStartTimes[j]) || (currentScheduleStartTimes[i-1] != currentScheduleStartTimes[i])) && (precedenceFree == true))	{
						swap(threadOrder[i], threadOrder[j]);

						uint32_t totalMoveCost = evaluateOrder(threadOrder);
						bool isPossibleMove = tabu->isPossibleMove(i, j, SWAP);

						if ((isPossibleMove == true && threadBestEval > totalMoveCost) || totalMoveCost < costOfBestSchedule)	{
							threadBestI = i; threadBestJ = j; threadBestMove = SWAP;
							threadBestEval = totalMoveCost;
							++threadNeighborhoodCounter;
						}

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
							// shift < i-1
							for (uint32_t k = shift; k < i; ++k)	{
								if (relationMatrix[activitiesOrder[k]][activitiesOrder[i]] == 1)	{
									penaltyFree = false;
									break;
								}
							}
						}

						if (penaltyFree == true)	{
							makeShift(threadOrder, ((int32_t) shift)-((int32_t) i), i);

							uint32_t totalMoveCost = evaluateOrder(threadOrder);
							bool isPossibleMove = tabu->isPossibleMove(i, i, SHIFT);

							if ((isPossibleMove == true && threadBestEval > totalMoveCost) || totalMoveCost < costOfBestSchedule)	{
								threadBestI = threadBestJ = i; threadBestMove = SHIFT;
								threadBestEval = totalMoveCost; threadShiftDiff = shift;
								++threadNeighborhoodCounter;
							}

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

			delete[] threadOrder;
		}

		delete[] currentScheduleStartTimes;

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
			costOfBestSchedule = iterBestEval;
			numberOfIterSinceBest = 0;
			tabu->bestSolutionFound();
			uint32_t *outBestSchedule = bestScheduleOrder;
			copy(activitiesOrder, activitiesOrder+numberOfActivities, outBestSchedule);
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

