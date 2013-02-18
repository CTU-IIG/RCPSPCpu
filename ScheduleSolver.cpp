#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <list>
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
	instance.numberOfResources = rcpspData.getNumberOfResources();
	instance.capacityOfResources = rcpspData.getCapacityOfResources();
	instance.numberOfActivities = rcpspData.getNumberOfActivities();
	instance.durationOfActivities = rcpspData.getActivitiesDuration();
	instance.numberOfSuccessors = rcpspData.getActivitiesNumberOfSuccessors();
	instance.successorsOfActivity = rcpspData.getActivitiesSuccessors();
	instance.requiredResourcesOfActivities = rcpspData.getActivitiesResources();

	// Create desired type of tabu list.
	if (ConfigureRCPSP::TABU_LIST_TYPE == SIMPLE_TABU)
		tabu = new SimpleTabuList(instance.numberOfActivities, ConfigureRCPSP::SIMPLE_TABU_LIST_SIZE);
	else if (ConfigureRCPSP::TABU_LIST_TYPE == ADVANCED_TABU)
		tabu = new AdvancedTabuList(ConfigureRCPSP::MAXIMAL_NUMBER_OF_ITERATIONS_SINCE_BEST);
	else
		throw invalid_argument("ScheduleSolver::ScheduleSolver: Invalid type of tabu list!");

	// Create initial solution and fill required data structures.
	initialiseInstanceDataAndInitialSolution(instance, instanceSolution);
	cout<<"Lower bound: "<<lowerBoundOfMakespan(instance)<<endl;
