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

#include "ScheduleSolver.h"
#include "SourcesLoad.h"
#include "ConfigureRCPSP.h"

using namespace std;

ScheduleSolver::ScheduleSolver(uint32_t resNum, uint32_t *capRes, uint32_t actNum, uint32_t *actDur, uint32_t **actSuc, uint32_t *actNumSuc, uint32_t **actRes, uint32_t maxIter)
	: numberOfResources(resNum), capacityOfResources(capRes), numberOfActivities(actNum), activitiesDuration(actDur), activitiesSuccessors(actSuc), numberOfSuccessors(actNumSuc),
	  activitesResources(actRes), maxIterToDiversification(maxIter), tabu(actNum, SIMPLE_TABU_LIST_SIZE), totalRunTime(0)	{

	activitiesOrder = new uint32_t[numberOfActivities];
	diversificationOrder = new uint32_t[numberOfActivities];
	bestScheduleOrder = new uint32_t[numberOfActivities];

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
/*
	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		cout<<"Activity "<<activityId+1<<":";
		for (uint32_t predIdx = 0; predIdx < numberOfPredecessors[activityId]; ++predIdx)	{
			cout<<" "<<activitiesPredecessors[activityId][predIdx]+1;
		}
		cout<<endl;
	}
*/

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

/*	
	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		cout<<"Activity "<<activityId+1<<": "<<levels[activityId]<<endl;
	}
*/
	
	// Current schedule index.
	uint32_t schedIdx = 0;
	for (uint32_t curDeep = 0; curDeep < deep; ++curDeep)	{
		for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
			if (levels[activityId] == curDeep)
				activitiesOrder[schedIdx++] = activityId;
		}
	}

	uint32_t *outDiversification = diversificationOrder, *outBestSchedule = bestScheduleOrder;
	copy(activitiesOrder, activitiesOrder+numberOfActivities, outBestSchedule);
	copy(activitiesOrder, activitiesOrder+numberOfActivities, outDiversification);	

	costOfBestSchedule = evaluateOrder(activitiesOrder);

	delete[] levels;
	delete[] currentLevel;
	delete[] newCurrentLevel;
}

uint32_t ScheduleSolver::evaluateOrder(const uint32_t *order, uint32_t *startTimesWriter, uint32_t *startTimesWriterById)	const	{
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

		start = max(load.getEarliestStartTime(activitesResources[activityId]), start);
		load.addActivity(start, start+activitiesDuration[activityId], activitesResources[activityId]);
		scheduleLength = max(scheduleLength, start+activitiesDuration[activityId]);

		if (startTimesWriter != NULL)
			*(startTimesWriter++) = start;

		startTimesWriterById[activityId] = start;
	}

	if (freeMem == true)
		delete[] startTimesWriterById;

	return scheduleLength;
}

uint32_t ScheduleSolver::computePrecedencePenalty(const uint32_t *startTimesById)	const	{
	uint32_t penalty = 0;
	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		for (uint32_t j = 0; j < numberOfSuccessors[activityId]; ++j)	{
			uint32_t successorId = activitiesSuccessors[activityId][j];	
			if (startTimesById[activityId]+activitiesDuration[activityId] > startTimesById[successorId])
				penalty += startTimesById[activityId]+activitiesDuration[activityId]-startTimesById[successorId];
		}
	}
	return PRECEDENCE_PENALTY*penalty;
}

void ScheduleSolver::printSchedule(uint32_t *scheduleOrder, bool verbose, ostream& OUT)	const	{
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
		OUT<<"Schedule solve time: "<<totalRunTime<<" s"<<endl;
	}	else	{
		OUT<<scheduleLength<<"+"<<precedencePenalty<<"\t["<<totalRunTime<<" s]"<<endl;
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
	delete[] diversificationOrder;
	delete[] bestScheduleOrder;
}


