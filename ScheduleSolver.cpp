#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <fstream>
#include <set>
#include <string>
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
						totalMoveCost += computeUpperBoundsOverhangPenalty(costOfBestSchedule-1, threadStartTimesById);

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
							totalMoveCost += computeUpperBoundsOverhangPenalty(costOfBestSchedule-1, threadStartTimesById);
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
			costOfBestSchedule = iterBestEval;
			uint32_t *bestScheduleStartTimesById = new uint32_t[numberOfActivities];
			uint32_t shakedCost = shakingDownEvaluation(activitiesOrder, bestScheduleStartTimesById);
			if (shakedCost < costOfBestSchedule)	{
				convertStartTimesById2ActivitiesOrder(activitiesOrder, bestScheduleStartTimesById);
				costOfBestSchedule = shakedCost;
			}
			uint32_t *outBestSchedule = bestScheduleOrder;
			copy(activitiesOrder, activitiesOrder+numberOfActivities, outBestSchedule);
			tabu->bestSolutionFound();
			numberOfIterSinceBest = 0;
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

void ScheduleSolver::writeBestScheduleToFile(const string& fileName) {
	ofstream out(fileName, ios::out | ios::binary | ios::trunc);

	/* WRITE INTANCE DATA */
	out.write((const char*) &numberOfActivities, sizeof(uint32_t));
	out.write((const char*) &numberOfResources, sizeof(uint32_t));

	out.write((const char*) activitiesDuration, numberOfActivities*sizeof(uint32_t));
	out.write((const char*) capacityOfResources, numberOfResources*sizeof(uint32_t));
	for (uint32_t i = 0; i < numberOfActivities; ++i)
		out.write((const char*) activitiesResources[i], numberOfResources*sizeof(uint32_t));

	out.write((const char*) numberOfSuccessors, numberOfActivities*sizeof(uint32_t));
	for (uint32_t i = 0; i < numberOfActivities; ++i)
		out.write((const char*) activitiesSuccessors[i], numberOfSuccessors[i]*sizeof(uint32_t));

	out.write((const char*) numberOfPredecessors, numberOfActivities*sizeof(uint32_t));
	for (uint32_t i = 0; i < numberOfActivities; ++i)
		out.write((const char*) activitiesPredecessors[i], numberOfPredecessors[i]*sizeof(uint32_t));

	/* WRITE RESULTS */
	uint32_t *startTimesById = new uint32_t[numberOfActivities], *copyOrder = new uint32_t[numberOfActivities];
	uint32_t scheduleLength = shakingDownEvaluation(bestScheduleOrder, startTimesById);

	uint32_t *copyWr = copyOrder;
	copy(bestScheduleOrder, bestScheduleOrder+numberOfActivities, copyWr);
	convertStartTimesById2ActivitiesOrder(copyOrder, startTimesById);

	out.write((const char*) &scheduleLength, sizeof(uint32_t));
	out.write((const char*) copyOrder, numberOfActivities*sizeof(uint32_t));
	out.write((const char*) startTimesById, numberOfActivities*sizeof(uint32_t));

	delete[] startTimesById;
	delete[] copyOrder;

	out.close();
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

	delete[] rightLeftLongestPaths;

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


	/* PRECOMPUTE MATRIX OF SUCCESSORS */

	relationMatrix = new int8_t*[numberOfActivities];
	for (uint32_t i = 0; i < numberOfActivities; ++i)	{
		relationMatrix[i] = new int8_t[numberOfActivities];
		memset(relationMatrix[i], 0, sizeof(int8_t)*numberOfActivities);
	}

	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		for (uint32_t j = 0; j < numberOfSuccessors[activityId]; ++j)	{
			relationMatrix[activityId][activitiesSuccessors[activityId][j]] = 1;
		}
	}

	/* IT COMPUTES THE CRITICAL PATH LENGTH */
	uint32_t *lb1 = computeLowerBounds(0);
	if (numberOfActivities > 1)
		criticalPathMakespan = lb1[numberOfActivities-1];
	else
		criticalPathMakespan = -1;
	delete[] lb1;

	/*
	 * It transformes the instance graph. Directions of edges are changed.
	 * The longest paths are computed from the end dummy activity to the others.
	 * After that the graph is transformed back.
	 */
	swap(numberOfSuccessors, numberOfPredecessors);
	swap(activitiesSuccessors, activitiesPredecessors);
	rightLeftLongestPaths = computeLowerBounds(numberOfActivities-1, true);
	swap(numberOfSuccessors, numberOfPredecessors);
	swap(activitiesSuccessors, activitiesPredecessors);

	/* CREATE AND COPY INITIAL SCHEDULE TO THE BEST SCHEDULE */

	uint32_t *bestScheduleStartTimesById = new uint32_t[numberOfActivities];
	costOfBestSchedule = shakingDownEvaluation(activitiesOrder, bestScheduleStartTimesById);
	convertStartTimesById2ActivitiesOrder(activitiesOrder, bestScheduleStartTimesById);
	uint32_t *outBestSchedule = bestScheduleOrder;
	copy(activitiesOrder, activitiesOrder+numberOfActivities, outBestSchedule);

	delete[] bestScheduleStartTimesById;
}