/*
	map<uint32_t, uint32_t> counters;
	for (vector<pair<uint32_t,uint32_t> >::const_iterator it = possibleCandidates.begin(); it != possibleCandidates.end(); ++it)	{
		// update cache + activities data.	
		uint32_t i = it->first, j = it->second;
		// Add the edge to the graph.
		uint32_t *newDirectSuccessorsOfActivityI = new uint32_t[numberOfSuccessors[i]+1];
		copy(successorsOfActivity[i], successorsOfActivity[i]+numberOfSuccessors[i], newDirectSuccessorsOfActivityI);
		newDirectSuccessorsOfActivityI[numberOfSuccessors[i]] = j;
		delete[] successorsOfActivity[i]; successorsOfActivity[i] = newDirectSuccessorsOfActivityI;
		uint32_t *newDirectPredecessorsOfActivityJ = new uint32_t[numberOfPredecessors[j]+1];
		copy(predecessorsOfActivity[j], predecessorsOfActivity[j]+numberOfPredecessors[j], newDirectPredecessorsOfActivityJ);
		newDirectPredecessorsOfActivityJ[numberOfPredecessors[j]] = i;
		delete[] predecessorsOfActivity[j]; predecessorsOfActivity[j] = newDirectPredecessorsOfActivityJ;
		numberOfSuccessors[i] += 1; numberOfPredecessors[j] += 1;

		// Regenerate cache.
		vector<vector<uint32_t> > sucCache = allSuccessorsCache, predCache = allPredecessorsCache;
		vector<uint32_t> activityJPredecessors = getAllActivityPredecessors(j);
		vector<uint32_t> activityISuccessors = getAllActivitySuccessors(i);
		for (uint32_t i = 0; i < activityJPredecessors.size(); ++i)	{
			vector<uint32_t> result;
			back_insert_iterator<vector<uint32_t> > back_insert(result);
			set_union(allSuccessorsCache[activityJPredecessors[i]].begin(), allSuccessorsCache[activityJPredecessors[i]].end(), activityISuccessors.begin(), activityISuccessors.end(), back_insert);
			allSuccessorsCache[activityJPredecessors[i]] = result;
		}

		for (uint32_t j = 0; j < activityISuccessors.size(); ++j)	{
			vector<uint32_t> result;
			back_insert_iterator<vector<uint32_t> > back_insert(result);
			set_union(allPredecessorsCache[activityISuccessors[j]].begin(), allPredecessorsCache[activityISuccessors[j]].end(), activityJPredecessors.begin(), activityJPredecessors.end(), back_insert);
			allPredecessorsCache[activityISuccessors[j]] = result;
		}

		uint32_t lowerBound = lowerBoundOfMakespan();
		counters[lowerBound]++;

		allSuccessorsCache = sucCache; allPredecessorsCache = predCache;
		numberOfSuccessors[i] -= 1; numberOfPredecessors[j] -= 1;
	}

	for (map<uint32_t,uint32_t>::const_iterator it = counters.begin(); it != counters.end(); ++it)	{
		cout<<it->first<<" -> "<<it->second<<endl;
	} */
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
			fprintf(graphFile, "0; %u; %u;\n", instanceSolution.costOfBestSchedule, instanceSolution.costOfBestSchedule);
		}
	}

	for (uint32_t iter = 0; iter < maxIter && ((uint32_t) instance.criticalPathMakespan) < instanceSolution.costOfBestSchedule; ++iter)	{
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
			InstanceSolution threadSolution = instanceSolution;
			uint32_t *threadStartTimesById = new uint32_t[instance.numberOfActivities];
			threadSolution.orderOfActivities = new uint32_t[instance.numberOfActivities];
			copy(instanceSolution.orderOfActivities, instanceSolution.orderOfActivities+instance.numberOfActivities, threadSolution.orderOfActivities);

			/* HUGE COMPUTING... */
			#pragma omp for schedule(dynamic)
			for (uint32_t i = 1; i < instance.numberOfActivities-1; ++i)	{

				/* SWAP MOVES */
				uint32_t u = min(i+1+ConfigureRCPSP::SWAP_RANGE, instance.numberOfActivities-1);
				for (uint32_t j = i+1; j < u; ++j)	{

					// Check if current selected swap is precedence penalty free.
					bool precedenceFree = checkSwapPrecedencePenalty(instance, instanceSolution, i, j);

					if (precedenceFree == true)	{
						swap(threadSolution.orderOfActivities[i], threadSolution.orderOfActivities[j]);

						uint32_t totalMoveCost = forwardScheduleEvaluation(instance, threadSolution, threadStartTimesById, algo);
						totalMoveCost += computeUpperBoundsOverhangPenalty(instance, instanceSolution, threadStartTimesById);

						bool isPossibleMove = tabu->isPossibleMove(i, j, SWAP);

						if ((isPossibleMove == true && threadBestEval > totalMoveCost) || totalMoveCost < instanceSolution.costOfBestSchedule)	{
							threadBestI = i; threadBestJ = j; threadBestMove = SWAP;
							threadBestEval = totalMoveCost;
							++threadNeighborhoodCounter;
						}
						++evaluatedSchedulesInIteration;

						swap(threadSolution.orderOfActivities[i], threadSolution.orderOfActivities[j]);
					} else if (instance.matrixOfSuccessors[instanceSolution.orderOfActivities[i]][instanceSolution.orderOfActivities[j]] == 1)	{
						break;
					}
				}

				/* SHIFT MOVES */
				uint32_t minStartIdx = max(1, ((int32_t) i)-((int32_t) ConfigureRCPSP::SHIFT_RANGE));
				uint32_t maxStartIdx = min(i+1+ConfigureRCPSP::SHIFT_RANGE, instance.numberOfActivities-1);

				for (uint32_t shift = minStartIdx; shift < maxStartIdx; ++shift)	{
					if (shift > i+1 || shift < i-1)	{

						bool penaltyFree = true;
						if (shift > i+1)	{
							for (uint32_t k = i+1; k < shift+1; ++k)	{
								if (instance.matrixOfSuccessors[instanceSolution.orderOfActivities[i]][instanceSolution.orderOfActivities[k]] == 1)	{
									penaltyFree = false;
									shift = maxStartIdx;
									break;
								}
							}
						} else {
							for (uint32_t k = shift; k < i; ++k)	{
								if (instance.matrixOfSuccessors[instanceSolution.orderOfActivities[k]][instanceSolution.orderOfActivities[i]] == 1)	{
									penaltyFree = false;
									break;
								}
							}
						}

						if (penaltyFree == true)	{
							makeShift(threadSolution.orderOfActivities, ((int32_t) shift)-((int32_t) i), i);

							uint32_t totalMoveCost = forwardScheduleEvaluation(instance, threadSolution, threadStartTimesById, algo);
							totalMoveCost += computeUpperBoundsOverhangPenalty(instance, instanceSolution, threadStartTimesById);
							bool isPossibleMove = tabu->isPossibleMove(i, i, SHIFT);

							if ((isPossibleMove == true && threadBestEval > totalMoveCost) || totalMoveCost < instanceSolution.costOfBestSchedule)	{
								threadBestI = threadBestJ = i; threadBestMove = SHIFT;
								threadBestEval = totalMoveCost; threadShiftDiff = shift;
								++threadNeighborhoodCounter;
							}
							++evaluatedSchedulesInIteration;

							makeShift(threadSolution.orderOfActivities, ((int32_t) i)-((int32_t) shift), shift);
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
			delete[] threadSolution.orderOfActivities;
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
				swap(instanceSolution.orderOfActivities[iterBestI], instanceSolution.orderOfActivities[iterBestJ]);
				break;
			case SHIFT:
				makeShift(instanceSolution.orderOfActivities, ((int32_t) iterShiftDiff)-((int32_t) iterBestI), iterBestI);
				break;
			default:
				throw runtime_error("ScheduleSolver::solveSchedule: Unsupported type of move!");
		}

		if (iterBestEval < instanceSolution.costOfBestSchedule)	{
			instanceSolution.costOfBestSchedule = iterBestEval;
			uint32_t *bestScheduleStartTimesById = new uint32_t[instance.numberOfActivities];
			uint32_t shakedCost = shakingDownEvaluation(instance, instanceSolution, bestScheduleStartTimesById);
			if (shakedCost < instanceSolution.costOfBestSchedule)	{
				convertStartTimesById2ActivitiesOrder(instance, instanceSolution, bestScheduleStartTimesById);
				instanceSolution.costOfBestSchedule = shakedCost;
			}
			copy(instanceSolution.orderOfActivities, instanceSolution.orderOfActivities+instance.numberOfActivities, instanceSolution.bestScheduleOrder);
			tabu->bestSolutionFound();
			numberOfIterSinceBest = 0;
			delete[] bestScheduleStartTimesById;
		} else {
			++numberOfIterSinceBest;
		}

		if (graphFile != NULL)	{
			fprintf(graphFile, "%u; %u; %u;\n", iter+1u, iterBestEval, instanceSolution.costOfBestSchedule);
		}

		if (numberOfIterSinceBest > ConfigureRCPSP::MAXIMAL_NUMBER_OF_ITERATIONS_SINCE_BEST)	{
			makeDiversification(instance, instanceSolution);
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
	printSchedule(instance, instanceSolution, totalRunTime, numberOfEvaluatedSchedules,  verbose, output);
}

void ScheduleSolver::writeBestScheduleToFile(const string& fileName) {
	ofstream out(fileName, ios::out | ios::binary | ios::trunc);
	if (!out)
		throw invalid_argument("ScheduleSolver::writeBestScheduleToFile: Cannot open the output file to write!");

	writeBestScheduleToFile(out, instance, instanceSolution).close();
}

ScheduleSolver::~ScheduleSolver()	{
	for (uint32_t i = 0; i < instance.numberOfActivities; ++i)	{
		delete[] instance.predecessorsOfActivity[i];
		delete[] instance.matrixOfSuccessors[i];
		delete instance.allSuccessorsCache[i];
		delete instance.allPredecessorsCache[i];
		delete[] instance.disjunctiveActivities[i];
	}

	delete[] instance.numberOfPredecessors;
	delete[] instance.predecessorsOfActivity;
	delete[] instance.matrixOfSuccessors;
	delete[] instance.rightLeftLongestPaths;
	delete[] instance.disjunctiveActivities;

	delete[] instanceSolution.orderOfActivities;
	delete[] instanceSolution.bestScheduleOrder;

	delete tabu;
}

void ScheduleSolver::initialiseInstanceDataAndInitialSolution(InstanceData& project, InstanceSolution& solution)	{
	// It computes the estimate of the longest duration of the project.
	project.upperBoundMakespan = accumulate(project.durationOfActivities, project.durationOfActivities+project.numberOfActivities, 0);

	/* PRECOMPUTE ACTIVITIES PREDECESSORS */

	project.predecessorsOfActivity = new uint32_t*[project.numberOfActivities];
	project.numberOfPredecessors = new uint32_t[project.numberOfActivities];
	memset(project.numberOfPredecessors, 0, sizeof(uint32_t)*project.numberOfActivities);

	for (uint32_t activityId = 0; activityId < project.numberOfActivities; ++activityId)	{
		for (uint32_t successorIdx = 0; successorIdx < project.numberOfSuccessors[activityId]; ++successorIdx)	{
			uint32_t successorId = project.successorsOfActivity[activityId][successorIdx];
			++project.numberOfPredecessors[successorId];
		}
	}

	for (uint32_t activityId = 0; activityId < project.numberOfActivities; ++activityId)	{
		project.predecessorsOfActivity[activityId] = new uint32_t[project.numberOfPredecessors[activityId]];
	}

	for (uint32_t activityId = 0; activityId < project.numberOfActivities; ++activityId)	{
		for (uint32_t successorIdx = 0; successorIdx < project.numberOfSuccessors[activityId]; ++successorIdx)	{
			uint32_t successorId = project.successorsOfActivity[activityId][successorIdx];
			*(project.predecessorsOfActivity[successorId]) = activityId;	
			++project.predecessorsOfActivity[successorId];
		}
	}

	for (uint32_t activityId = 0; activityId < project.numberOfActivities; ++activityId)	{
		project.predecessorsOfActivity[activityId] -= project.numberOfPredecessors[activityId];
	}


	/* CREATE INIT ORDER OF ACTIVITIES */

	uint32_t deep = 0;
	uint32_t *levels = new uint32_t[project.numberOfActivities];
	memset(levels, 0, sizeof(uint32_t)*project.numberOfActivities);

	uint8_t *currentLevel = new uint8_t[project.numberOfActivities];
	uint8_t *newCurrentLevel = new uint8_t[project.numberOfActivities];
	memset(currentLevel, 0, sizeof(uint8_t)*project.numberOfActivities);

	// Add first task with id 0. (currentLevel contain ID's)
	currentLevel[0] = 1;
	bool anyActivity = true;

	while (anyActivity == true)	{
		anyActivity = false;
		memset(newCurrentLevel, 0, sizeof(uint8_t)*project.numberOfActivities);
		for (uint32_t activityId = 0; activityId < project.numberOfActivities; ++activityId)	{
			if (currentLevel[activityId] == 1)	{
				for (uint32_t nextLevelIdx = 0; nextLevelIdx < project.numberOfSuccessors[activityId]; ++nextLevelIdx)	{
					newCurrentLevel[project.successorsOfActivity[activityId][nextLevelIdx]] = 1;
					anyActivity = true;
				}
				levels[activityId] = deep;
			}
		}

		swap(currentLevel, newCurrentLevel);
		++deep;
	}

	solution.orderOfActivities = new uint32_t[project.numberOfActivities];
	solution.bestScheduleOrder = new uint32_t[project.numberOfActivities];

	uint32_t schedIdx = 0;
	for (uint32_t curDeep = 0; curDeep < deep; ++curDeep)	{
		for (uint32_t activityId = 0; activityId < project.numberOfActivities; ++activityId)	{
			if (levels[activityId] == curDeep)
				solution.orderOfActivities[schedIdx++] = activityId;
		}
	}

	delete[] levels;
	delete[] currentLevel;
	delete[] newCurrentLevel;


	/* PRECOMPUTE MATRIX OF SUCCESSORS */

	project.matrixOfSuccessors = new int8_t*[project.numberOfActivities];
	for (uint32_t i = 0; i < project.numberOfActivities; ++i)	{
		project.matrixOfSuccessors[i] = new int8_t[project.numberOfActivities];
		memset(project.matrixOfSuccessors[i], 0, sizeof(int8_t)*project.numberOfActivities);
	}

	for (uint32_t activityId = 0; activityId < project.numberOfActivities; ++activityId)	{
		for (uint32_t j = 0; j < project.numberOfSuccessors[activityId]; ++j)	{
			project.matrixOfSuccessors[activityId][project.successorsOfActivity[activityId][j]] = 1;
		}
	}

	/* IT COMPUTES THE CRITICAL PATH LENGTH */
	uint32_t *lb1 = computeLowerBounds(0, project);
	if (project.numberOfActivities > 1)
		project.criticalPathMakespan = lb1[project.numberOfActivities-1];
	else
		project.criticalPathMakespan = -1;
	delete[] lb1;

	/* IT FILLS THE CACHES OF SUCCESSORS/PREDECESSORS */
	for (uint32_t id = 0; id < project.numberOfActivities; ++id)	{
		project.allSuccessorsCache.push_back(getAllActivitySuccessors(id, project));
		project.allPredecessorsCache.push_back(getAllActivityPredecessors(id, project));
	}

	/*
	 * It transformes the instance graph. Directions of edges are changed.
	 * The longest paths are computed from the end dummy activity to the others.
	 * After that the graph is transformed back.
	 */
	changeDirectionOfEdges(project);
	project.rightLeftLongestPaths = computeLowerBounds(project.numberOfActivities-1, project, true);
	changeDirectionOfEdges(project);

	/* CREATE AND COPY INITIAL SCHEDULE TO THE BEST SCHEDULE */

	uint32_t *bestScheduleStartTimesById = new uint32_t[project.numberOfActivities];
	solution.costOfBestSchedule = shakingDownEvaluation(project, solution, bestScheduleStartTimesById);
	convertStartTimesById2ActivitiesOrder(project, solution, bestScheduleStartTimesById);
	copy(solution.orderOfActivities, solution.orderOfActivities+project.numberOfActivities, solution.bestScheduleOrder);

	delete[] bestScheduleStartTimesById;

	/* COMPUTE A MATRIX OF MUTUALLY DISJUNCTIVE ACTIVITIES */

	project.disjunctiveActivities = new bool*[project.numberOfActivities];
	for (uint32_t i = 0; i < project.numberOfActivities; ++i)	{
		project.disjunctiveActivities[i] = new bool[project.numberOfActivities];
		fill(project.disjunctiveActivities[i], project.disjunctiveActivities[i]+project.numberOfActivities, false);
	}

	for (uint32_t i = 0; i < project.numberOfActivities; ++i)	{
		for (uint32_t j = i+1; j < project.numberOfActivities; ++j)	{
			bool simultaneous = true;
			if (binary_search(project.allSuccessorsCache[i]->begin(), project.allSuccessorsCache[i]->end(), j) == true)	{
				simultaneous = false;
			}
			if (simultaneous && binary_search(project.allPredecessorsCache[i]->begin(), project.allPredecessorsCache[i]->end(), j) == true)	{
				simultaneous = false;
			}

			for (uint32_t k = 0; k < project.numberOfResources && simultaneous; ++k)	{
				if (project.requiredResourcesOfActivities[i][k]+project.requiredResourcesOfActivities[j][k] > project.capacityOfResources[k])
					simultaneous = false;
			}

			// Since the matrix is symmetric then matrix[i][j] = matrix[j][i].
			project.disjunctiveActivities[j][i] = project.disjunctiveActivities[i][j] = !simultaneous;
		}
	}
/*
	for (uint32_t i = 0; i < project.numberOfActivities; ++i)	{
		uint64_t counter = 0;
		cout<<i<<"\t";
		for (uint32_t j = 0; j < project.numberOfActivities; ++j)	{
			cout<<" "<<project.disjunctiveActivities[i][j];
			if (project.disjunctiveActivities[i][j])
				++counter;
		}
		cout<<"\t"<<counter<<endl;
	} */
}

ofstream& ScheduleSolver::writeBestScheduleToFile(ofstream& out, const InstanceData& project, const InstanceSolution& solution)	{
	/* WRITE INTANCE DATA */
	out.write((const char*) &project.numberOfActivities, sizeof(uint32_t));
	out.write((const char*) &project.numberOfResources, sizeof(uint32_t));

	out.write((const char*) project.durationOfActivities, project.numberOfActivities*sizeof(uint32_t));
	out.write((const char*) project.capacityOfResources, project.numberOfResources*sizeof(uint32_t));
	for (uint32_t i = 0; i < project.numberOfActivities; ++i)
		out.write((const char*) project.requiredResourcesOfActivities[i], project.numberOfResources*sizeof(uint32_t));

	out.write((const char*) project.numberOfSuccessors, project.numberOfActivities*sizeof(uint32_t));
	for (uint32_t i = 0; i < project.numberOfActivities; ++i)
		out.write((const char*) project.successorsOfActivity[i], project.numberOfSuccessors[i]*sizeof(uint32_t));

	out.write((const char*) project.numberOfPredecessors, project.numberOfActivities*sizeof(uint32_t));
	for (uint32_t i = 0; i < project.numberOfActivities; ++i)
		out.write((const char*) project.predecessorsOfActivity[i], project.numberOfPredecessors[i]*sizeof(uint32_t));

	/* WRITE RESULTS */
	InstanceSolution copySolution = solution;
	uint32_t *startTimesById = new uint32_t[project.numberOfActivities];
	copySolution.orderOfActivities = new uint32_t[project.numberOfActivities];
	copy(solution.bestScheduleOrder, solution.bestScheduleOrder+project.numberOfActivities, copySolution.orderOfActivities);

	uint32_t scheduleLength = shakingDownEvaluation(project, solution, startTimesById);
	convertStartTimesById2ActivitiesOrder(project, copySolution, startTimesById);

	out.write((const char*) &scheduleLength, sizeof(uint32_t));
	out.write((const char*) copySolution.orderOfActivities, project.numberOfActivities*sizeof(uint32_t));
	out.write((const char*) startTimesById, project.numberOfActivities*sizeof(uint32_t));

	delete[] startTimesById;
	delete[] copySolution.orderOfActivities;

	return out;
}

uint32_t* ScheduleSolver::computeLowerBounds(const uint32_t& startActivityId, const InstanceData& project, const bool& energyReasoning) {
	// The first dummy activity is added to list.
	list<uint32_t> expandedNodes(1, startActivityId);
	// We have to remember closed activities. (the bound of the activity is determined)
	bool *closedActivities = new bool[project.numberOfActivities];
	fill(closedActivities, closedActivities+project.numberOfActivities, false);
	// The longest path from the start activity to the activity at index "i".
	uint32_t *maxDistances = new uint32_t[project.numberOfActivities];
	fill(maxDistances, maxDistances+project.numberOfActivities, 0);
	// All branches that go through nodes are saved.
	// branches[i][j] = p -> The p-nd branch that started in the node j goes through node i.
	int32_t ** branches = NULL;
	// An auxiliary array that stores all activities between the start activity and end activity.
	uint32_t *intersectionOfActivities = NULL;
	// An auxiliary array that stores the predecessors branches.
	int32_t **predecessorsBranches = NULL;
	// It allocates/initialises memory only if it is required.
	if (energyReasoning == true)	 {
		branches = new int32_t*[project.numberOfActivities];
		branches[startActivityId] = new int32_t[project.numberOfActivities];
		fill(branches[startActivityId], branches[startActivityId]+project.numberOfActivities, -1);
		for (uint32_t id = 0; id < project.numberOfActivities; ++id)	{
			if (id != startActivityId)
				branches[id] = NULL;
		}
		intersectionOfActivities = new uint32_t[project.numberOfActivities];
		predecessorsBranches = new int32_t*[project.numberOfActivities];
	}

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
				for (uint32_t p = 0; p < project.numberOfPredecessors[activityId]; ++p)	{
					uint32_t predecessor = project.predecessorsOfActivity[activityId][p];
					if (closedActivities[predecessor] == false)	{
						allPredecessorsClosed = false;
						break;
					} else {
						// It updates the maximal distance from the start activity to the activity "activityId".
						minimalStartTime = max(maxDistances[predecessor]+project.durationOfActivities[predecessor], minimalStartTime);
						if (project.numberOfPredecessors[activityId] > 1 && energyReasoning)
							predecessorsBranches[p] = branches[predecessor];
					}
				}
				if (allPredecessorsClosed)	{
					if (project.numberOfPredecessors[activityId] > 1 && energyReasoning) {
						// Output branches are found out for the node with more predecessors.
						set<uint32_t> startNodesOfMultiPaths;
						branches[activityId] = new int32_t[project.numberOfActivities];
						fill(branches[activityId], branches[activityId]+project.numberOfActivities, -1);
						for (uint32_t p = 0; p < project.numberOfPredecessors[activityId]; ++p)	{
							int32_t * activityGoThroughBranches = predecessorsBranches[p];
							for (uint32_t id = 0; id < project.numberOfActivities; ++id)	{
								if (branches[activityId][id] == -1)	{
									branches[activityId][id] = activityGoThroughBranches[id];
								} else if (activityGoThroughBranches[id] != -1) {
									// The branch number has to be checked.
									if (activityGoThroughBranches[id] != branches[activityId][id])	{
										// Multi-paths were detected! New start node is stored.
										startNodesOfMultiPaths.insert(id);
									}
								}
							}
						}
						// If more than one path exists to the node "activityId", then the resource restrictions
						// are taken into accout to improve lower bound.
						uint32_t minimalResourceStartTime = 0;
						for (set<uint32_t>::const_iterator sit = startNodesOfMultiPaths.begin(); sit != startNodesOfMultiPaths.end(); ++sit)	{
							// Vectors are sorted by activity id's.
							vector<uint32_t>* allSuccessors = project.allSuccessorsCache[*sit];
							vector<uint32_t>* allPredecessors = project.allPredecessorsCache[activityId];
							// The array of all activities between activity "i" and activity "j".
							uint32_t *intersectionEndPointer = set_intersection(allPredecessors->begin(), allPredecessors->end(),
									allSuccessors->begin(), allSuccessors->end(), intersectionOfActivities);
							for (uint32_t k = 0; k < project.numberOfResources; ++k)	{
								uint32_t sumOfEnergy = 0, timeInterval;
								for (uint32_t *id = intersectionOfActivities; id < intersectionEndPointer; ++id)	{
									uint32_t innerActivityId = *id;
									sumOfEnergy += project.durationOfActivities[innerActivityId]*project.requiredResourcesOfActivities[innerActivityId][k];
								}

								timeInterval = sumOfEnergy/project.capacityOfResources[k];
								if ((sumOfEnergy % project.capacityOfResources[k]) != 0)
									++timeInterval;
								
								minimalResourceStartTime = max(minimalResourceStartTime, maxDistances[*sit]+project.durationOfActivities[*sit]+timeInterval); 
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
			// The successors of the current activity are added.
			uint32_t numberOfSuccessorsOfClosedActivity = project.numberOfSuccessors[activityId];
			for (uint32_t s = 0; s < numberOfSuccessorsOfClosedActivity; ++s)	{
				uint32_t successorId = project.successorsOfActivity[activityId][s];
				if (project.numberOfPredecessors[successorId] <= 1 && energyReasoning)	{
					branches[successorId] = new int32_t[project.numberOfActivities];
					if (branches[activityId] == NULL)	{
						fill(branches[successorId], branches[successorId], -1);
					} else	{
						copy(branches[activityId], branches[activityId]+project.numberOfActivities, branches[successorId]);
					}

					if (numberOfSuccessorsOfClosedActivity > 1)	{
						branches[successorId][activityId] = s;
					}
				}
				expandedNodes.push_back(successorId);
			}

			// The proccessed activity is closed and its distance from the start activity is updated.
			closedActivities[activityId] = true;
			maxDistances[activityId] = minimalStartTime;
			// It erases a proccessed activity from the list.
			expandedNodes.erase(lit);
		} else {
			break;
		}
	}

	// It frees all allocated memory.
	if (energyReasoning == true)	{
		delete[] predecessorsBranches;
		delete[] intersectionOfActivities;
		for (uint32_t i = 0; i < project.numberOfActivities; ++i)
			delete[] branches[i];
		delete[] branches;
	}
	delete[] closedActivities; 

	return maxDistances;
}

uint32_t ScheduleSolver::lowerBoundOfMakespan(const InstanceData& project) {

	// The base data-structure - an element in the list for the "Extended Node Packing Bound" problem.
	struct ListActivity {
		ListActivity(uint32_t id, uint32_t par, uint32_t d) : activityId(id), concurrencyCoeff(par), duration(d) { };
		bool operator<(const ListActivity& x) const {
			if (concurrencyCoeff < x.concurrencyCoeff)
				return true;
			else if (concurrencyCoeff > x.concurrencyCoeff)
				return false;
			else if (duration > x.duration)
				return true;
			else
				return false;
		}

		uint32_t activityId;
		uint32_t concurrencyCoeff; 
		uint32_t duration;
	};

	// It creates the list of activities.
	vector<ListActivity> listOfActivities;
	for (uint32_t i = 0; i < project.numberOfActivities; ++i)	{
		uint32_t concurrencyLevel = 0;
		for (uint32_t j = 0; j < project.numberOfActivities; ++j)	{
			if (i != j && project.disjunctiveActivities[i][j] == false)
				++concurrencyLevel;
		}
		listOfActivities.push_back(ListActivity(i, concurrencyLevel, project.durationOfActivities[i]));
	}
	
	// A sorting heuristic is applied to the list.
	sort(listOfActivities.begin(), listOfActivities.end());

	InstanceData copyOfProject = project;
	uint32_t maximalLowerBound = 0, lowerBound = 0;
	copyOfProject.durationOfActivities = new uint32_t[project.numberOfActivities];
	copy(project.durationOfActivities, project.durationOfActivities+project.numberOfActivities, copyOfProject.durationOfActivities);
	for (uint32_t i = 0; i < copyOfProject.numberOfActivities; ++i)	{
		// It creates the new subset of the unprocessed activities.
		uint32_t activityId1 = listOfActivities[i].activityId, subsetDuration = listOfActivities[i].duration;
		if (subsetDuration > 0)	{
			// It computes an estimate of the lower bound.
			changeDirectionOfEdges(copyOfProject);
			uint32_t *ub = computeLowerBounds(copyOfProject.numberOfActivities-1, copyOfProject, true);
			changeDirectionOfEdges(copyOfProject);
			uint32_t *lb = computeLowerBounds(0, copyOfProject, true);
			maximalLowerBound = max(maximalLowerBound, lowerBound+max(lb[copyOfProject.numberOfActivities-1], ub[0]));
			delete[] lb; delete[] ub;

			// All activities which are able to run concurrently with activity "activityId1" have reduced duration.
			for (uint32_t j = i+1; j < copyOfProject.numberOfActivities; ++j)	{
				if (j != i)	{
					uint32_t durationJ = listOfActivities[j].duration;
					uint32_t activityId2 = listOfActivities[j].activityId;
					if (project.disjunctiveActivities[activityId1][activityId2] == false && durationJ > 0)
						copyOfProject.durationOfActivities[activityId2] = listOfActivities[j].duration = ((durationJ > subsetDuration) ? durationJ-subsetDuration : 0);
				}
			}

			// Remove the selected activity from the list.
			copyOfProject.durationOfActivities[activityId1] = listOfActivities[i].duration = 0;
			lowerBound += subsetDuration;
		}
	}
	maximalLowerBound = max(maximalLowerBound, lowerBound);

	delete[] copyOfProject.durationOfActivities;

	return maximalLowerBound;
}

uint32_t ScheduleSolver::computeUpperBoundsOverhangPenalty(const InstanceData& project, const InstanceSolution& solution, const uint32_t * const& startTimesById) 	{
	uint32_t overhangPenalty = 0;
	for (uint32_t id = 0; id < project.numberOfActivities; ++id)	{
		if (startTimesById[id]+project.durationOfActivities[id]+project.rightLeftLongestPaths[id]+1 > solution.costOfBestSchedule)	{
			overhangPenalty += startTimesById[id]+project.durationOfActivities[id]+project.rightLeftLongestPaths[id]+1-solution.costOfBestSchedule;
		}
	}
	return overhangPenalty;
}

void ScheduleSolver::printSchedule(const InstanceData& project, const InstanceSolution& solution, double runTime, uint64_t evaluatedSchedules, bool verbose, ostream& output)	{
	uint32_t *startTimesById = new uint32_t[project.numberOfActivities];
	uint32_t scheduleLength = shakingDownEvaluation(project, solution, startTimesById);
	uint32_t precedencePenalty = computePrecedencePenalty(project, startTimesById);
	
	if (verbose == true)	{
		output<<"start\tactivities"<<endl;
		for (uint32_t c = 0; c <= scheduleLength; ++c)	{
			bool first = true;
			for (uint32_t id = 0; id < project.numberOfActivities; ++id)	{
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
		output<<"Critical path makespan: "<<project.criticalPathMakespan<<endl;
		output<<"Schedule solve time: "<<runTime<<" s"<<endl;
		output<<"Total number of evaluated schedules: "<<evaluatedSchedules<<endl;
	}	else	{
		output<<scheduleLength<<"+"<<precedencePenalty<<" "<<project.criticalPathMakespan<<"\t["<<runTime<<" s]\t"<<evaluatedSchedules<<endl;
	}

	delete[] startTimesById;
}

uint32_t ScheduleSolver::evaluateOrder(const InstanceData& project, const InstanceSolution& solution, uint32_t *& timeValuesById, bool forwardEvaluation, EvaluationAlgorithm algorithm)	{
	SourcesLoad *sourcesLoad;
	if (algorithm == CAPACITY_RESOLUTION)
		sourcesLoad = new SourcesLoadCapacityResolution(project.numberOfResources, project.capacityOfResources);
	else
		sourcesLoad = new SourcesLoadTimeResolution(project.numberOfResources, project.capacityOfResources, project.upperBoundMakespan);

	uint32_t scheduleLength = 0;
	for (uint32_t i = 0; i < project.numberOfActivities; ++i)	{
		uint32_t start = 0;
		uint32_t activityId = solution.orderOfActivities[forwardEvaluation == true ? i : project.numberOfActivities-i-1];
		for (uint32_t j = 0; j < project.numberOfPredecessors[activityId]; ++j)	{
			uint32_t predecessorActivityId = project.predecessorsOfActivity[activityId][j];
			start = max(timeValuesById[predecessorActivityId]+project.durationOfActivities[predecessorActivityId], start);
		}

		start = max(sourcesLoad->getEarliestStartTime(project.requiredResourcesOfActivities[activityId], start, project.durationOfActivities[activityId]), start);
		sourcesLoad->addActivity(start, start+project.durationOfActivities[activityId], project.requiredResourcesOfActivities[activityId]);
		scheduleLength = max(scheduleLength, start+project.durationOfActivities[activityId]);

		timeValuesById[activityId] = start;
	}

	delete sourcesLoad;
	return scheduleLength;
}

uint32_t ScheduleSolver::forwardScheduleEvaluation(const InstanceData& project, const InstanceSolution& solution, uint32_t *& startTimesById, EvaluationAlgorithm algorithm) {
	return evaluateOrder(project, solution, startTimesById, true, algorithm);
}

uint32_t ScheduleSolver::backwardScheduleEvaluation(const InstanceData& project, const InstanceSolution& solution, uint32_t *& startTimesById, EvaluationAlgorithm algorithm) {
	InstanceData copyProject = project;
	changeDirectionOfEdges(copyProject);
	uint32_t makespan = evaluateOrder(copyProject, solution, startTimesById, false, algorithm);
	// It computes the latest start time value for each activity.
	for (uint32_t id = 0; id < copyProject.numberOfActivities; ++id)
		startTimesById[id] = makespan-startTimesById[id]-copyProject.durationOfActivities[id];
	return makespan;
}

uint32_t ScheduleSolver::shakingDownEvaluation(const InstanceData& project, const InstanceSolution& solution, uint32_t *bestScheduleStartTimesById)	{
	uint32_t scheduleLength = 0;
	uint32_t bestScheduleLength = UINT32_MAX;
	uint32_t *currentOrder = new uint32_t[project.numberOfActivities];
	uint32_t *timeValuesById = new uint32_t[project.numberOfActivities];
	InstanceSolution copySolution = solution;
	copy(solution.orderOfActivities, solution.orderOfActivities+project.numberOfActivities, currentOrder);
	copySolution.orderOfActivities = currentOrder;

	while (true)	{
		// Forward schedule...
		scheduleLength = forwardScheduleEvaluation(project, copySolution, timeValuesById, TIME_RESOLUTION);
		if (scheduleLength < bestScheduleLength)	{
			bestScheduleLength = scheduleLength;
			if (bestScheduleStartTimesById != NULL)	{
				for (uint32_t id = 0; id < project.numberOfActivities; ++id)
					bestScheduleStartTimesById[id] = timeValuesById[id];
			}
		} else	{
			// No additional improvement can be found...
			break;
		}

		// It computes the earliest activities finish time.
		for (uint32_t id = 0; id < project.numberOfActivities; ++id)
			timeValuesById[id] += project.durationOfActivities[id];

		// Sort for backward phase..
		insertSort(project, copySolution, timeValuesById);

		// Backward phase.
		uint32_t scheduleLengthBackward = backwardScheduleEvaluation(project, copySolution, timeValuesById, TIME_RESOLUTION);
		int32_t diffCmax = scheduleLength-scheduleLengthBackward;

		// It computes the latest start time of activities.
		for (uint32_t id = 0; id < project.numberOfActivities; ++id)	{
			if (((int32_t) timeValuesById[id])+diffCmax > 0)
				timeValuesById[id] += diffCmax;
			else
				timeValuesById[id] = 0;
		}

		// Sort for forward phase..
		insertSort(project, copySolution, timeValuesById);
	}

	delete[] copySolution.orderOfActivities;
	delete[] timeValuesById;

	return bestScheduleLength;
}

uint32_t ScheduleSolver::computePrecedencePenalty(const InstanceData& project, const uint32_t * const& startTimesById)	{
	uint32_t penalty = 0;
	for (uint32_t activityId = 0; activityId < project.numberOfActivities; ++activityId)	{
		for (uint32_t j = 0; j < project.numberOfSuccessors[activityId]; ++j)	{
			uint32_t successorId = project.successorsOfActivity[activityId][j];	
			if (startTimesById[activityId]+project.durationOfActivities[activityId] > startTimesById[successorId])
				penalty += startTimesById[activityId]+project.durationOfActivities[activityId]-startTimesById[successorId];
		}
	}
	return penalty;
}

bool ScheduleSolver::checkSwapPrecedencePenalty(const InstanceData& project, const InstanceSolution& solution, uint32_t i, uint32_t j)	{
	if (i > j) swap(i,j);
	for (uint32_t k = i; k < j; ++k)	{
		if (project.matrixOfSuccessors[solution.orderOfActivities[k]][solution.orderOfActivities[j]] == 1)	{
			return false;
		}
	}
	for (uint32_t k = i+1; k <= j; ++k)	{
		if (project.matrixOfSuccessors[solution.orderOfActivities[i]][solution.orderOfActivities[k]] == 1)	{
			return false;
		}
	}
	return true;
}

void ScheduleSolver::convertStartTimesById2ActivitiesOrder(const InstanceData& project, InstanceSolution& solution, const uint32_t * const& startTimesById) {
	insertSort(project, solution, startTimesById);
}

void ScheduleSolver::insertSort(const InstanceData& project, InstanceSolution& solution, const uint32_t * const& timeValuesById) {
	for (int32_t i = 1; i < (int32_t) project.numberOfActivities; ++i)	{
		for (int32_t j = i; (j > 0) && ((timeValuesById[solution.orderOfActivities[j]] < timeValuesById[solution.orderOfActivities[j-1]]) == true); --j)	{
			swap(solution.orderOfActivities[j], solution.orderOfActivities[j-1]);
		}
	}
}

void ScheduleSolver::makeShift(uint32_t * const& order, const int32_t& diff, const uint32_t& baseIdx)	{
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

void ScheduleSolver::makeDiversification(const InstanceData& project, InstanceSolution& solution)	{
	uint32_t performedSwaps = 0;
	while (performedSwaps < ConfigureRCPSP::DIVERSIFICATION_SWAPS)	{
		uint32_t i = (rand() % (project.numberOfActivities-2)) + 1;
		uint32_t j = (rand() % (project.numberOfActivities-2)) + 1;

		if ((i != j) && (checkSwapPrecedencePenalty(project, solution, i, j) == true))	{
			swap(solution.orderOfActivities[i], solution.orderOfActivities[j]);
			++performedSwaps;
		}
	}
}

void ScheduleSolver::changeDirectionOfEdges(InstanceData& project)	{
	swap(project.numberOfSuccessors, project.numberOfPredecessors);
	swap(project.successorsOfActivity, project.predecessorsOfActivity);
	for (uint32_t i = 0; i < project.numberOfActivities; ++i)
		swap(project.allSuccessorsCache[i], project.allPredecessorsCache[i]);
}

vector<uint32_t>* ScheduleSolver::getAllRelatedActivities(uint32_t activityId, uint32_t *numberOfRelated, uint32_t **related, uint32_t numberOfActivities) {
	vector<uint32_t>* relatedActivities = new vector<uint32_t>();
	bool *activitiesSet = new bool[numberOfActivities];
	fill(activitiesSet, activitiesSet+numberOfActivities, false);

	for (uint32_t j = 0; j < numberOfRelated[activityId]; ++j)	{
		activitiesSet[related[activityId][j]] = true;
		vector<uint32_t>* indirectRelated = getAllRelatedActivities(related[activityId][j], numberOfRelated, related, numberOfActivities);
		for (vector<uint32_t>::const_iterator it = indirectRelated->begin(); it != indirectRelated->end(); ++it)
			activitiesSet[*it] = true;
		delete indirectRelated;
	}

	for (uint32_t id = 0; id < numberOfActivities; ++id)	{
		if (activitiesSet[id] == true)
			relatedActivities->push_back(id);
	}
	
	delete[] activitiesSet;
	return relatedActivities;
}

vector<uint32_t>* ScheduleSolver::getAllActivitySuccessors(const uint32_t& activityId, const InstanceData& project) {
	return getAllRelatedActivities(activityId, project.numberOfSuccessors, project.successorsOfActivity, project.numberOfActivities);
}

vector<uint32_t>* ScheduleSolver::getAllActivityPredecessors(const uint32_t& activityId, const InstanceData& project) {
	return getAllRelatedActivities(activityId, project.numberOfPredecessors, project.predecessorsOfActivity, project.numberOfActivities);
}