void ScheduleSolver::solveSchedule(const uint32_t& maxIter)	{
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

	// Prepare matrix of successors (~1) / predecessors (~-1).
	int8_t ** relationMatrix = new int8_t*[numberOfActivities];
	for (int8_t** ptr = relationMatrix; ptr < relationMatrix+numberOfActivities; ++ptr)	{
		*ptr = new int8_t[numberOfActivities];
		memset(*ptr, 0, sizeof(int8_t)*numberOfActivities);
	}

	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		for (uint32_t j = 0; j < numberOfSuccessors[activityId]; ++j)	{
			relationMatrix[activityId][activitiesSuccessors[activityId][j]] = 1;
		}
	}

	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		for (uint32_t j = 0; j < numberOfPredecessors[activityId]; ++j)	{
			relationMatrix[activityId][activitiesPredecessors[activityId][j]] = -1;
		}
	}

	for (uint32_t iter = 0; iter < maxIter; ++iter)	{
		size_t neighborhoodSize = 0;
		MoveType iterBestMove = NONE;
		uint32_t iterBestI = 0, iterBestJ = 0, iterShiftDiff = 0;
		size_t iterBestEval = UINT32_MAX;

		uint32_t *currentScheduleStartTimes = new uint32_t[numberOfActivities];
		uint32_t *currentScheduleStartTimesById = new uint32_t[numberOfActivities];
		evaluateOrder(activitiesOrder, currentScheduleStartTimes, currentScheduleStartTimesById);
		delete[] currentScheduleStartTimesById;

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
			uint32_t *threadStartTimesById = new uint32_t[numberOfActivities];

			/* HUGE COMPUTING... */
			#pragma omp for schedule(dynamic)
			for (uint32_t i = 1; i < numberOfActivities-1; ++i)	{

				/* SWAP MOVES */
				uint32_t u = min(i+1+SWAP_RANGE, numberOfActivities-1);
				for (uint32_t j = i+1; j < u; ++j)	{

					// Check if current selected swap is precedence penalty free.
					bool precedenceFree = true;
					for (uint32_t k = i; k < j; ++k)	{
						if (relationMatrix[activitiesOrder[k]][activitiesOrder[j]] == 1)	{
							precedenceFree = false;
							break;
						}
					}

					if ((currentScheduleStartTimes[i] != currentScheduleStartTimes[j]) && (precedenceFree == true))	{
						swap(threadOrder[i], threadOrder[j]);
						#ifdef SIMPLE_TABU
						bool isPossibleMove = tabu.isPossibleMove(i, j);
						#endif
						#ifdef ADVANCE_TABU
						bool isPossibleMove = tabu.isPossibleMove(i, j, SWAP);
						#endif
						uint32_t totalMoveCost = evaluateOrder(threadOrder, NULL, threadStartTimesById);
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
				uint32_t minStartIdx = max(1, ((int32_t) i)-SHIFT_RANGE);
				uint32_t maxStartIdx = min(i+1+SHIFT_RANGE, numberOfActivities-1);

				for (uint32_t shift = minStartIdx; shift < maxStartIdx; ++shift)	{
					if (shift > i+1 || shift < i-1)	{
						makeShift(threadOrder, ((int32_t) shift)-((int32_t) i), i);
						#ifdef SIMPLE_TABU
						bool isPossibleMove = tabu.isPossibleMove(i, i);
						#endif
						#ifdef ADVANCE_TABU
						bool isPossibleMove = tabu.isPossibleMove(i, i, SHIFT);
						#endif
						uint32_t scheduleLength = evaluateOrder(threadOrder, NULL, threadStartTimesById);
						uint32_t precedencePenalty = computePrecedencePenalty(threadStartTimesById);
						uint32_t totalMoveCost = scheduleLength+precedencePenalty;
						if ((isPossibleMove == true && threadBestEval > totalMoveCost) || totalMoveCost < costOfBestSchedule)	{
							threadBestI = threadBestJ = i; threadBestMove = SHIFT;
							threadBestEval = totalMoveCost; threadShiftDiff = shift;
							++threadNeighborhoodCounter;
						}
						makeShift(threadOrder, ((int32_t) i)-((int32_t) shift), shift);
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
			delete[] threadStartTimesById;
		}

		delete[] currentScheduleStartTimes;

		/* CHECK BEST SOLUTION AND UPDATE TABU LIST */

		if (neighborhoodSize > 0)	{
			#ifdef SIMPLE_TABU
			if (tabu.isPossibleMove(iterBestI, iterBestJ))	{
				tabu.addTurnToTabuList(iterBestI, iterBestJ);
			}
			#endif
			#ifdef ADVANCE_TABU
			if (tabu.isPossibleMove(iterBestI, iterBestJ, iterBestMove))	{
				tabu.addTurnToTabuList(iterBestI, iterBestJ, iterBestMove);
			}
			#endif
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
		//	cout<<"New best cost "<<iter<<": "<<iterBestEval<<endl;
			costOfBestSchedule = iterBestEval;
			numberOfIterSinceBest = 0;
			#ifdef ADVANCE_TABU
			tabu.bestSolutionFound();
			#endif
			uint32_t *outBestSchedule = bestScheduleOrder;
			copy(activitiesOrder, activitiesOrder+numberOfActivities, outBestSchedule);
		} else {
			++numberOfIterSinceBest;
		}

		if (numberOfIterSinceBest > maxIterToDiversification)	{
			numberOfIterSinceBest = 0;
		}
		#ifdef ADVANCE_TABU
		tabu.goToNextIter();
		#endif
	}

	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)
		delete[] relationMatrix[activityId];
	delete[] relationMatrix;

	#ifdef __GNUC__
	gettimeofday(&endTime, NULL);
	timersub(&endTime, &startTime, &diffTime);
	totalRunTime = diffTime.tv_sec+diffTime.tv_usec/1000000.;
	#elif defined _WIN32 || defined _WIN64 || defined WIN32 || defined WIN64
	QueryPerformanceCounter(&stopTimeStamp);
	totalRunTime = (stopTimeStamp.QuadPart-startTimeStamp.QuadPart)/((double) ticksPerSecond.QuadPart);
	#endif
}


void ScheduleSolver::makeShift(uint32_t *order, int32_t diff, uint32_t baseIdx)	const	{
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