uint32_t* ScheduleSolver::computeLowerBounds(const uint32_t& startActivityId, const bool& energyReasoning) const {
	// The first dummy activity is added to list.
	list<uint32_t> expandedNodes(1, startActivityId);
	// We have to remember closed activities. (the bound of the activity is determined)
	bool *closedActivities = new bool[numberOfActivities];
	fill(closedActivities, closedActivities+numberOfActivities, false);
	// The longest path from the start activity to the activity at index "i".
	uint32_t *maxDistances = new uint32_t[numberOfActivities];
	fill(maxDistances, maxDistances+numberOfActivities, 0);
	// All branches that go through nodes are saved.
	// branches[i][j] = p -> The p-nd branch that started in the node j goes through node i.
	map<uint32_t, uint32_t> * branches = new map<uint32_t, uint32_t>[numberOfActivities];

	while (!expandedNodes.empty())	{
		uint32_t activityId;
		uint32_t minimalStartTime;
		// We select the first activity with all predecessors closed.
		list<uint32_t>::iterator lit = expandedNodes.begin();
		while (lit != expandedNodes.end())	{
			activityId = *lit;
			if (closedActivities[activityId] == false)	{
				minimalStartTime = 0;
				bool allPredecessorsClosed = true;
				vector<map<uint32_t, uint32_t> > predecessorsBranches;
				uint32_t *activityPredecessors = activitiesPredecessors[activityId];
				for (uint32_t* p = activityPredecessors; p < activityPredecessors+numberOfPredecessors[activityId]; ++p)	{
					if (closedActivities[*p] == false)	{
						allPredecessorsClosed = false;
						break;
					} else {
						// It updates the maximal distance from the start activity to the activity "activityId".
						minimalStartTime = max(maxDistances[*p]+activitiesDuration[*p], minimalStartTime);
						if (numberOfPredecessors[activityId] > 1 && energyReasoning)
							predecessorsBranches.push_back(branches[*p]);
					}
				}
				if (allPredecessorsClosed)	{
					if (numberOfPredecessors[activityId] > 1 && energyReasoning) {
						// Output branches are found out for the node with more predecessors.
						map<uint32_t, uint32_t> newBranches;
						set<uint32_t> startNodesOfMultiPaths;
						for (uint32_t k = 0; k < predecessorsBranches.size(); ++k)	{
							map<uint32_t, uint32_t>& m = predecessorsBranches[k];
							for (map<uint32_t, uint32_t>::const_iterator mit = m.begin(); mit != m.end(); ++mit)	{
								map<uint32_t, uint32_t>::const_iterator sit;
								if ((sit = newBranches.find(mit->first)) == newBranches.end())	{
									newBranches[mit->first] = mit->second;
								} else {
									// The branch number has to be checked.
									if (mit->second != sit->second)	{
										// Multi-paths were detected! New start node is stored.
										startNodesOfMultiPaths.insert(mit->first);
									}
								}
							}
						}
						branches[activityId] = newBranches;
						// If more than one path exists to the node "activityId", then the resource restrictions
						// are taken into accout to improve lower bound.
						uint32_t minimalResourceStartTime = 0;
						for (set<uint32_t>::const_iterator sit = startNodesOfMultiPaths.begin(); sit != startNodesOfMultiPaths.end(); ++sit)	{
							// Vectors are sorted by activity id's.
							vector<uint32_t> allSuccessors = getAllActivitySuccessors(*sit);
							vector<uint32_t> allPredecessors = getAllActivityPredecessors(activityId);
							// The vector of all activities between the activity "i" and activity "j".
							vector<uint32_t> intersectionOfActivities;
							set_intersection(allPredecessors.begin(), allPredecessors.end(), allSuccessors.begin(),
									allSuccessors.end(), back_inserter(intersectionOfActivities));
							for (uint32_t k = 0; k < numberOfResources; ++k)	{
								uint32_t sumOfEnergy = 0, timeInterval;
								for (uint32_t i = 0; i < intersectionOfActivities.size(); ++i)	{
									uint32_t innerActivityId = intersectionOfActivities[i];
									sumOfEnergy += activitiesDuration[innerActivityId]*activitiesResources[innerActivityId][k];
								}
								timeInterval = sumOfEnergy/capacityOfResources[k];
								if ((sumOfEnergy % capacityOfResources[k]) != 0)
									++timeInterval;
								
								minimalResourceStartTime = max(minimalResourceStartTime, 
										maxDistances[*sit]+activitiesDuration[*sit]+timeInterval); 
							}
						}
						minimalStartTime = max(minimalStartTime, minimalResourceStartTime);
					}
					break;
				}

				++lit;
			} else {
				lit = expandedNodes.erase(lit);
			}
		}
		
		if (lit != expandedNodes.end())	{
			closedActivities[activityId] = true;
			maxDistances[activityId] = minimalStartTime;
			expandedNodes.erase(lit);
			uint32_t numberOfSuccessorsOfClosedActivity = numberOfSuccessors[activityId];
			for (uint32_t s = 0; s < numberOfSuccessorsOfClosedActivity; ++s)	{
				uint32_t successorId = activitiesSuccessors[activityId][s];
				if (numberOfPredecessors[successorId] <= 1 && energyReasoning)	{
					branches[successorId] = branches[activityId];
					if (numberOfSuccessorsOfClosedActivity > 1)	{
						branches[successorId][activityId] = s;
					}
				}
				expandedNodes.push_back(successorId);
			}
		} else {
			break;
		}
	}

	delete[] branches;
	delete[] closedActivities; 

	return maxDistances;
}

