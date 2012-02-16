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
		throw runtime_error("InputReader::readFromFile: Cannot open input file \""+filename+"\"!");

	readFromStream(IN);
	IN.close();
}

void InputReader::readFromStream(istream& IN) {

	freeInstanceData();

	uint32_t shred;
	string readLine;

	if (getline(IN, readLine))	{
		IN.seekg (0, ios::beg);
		
		if (!readLine.empty() && readLine[0] == '*')	{
			/* PROGEN-SFX FORMAT */
			string::const_iterator sit;
			string searchPattern1 = "- renewable";
			string searchPattern2 = "MPM-Time";
			string searchPattern3 = "#successors";

			// Read project basic parameters.
			while (getline(IN,readLine))	{
				if ((sit = search(readLine.begin(), readLine.end(), searchPattern1.begin(), searchPattern1.end())) != readLine.end())	{
					string parsedNumber;
					for (string::const_iterator it = sit; it != readLine.end(); ++it)	{
						if (isdigit(*it) > 0)
							parsedNumber.push_back(*it);			
					}
					totalNumberOfResources = strToNumber(parsedNumber);
					if (parsedNumber.empty() || totalNumberOfResources == 0)
						throw runtime_error("InputReader::readFromStream: Cannot read number of resources!");
				}

				if ((sit = search(readLine.begin(), readLine.end(), searchPattern2.begin(), searchPattern2.end())) != readLine.end())	{
					if (!(IN>>shred>>numberOfActivities))
						throw runtime_error("InputReader::readFromStream: Cannot read number of activities!");
					if (numberOfActivities == 0)
						throw runtime_error("InputReader::readFromStream: Number of activities is number greater than zero!");
					numberOfActivities += 2;
				}

				if ((sit = search(readLine.begin(), readLine.end(), searchPattern3.begin(), searchPattern3.end())) != readLine.end())	{
					break;
				}
			}

			if (numberOfActivities == 0 || totalNumberOfResources == 0)
				throw runtime_error("InputReader::readFromStream: Invalid format of input file!");

			allocateBaseArrays();

			// Read succesors.
			for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
				uint32_t numberOfSuccessors, testId;
				if (!(IN>>testId>>shred>>numberOfSuccessors))
					throw runtime_error("InputReader::readFromStream: Cannot read successors of activity "+numberToStr(activityId+1)+"!");
				if (activityId+1 != testId)
					throw runtime_error("InputReader::readFromStream: Probably inconsistence of instance file!\nCheck activity ID failed.");

				activitiesNumberOfSuccessors[activityId] = numberOfSuccessors;
				activitiesSuccessors[activityId] = new uint32_t[numberOfSuccessors];

				uint32_t successor;
				for (uint32_t i = 0; i < numberOfSuccessors; ++i)	{
					if (!(IN>>successor))
						throw runtime_error("InputReader::readFromStream: Cannot read next ("+numberToStr(i+1)+") successor of activity "+numberToStr(activityId+1)+"!");
					if (successor > numberOfActivities)
						throw runtime_error("InputReader::readFromStream: Invalid successor ID of activity "+numberToStr(activityId+1)+"!");
					activitiesSuccessors[activityId][i] = successor-1;
				}
			}

			for (uint32_t i = 0; i < 5; ++i)
				getline(IN,readLine);

			// Read resource requirements.
			for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
				uint32_t activityDuration, testId;
				if (!(IN>>testId>>shred>>activityDuration))
					throw runtime_error("InputReader::readFromStream: Invalid read of activity requirement!");
				if (activityId+1 != testId)
					throw runtime_error("InputReader::readFromStream: Probably inconsistence of instance file!\nCheck activity ID failed.");

				activitiesDuration[activityId] = activityDuration;

				uint32_t unitsReq; 
				for (uint32_t resourceId = 0; resourceId < totalNumberOfResources; ++resourceId)	{
					if (!(IN>>unitsReq))
						throw runtime_error("InputReader::readFromStream: Cannot read next activity requirement ("+numberToStr(resourceId+1)+") of activity "+numberToStr(activityId+1)+"!");
					activitiesRequiredResources[activityId][resourceId] = unitsReq;
				}
			}

			for (uint32_t i = 0; i < 4; ++i)
				getline(IN,readLine);

			// Read capacity of resources.
			for (uint32_t resourceId = 0; resourceId < totalNumberOfResources; ++resourceId)	{
				uint32_t resourceCapacity;
				if (!(IN>>resourceCapacity))
					throw runtime_error("InputReader::readFromStream: Invalid read of resource capacity!\nResource ID is "+numberToStr(resourceId+1)+".");
				capacityOfResources[resourceId] = resourceCapacity;
			}
		} else if (!readLine.empty())	{
			/* PROGEN/MAX 1.0 FORMAT */
			if (!(IN>>numberOfActivities>>totalNumberOfResources>>shred>>shred))
				throw runtime_error("InputReader::readFromStream: Cannot read number of activities and number of resource!\nCheck file format.");
			if (numberOfActivities == 0 || totalNumberOfResources == 0)
				throw runtime_error("InputReader::readFromStream: Invalid value of number of activities or number of resources!");
			numberOfActivities += 2;

			allocateBaseArrays();

			// Read activity successors.
			for (uint32_t actId = 0; actId < numberOfActivities; ++actId)	{
				uint32_t successor = 0, numberOfSuccessors = 0, testId = 0;
				if (!(IN>>testId>>shred>>numberOfSuccessors))
					throw runtime_error("InputReader::readFromStream: Cannot read number of successors of activity "+numberToStr(actId)+"!");
				if (actId != testId)
					throw runtime_error("InputReader::readFromStream: Probably inconsistence of instance file!\nCheck activity ID failed.");

				activitiesNumberOfSuccessors[actId] = numberOfSuccessors;
				activitiesSuccessors[actId] = new uint32_t[numberOfSuccessors];
				for (uint32_t k = 0; k < numberOfSuccessors; ++k)	{
					if (!(IN>>successor))
						throw runtime_error("InputReader::readFromStream: Cannot read next ("+numberToStr(k+1)+") successor of activity "+numberToStr(actId)+"!");
					if (successor >= numberOfActivities)
						throw runtime_error("InputReader::readFromStream: Invalid successor ID of activity "+numberToStr(actId)+"!");
					activitiesSuccessors[actId][k] = successor;
				}
			}

			// Read activities resources requirement.
			for (uint32_t actId = 0; actId < numberOfActivities; ++actId)	{
				uint32_t resourceReq = 0, activityDuration = 0, testId = 0;
				if (!(IN>>testId>>shred>>activityDuration))
					throw runtime_error("InputReader::readFromStream: Invalid read of activity requirement!");
				if (actId != testId)
					throw runtime_error("InputReader::readFromStream: Probably inconsistence of instance file!\nCheck activity ID failed.");

				activitiesDuration[actId] = activityDuration;

				for (uint32_t r = 0; r < totalNumberOfResources; ++r)	{
					if (!(IN>>resourceReq))
						throw runtime_error("InputReader::readFromStream: Cannot read next activity requirement ("+numberToStr(r)+") of activity "+numberToStr(actId)+"!");
					activitiesRequiredResources[actId][r] = resourceReq;
				}
			}	
			
			// Read resources capacity.
			for (uint32_t r = 0; r < totalNumberOfResources; ++r)	{
				uint32_t resourceCapacity = 0;
				if (!(IN>>resourceCapacity))
					throw runtime_error("InputReader::readFromStream: Invalid read of resource capacity!\nResource ID is "+numberToStr(r)+".");
				capacityOfResources[r] = resourceCapacity;
			}
		} else {
			throw runtime_error("InputReader::readFromStream: Empty instance file??");
		}
	}

	// Check if resources are sufficient for activities.
	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		for (uint32_t resourceId = 0; resourceId < totalNumberOfResources; ++resourceId)	{
			if (activitiesRequiredResources[activityId][resourceId] > capacityOfResources[resourceId])
				throw runtime_error("InputReader::readFromStream: Suggested resources are insufficient for activities requirement!");
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

void InputReader::allocateBaseArrays()	{
	activitiesDuration = new uint32_t[numberOfActivities];
	activitiesSuccessors = new uint32_t*[numberOfActivities];
	activitiesNumberOfSuccessors = new uint32_t[numberOfActivities];
	capacityOfResources = new uint32_t[totalNumberOfResources];
	activitiesRequiredResources = new uint32_t*[numberOfActivities];
	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)
		activitiesRequiredResources[activityId] = new uint32_t[totalNumberOfResources];
}

uint32_t InputReader::strToNumber(const string& number)	const {
	istringstream istr(number, istringstream::in);
	uint32_t ret = 0; istr>>ret;
	return ret;
}

string InputReader::numberToStr(const uint32_t& number) const {
	stringstream ss;
	ss<<number;
	return ss.str();
}

void InputReader::freeInstanceData()	{
	for (uint32_t activityId = 0; activityId < numberOfActivities; ++activityId)	{
		if (activitiesSuccessors != NULL)
			delete[] activitiesSuccessors[activityId];
		if (activitiesRequiredResources != NULL)
			delete[] activitiesRequiredResources[activityId];
	}

	delete[] activitiesSuccessors;	
	delete[] activitiesRequiredResources;
	delete[] activitiesDuration;
	delete[] activitiesNumberOfSuccessors;
	delete[] capacityOfResources;
}

