#include <iostream>
#include <typeinfo>
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "ConfigureRCPSP.h"
#include "InputReader.h"
#include "ScheduleSolver.h"

using namespace std;

int rcpsp(int argc, char* argv[]);

int main(int argc, char* argv[])	{
	return rcpsp(argc,argv);
}

template <typename T>
T optionHelper(const string& option, int& i, const int& argc, char* argv[])	{
	if (i+1 < argc)	{
		T value = T();
		string numStr = argv[++i];
		istringstream istr(numStr, istringstream::in);
		if (!(istr>>value))
			throw invalid_argument("Cannot read argument! (option \""+option+"\")");
		if (value < 0 || (!numStr.empty() && numStr[0] == '-'))
			throw range_error("Argument value cannot be negative!");
		if (typeid(value) == typeid(double) && (value < 0 || value > 1))
			throw range_error("Invalid range of double! (correct range 0-1)");
		return value;
	} else {
		throw invalid_argument("Option \""+option+"\" require argument!");
	}
}

int rcpsp(int argc, char* argv[])	{

	vector<string> inputFiles;

	for (int i = 1; i < argc; ++i)	{

		string arg = argv[i];

		if (arg == "--input-files" || arg == "-if")	{
			if (i+1 < argc)	{
				while (i+1 < argc && argv[i+1][0] != '-')	{
					inputFiles.push_back(argv[++i]);
				}
			} else {
				cerr<<"Option \"--input-files\" require argument(s)!"<<endl;
				return 1;
			}
		}
		
		if (arg == "--simple-tabu-list" || arg == "-stl")
			ConfigureRCPSP::TABU_LIST_TYPE = SIMPLE_TABU;

		if (arg == "--advance-tabu-list" || arg == "-atl")
			ConfigureRCPSP::TABU_LIST_TYPE = ADVANCE_TABU;

		try {
			if (arg == "--number-of-iterations" || arg == "-noi")
				ConfigureRCPSP::NUMBER_OF_ITERATIONS = optionHelper<uint32_t>("--number-of-iterations", i, argc, argv);
			if (arg == "--max-iter-since-best" || arg == "-misb")
				ConfigureRCPSP::MAXIMAL_NUMBER_OF_ITERATIONS_SINCE_BEST = optionHelper<uint32_t>("--max-iter-since-best", i, argc, argv);
			if (arg == "--tabu-list-size" || arg == "-tls")
				ConfigureRCPSP::SIMPLE_TABU_LIST_SIZE = optionHelper<uint32_t>("--tabu-list-size", i, argc, argv);
			if (arg == "--randomize-erase-amount" || arg == "-rea")
				ConfigureRCPSP::ADVANCE_TABU_RANDOMIZE_ERASE_AMOUNT = optionHelper<double>("--randomize-erase-amount", i, argc, argv);
			if (arg == "--swap-live-factor" || arg == "-swlf")
				ConfigureRCPSP::ADVANCE_TABU_SWAP_LIVE = optionHelper<uint32_t>("--swap-live-factor", i, argc, argv);
			if (arg == "--shift-live-factor" || arg == "-shlf")
				ConfigureRCPSP::ADVANCE_TABU_SHIFT_LIVE = optionHelper<uint32_t>("--shift-live-factor", i, argc, argv);
			if (arg == "--swap-range" || arg == "-swr")
				ConfigureRCPSP::SWAP_RANGE = optionHelper<uint32_t>("--swap-range", i, argc, argv);
			if (arg == "--shift-range" || arg == "-shr")
				ConfigureRCPSP::SHIFT_RANGE = optionHelper<uint32_t>("--shift-range", i, argc, argv);
			if (arg == "--diversification-swaps" || arg == "-ds")
				ConfigureRCPSP::DIVERSIFICATION_SWAPS = optionHelper<uint32_t>("--diversification-swaps", i, argc, argv);
		} catch (exception& e)	{
			cerr<<e.what()<<endl;
			return 1;
		}

		if (arg == "--help" || arg == "-h")	{
			cout<<"RCPSP schedule solver."<<endl<<endl;
			cout<<"Usage:"<<endl;
			cout<<"\t"<<argv[0]<<" [options+parameters] --input-files file1 file2 ..."<<endl;
			cout<<"Options:"<<endl;
			cout<<"\t"<<"--input-files ARG, -if ARG, ARG=\"FILE1 FILE2 ... FILEX\""<<endl;
			cout<<"\t\t"<<"Instances data. Input files are delimited by space."<<endl;
			cout<<"\t"<<"--simple-tabu-list, -stl"<<endl;
			cout<<"\t\t"<<"Simple version of tabu list is used."<<endl;
			cout<<"\t"<<"--advance-tabu-list, -atl"<<endl;
			cout<<"\t\t"<<"More sophistic version of tabu list is used."<<endl;
			cout<<"\t"<<"--number-of-iterations ARG, -noi ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Number of iterations after which search process will be stopped."<<endl;
			cout<<"\t"<<"--max-iter-since-best ARG, -misb ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Maximal number of iterations without improving solution after which diversification will be called."<<endl;
			cout<<"\t"<<"--tabu-list-size ARG, -tls ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Size of simple tabu list. Ignored for advance tabu list."<<endl;
			cout<<"\t"<<"--randomize-erase-amount ARG, -rea ARG, ARG=POSITIVE_DOUBLE"<<endl;
			cout<<"\t\t"<<"Relative amount (0-1) of elements that will be erased from advance tabu list if diversification will be called."<<endl;
			cout<<"\t"<<"--swap-live-factor ARG, -swlf ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Set how long will be added swap moves at advance tabu list."<<endl;
			cout<<"\t"<<"--shift-live-factor ARG, -shlf ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Set how long will be added shift moves at advance tabu list."<<endl;
			cout<<"\t"<<"--swap-range ARG, -swr ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Maximal distance between swapped activities."<<endl;
			cout<<"\t"<<"--shift-range ARG, -shr ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Maximal number of activities which can moved activity go through."<<endl;
			cout<<"\t"<<"--diversification-swaps ARG, -ds ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"How many swaps should be performed when diversification is callled."<<endl<<endl;
			cout<<"Default values can be modified at \"DefaultConfigureRCPSP.h\" file."<<endl;
			return 0;
		}
	}

	try {
		bool verbose = (inputFiles.size() == 1 ? true : false);
		for (vector<string>::const_iterator it = inputFiles.begin(); it != inputFiles.end(); ++it)	{
			// Filename of instance.
			string filename = *it;
			InputReader reader;
			// Read instance data.
			reader.readFromFile(filename);
			// Init schedule solver.
			ScheduleSolver solver(reader);
			// Solve readed instance.	
			solver.solveSchedule(ConfigureRCPSP::NUMBER_OF_ITERATIONS);
			// Print results.
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

