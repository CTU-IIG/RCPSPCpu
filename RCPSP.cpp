#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "InputReader.h"
#include "ScheduleSolver.h"
#include "SourcesLoad.h"

using namespace std;

int main(int argc, char* argv[])	{

	vector<string> inputFiles;
	uint32_t maxIterSinceBest = 100;
	uint32_t numberOfIteration = 1000;

	for (int i = 1; i < argc; ++i)	{
		string arg = argv[i];
		if (arg == "--input-files" || arg == "-if")	{
			if (i+1 < argc)	{
				while (i+1 < argc && argv[i+1][0] != '-')	{
					inputFiles.push_back(argv[++i]);
				}
			} else {
				cerr<<"Option \"--input-files\" require parameter(s)!"<<endl;
				return 1;
			}
		}

		if (arg == "--number-of-iter" || arg == "-nit")	{
			if (i+1 < argc)	{
				string numStr = argv[++i];				
				istringstream istr(numStr,istringstream::in);
				istr>>numberOfIteration;
			} else {
				cerr<<"Option \"--number-of-iter\" require parameter!"<<endl;
				return 1;
			}
		}

		if (arg == "--max-iter-since-best" || arg == "-misb")	{
			if (i+1 < argc)	{
				string iterMax = argv[++i];				
				istringstream istr(iterMax,istringstream::in);
				istr>>maxIterSinceBest;
			} else {
				cerr<<"Option \"--max-iter-since-best\" require parameter!"<<endl;
				return 1;
			}
		}

		if (arg == "--help" || arg == "-h")	{
			cout<<"TODO: Print help..."<<endl;
			return 0;
		}
	}

/*
	uint32_t capacityOfResources[] = {5,5,5,5,5};

	SourcesLoad load(5,capacityOfResources);

	load.printCurrentState();
	cout<<endl;

	uint32_t reqRes[] = {4,0,2,2,3};
	load.addActivity(0,3,reqRes);

	load.printCurrentState();
	cout<<endl;

	reqRes = {1,0,3,0,0};
	load.addActivity(0,10,reqRes);

	load.printCurrentState();
	cout<<endl;

	reqRes = {0,0,0,3,0};
	load.addActivity(1,4,reqRes);

	load.printCurrentState();
	cout<<endl;

	reqRes = {4,1,0,0,0};
	load.addActivity(4,7,reqRes);

	load.printCurrentState();
	cout<<endl;

	reqRes = {0,1,2,4,0};
	load.addActivity(8,11,reqRes);

	load.printCurrentState();
	cout<<endl;

	reqRes = {0,1,0,0,0};
	load.addActivity(8,9,reqRes);

	load.printCurrentState();
	cout<<endl;

	reqRes = {0,4,4,0,4};
	load.addActivity(11,21,reqRes);

	load.printCurrentState();
	cout<<endl;

	reqRes = {5,0,0,1,1};
	load.addActivity(15,21,reqRes);

	load.printCurrentState();
	cout<<endl;

	reqRes = {2,3,4,0,0};
	load.addActivity(21,26,reqRes);

	load.printCurrentState();
	cout<<endl;

	reqRes = {2,0,0,4,4};
	load.addActivity(24,26,reqRes);

	load.printCurrentState();
*/

/*
	uint32_t capacityRes[] = {5};
	SourcesLoad load(1,capacityRes);

	uint32_t reqRes[] = {1};
	load.addActivity(0,3,reqRes);
	load.printCurrentState();

	reqRes = {1};
	load.addActivity(0,2,reqRes);
	load.printCurrentState();
	
	reqRes = {3};
	load.addActivity(1,4,reqRes);
	load.printCurrentState();
*/

/*
	uint32_t capacityRes[] = {5};
	SourcesLoad load(1,capacityRes);

	uint32_t reqRes[] = {2};
	load.addActivity(0,3,reqRes);
	load.printCurrentState();

	reqRes = {1};
	load.addActivity(1,4,reqRes);
	load.printCurrentState();
*/


	try {
		bool verbose = (inputFiles.size() == 1 ? true : false);
		for (vector<string>::const_iterator it = inputFiles.begin(); it != inputFiles.end(); ++it)	{
			string filename = *it;
			InputReader reader;
			reader.readFromFile(filename);

			ScheduleSolver solver(reader.getNumberOfResources(), reader.getCapacityOfResources(), reader.getNumberOfActivities(), reader.getActivitiesDuration(), 
					reader.getActivitiesSuccessors(), reader.getActivitiesNumberOfSuccessors(), reader.getActivitiesResources(), maxIterSinceBest);

		
			solver.solveSchedule(numberOfIteration);

			if (verbose == true)	{
				solver.printBestSchedule();
			}	else	{
				cout<<filename<<": "; 
				solver.printBestSchedule(false);
			}
		}
	} catch (exception& e)	{
		cerr<<e.what()<<endl;
		return 2;
	}

	return 0;
}