uint32_t ScheduleSolver::computeUpperBoundsOverhangPenalty(const uint32_t& makespan, const uint32_t * const& startTimesById)	const	{
	uint32_t overhangPenalty = 0;
	for (uint32_t id = 0; id < numberOfActivities; ++id)	{
		if (startTimesById[id]+activitiesDuration[id]+rightLeftLongestPaths[id] > makespan)	{
			overhangPenalty += startTimesById[id]+activitiesDuration[id]+rightLeftLongestPaths[id]-makespan;
		}
	}
	return overhangPenalty;
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
						output<<c<<":\t"<<id;
						first = false;
					} else {
						output<<" "<<id;
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

vector<uint32_t> ScheduleSolver::getAllRelatedActivities(uint32_t activityId, uint32_t *numberOfRelated, uint32_t **related) const	{
	vector<uint32_t> relatedActivities;
	bool *activitiesSet = new bool[numberOfActivities];
	fill(activitiesSet, activitiesSet+numberOfActivities, false);

	for (uint32_t j = 0; j < numberOfRelated[activityId]; ++j)	{
		activitiesSet[related[activityId][j]] = true;
		vector<uint32_t> indirectRelated = getAllRelatedActivities(related[activityId][j], numberOfRelated, related);
		for (vector<uint32_t>::const_iterator it = indirectRelated.begin(); it != indirectRelated.end(); ++it)
			activitiesSet[*it] = true;
	}

	for (uint32_t id = 0; id < numberOfActivities; ++id)	{
		if (activitiesSet[id] == true)
			relatedActivities.push_back(id);
	}
	
	delete[] activitiesSet;
	return relatedActivities;
}

vector<uint32_t> ScheduleSolver::getAllActivitySuccessors(const uint32_t& activityId) const	{
	return getAllRelatedActivities(activityId, numberOfSuccessors, activitiesSuccessors);
}

vector<uint32_t> ScheduleSolver::getAllActivityPredecessors(const uint32_t& activityId) const 	{
	return getAllRelatedActivities(activityId, numberOfPredecessors, activitiesPredecessors);
}

