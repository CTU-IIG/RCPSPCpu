#include <algorithm>
#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include "InputReader.h"

using namespace std;

void InputReader::readFromFile(const string& filename) {
	ifstream IN(filename.c_str());	
	if (!IN)
		throw runtime_error("InputReader::readFromFIle: Cannot open input file!");

	readFromStream(IN);
	IN.close();
}

void InputReader::readFromStream(istream& IN) {

	freeInstanceData();

	uint32_t shred;
	string readedLine;

	if (getline(IN, readedLine))	{
		IN.seekg (0, ios::beg);
		
		if (!readedLine.empty() && readedLine[0] == '*')	{
			/* PROGEN-SFX FORMAT */
			string::const_iterator sit;
			string searchPattern1 = "- renewable";
			string searchPattern2 = "MPM-Time";
			string searchPattern3 = "#successors";

			// Read project basic parameters.
			while (getline(IN,readedLine))	{
				if ((sit = search(readedLine.begin(), readedLine.end(), searchPattern1.begin(), searchPattern1.end())) != readedLine.end())	{
					string parsedNumber;
					for (string::const_iterator it = sit; it != readedLine.end(); ++it)	{
						if (isdigit(*it) > 0)
							parsedNumber.push_back(*it);			
					}
					totalNumberOfResources = strToNumber(parsedNumber);
				}

				if ((sit = search(readedLine.begin(), readedLine.end(), searchPattern2.begin(), searchPattern2.end())) != readedLine.end())	{
					IN>>shred>>numberOfActivities;
					numberOfActivities += 2;
				}

				if ((sit = search(readedLine.begin(), readedLine.end(), searchPattern3.begin(), searchPattern3.end())) != readedLine.end())	{
					break;
				}
			}

			activitiesDuration = new uint32_t[numberOfActivities];
			activitiesRequiredResources = new uint32_t*[numberOfActivities];
			activitiesSuccessors = new uint32_t*[numberOfActivities];
			activitiesNumberOfSuccessors = new uint32_t[numberOfActivities];
			capacityOfResources = new uint32_t[totalNumberOfResources];

			// Read succesors.
			for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
				size_t numberOfSuccessors;
				IN>>shred>>shred>>numberOfSuccessors;

				activitiesNumberOfSuccessors[activityId] = numberOfSuccessors;
				activitiesSuccessors[activityId] = new uint32_t[numberOfSuccessors];

				uint32_t successor;
				for (uint32_t i = 0; i < numberOfSuccessors; ++i)	{
					IN>>successor;
					activitiesSuccessors[activityId][i] = successor-1;
				}
			}

			for (uint32_t i = 0; i < 5; ++i)
				getline(IN,readedLine);

			// Read resource requirements.
			for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
				uint32_t activityDuration;
				IN>>shred>>shred>>activityDuration;

				activitiesDuration[activityId] = activityDuration;
				activitiesRequiredResources[activityId] = new uint32_t[totalNumberOfResources];

				for (uint32_t resourceId = 0; resourceId < totalNumberOfResources; ++resourceId)	{
					uint32_t unitsReq; IN>>unitsReq;
					activitiesRequiredResources[activityId][resourceId] = unitsReq;
				}
			}

			for (uint32_t i = 0; i < 4; ++i)
				getline(IN,readedLine);

			// Read capacity of resources.
			for (uint32_t resourceId = 0; resourceId < totalNumberOfResources; ++resourceId)	{
				uint32_t resourceCapacity; IN>>resourceCapacity;
				capacityOfResources[resourceId] = resourceCapacity;
			}
		} else if (!readedLine.empty())	{
			/* PROGEN/MAX 1.0 FORMAT */
			IN>>numberOfActivities>>totalNumberOfResources>>shred>>shred;
			numberOfActivities += 2;

			activitiesDuration = new uint32_t[numberOfActivities];
			activitiesRequiredResources = new uint32_t*[numberOfActivities];
			activitiesSuccessors = new uint32_t*[numberOfActivities];
			activitiesNumberOfSuccessors = new uint32_t[numberOfActivities];
			capacityOfResources = new uint32_t[totalNumberOfResources];

			// Read activity successors.
			for (uint32_t actId = 0; actId < numberOfActivities; ++actId)	{
				uint32_t successor = 0;
				size_t numberOfSuccessors = 0;
				IN>>shred>>shred>>numberOfSuccessors;
				activitiesNumberOfSuccessors[actId] = numberOfSuccessors;
				activitiesSuccessors[actId] = new uint32_t[numberOfSuccessors];
				for (uint32_t k = 0; k < numberOfSuccessors; ++k)	{
					IN>>successor;
					activitiesSuccessors[actId][k] = successor;
				}
			}

			// Read activities resources requirement.
			for (uint32_t actId = 0; actId < numberOfActivities; ++actId)	{
				uint32_t resourceReq = 0;
				uint32_t activityDuration = 0;
				IN>>shred>>shred>>activityDuration;
				activitiesDuration[actId] = activityDuration;
				activitiesRequiredResources[actId] = new uint32_t[totalNumberOfResources];
				for (uint32_t r = 0; r < totalNumberOfResources; ++r)	{
					IN>>resourceReq;	
					activitiesRequiredResources[actId][r] = resourceReq;
				}
			}	
			
			// Read resources capacity.
			for (uint32_t r = 0; r < totalNumberOfResources; ++r)	{
				uint32_t resourceCapacity = 0;
				IN>>resourceCapacity;
				capacityOfResources[r] = resourceCapacity;
			}
		}
	}
}

void InputReader::printInstance(ostream& OUT)	const	{
	for (uint32_t actId = 0; actId < numberOfActivities; ++actId)	{
		OUT<<string(50,'+')<<endl;
		OUT<<"Activity number: "<<actId+1<<endl;
		OUT<<"Duration of activity: "<<activitiesDuration[actId]<<endl;	
		OUT<<"Required sources (Resource ID : Units required):"<<endl;
		for (uint32_t *resPtr = activitiesRequiredResources[actId]; resPtr < activitiesRequiredResources[actId]+totalNumberOfResources; ++resPtr)	{
			OUT<<"\t("<<((resPtr-activitiesRequiredResources[actId])+1)<<" : "<<*resPtr<<")"<<endl;
		}
		OUT<<"Successors of activity:";
		for (uint32_t *sucPtr = activitiesSuccessors[actId]; sucPtr < activitiesSuccessors[actId]+activitiesNumberOfSuccessors[actId]; ++sucPtr)	{
			OUT<<" "<<*sucPtr+1;
		}
		OUT<<endl;
		OUT<<string(50,'-')<<endl;
	}
	OUT<<"Max capacity of resources:";
	for (uint32_t *capPtr = capacityOfResources; capPtr < capacityOfResources+totalNumberOfResources; ++capPtr)
		OUT<<" "<<*capPtr;
	OUT<<endl;
}

uint32_t InputReader::strToNumber(const string& number)	const {
	istringstream istr(number,istringstream::in);
	uint32_t ret; istr>>ret;
	return ret;
}

void InputReader::freeInstanceData()	{
	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		delete[] activitiesSuccessors[activityId];
		delete[] activitiesRequiredResources[activityId];
	}

	delete[] activitiesSuccessors;	
	delete[] activitiesRequiredResources;
	delete[] activitiesDuration;
	delete[] activitiesNumberOfSuccessors;
	delete[] capacityOfResources;
}

