/*!
 * \file RCPSP.cpp
 * \author Libor Bukata
 * \brief RCPSP - Resource Constrained Project Scheduling Problem
 */

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

// Forward declaration entry point of the program.
int rcpsp(int argc, char* argv[]);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Trick how can be entry point of the program ,,renamed'' from main to rcpsp. (for purposes of documentation - Doxygen)
int main(int argc, char* argv[])	{
	return rcpsp(argc,argv);
}
#endif

/*!
 * \param option Command line switch.
 * \param i Current index at argv two-dimensional array. Can be modified.
 * \param argc Number of arguments (program name + switches + parameters) that were given through command line.
 * \param argv Command line arguments.
 * \tparam T Integer or double.
 * \exception invalid_argument Parameter cannot be read.
 * \exception range_error Invalid value of read parameter.
 * \return Parameter of the switch.
 * \brief Helper function for command line processing.
 */
template <typename T>
T optionHelper(const string& option, int& i, const int& argc, char* argv[])	{
	if (i+1 < argc)	{
		T value = T();
		string numStr = argv[++i];
		istringstream istr(numStr, istringstream::in);
		if (!(istr>>value))
			throw invalid_argument("Cannot read parameter! (option \""+option+"\")");
		if (value < 0 || (!numStr.empty() && numStr[0] == '-'))
			throw range_error("Parameter value cannot be negative!");
		if (typeid(value) == typeid(double) && (value < 0 || value > 1))
			throw range_error("Invalid range of double! (correct range 0-1)");
		return value;
	} else {
		throw invalid_argument("Option \""+option+"\" require argument!");
	}
}

/*!
 * Entry point for RCPSP solver. Command line arguments are processed, input instances are
 * read and solved. Results are printed to console (can be easily redirected to file). 
 * Verbose mode is turned on if and only if one input file is read.
 * \param argc Number of command line arguments.
 * \param argv Command line arguments.
 * \return Zero if success else error code (positive number).
 * \brief Process arguments, read instances, solve instances and print results.
 */
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
				cerr<<"Option \"--input-files\" require parameter(s)!"<<endl;
				return 1;
			}
		}
		
		if (arg == "--simple-tabu-list" || arg == "-stl")
			ConfigureRCPSP::TABU_LIST_TYPE = SIMPLE_TABU;

		if (arg == "--advanced-tabu-list" || arg == "-atl")
			ConfigureRCPSP::TABU_LIST_TYPE = ADVANCED_TABU;

		if (arg == "--write-makespan-graph" || arg == "-wmg")
			ConfigureRCPSP::WRITE_GRAPH = true;

		if (arg == "--write-result-file" || arg == "-wrf")
			ConfigureRCPSP::WRITE_RESULT_FILE = true;

		try {
			if (arg == "--number-of-iterations" || arg == "-noi")
				ConfigureRCPSP::NUMBER_OF_ITERATIONS = optionHelper<uint32_t>("--number-of-iterations", i, argc, argv);
			if (arg == "--max-iter-since-best" || arg == "-misb")
				ConfigureRCPSP::MAXIMAL_NUMBER_OF_ITERATIONS_SINCE_BEST = optionHelper<uint32_t>("--max-iter-since-best", i, argc, argv);
			if (arg == "--tabu-list-size" || arg == "-tls")
				ConfigureRCPSP::SIMPLE_TABU_LIST_SIZE = optionHelper<uint32_t>("--tabu-list-size", i, argc, argv);
			if (arg == "--randomize-erase-amount" || arg == "-rea")
				ConfigureRCPSP::ADVANCED_TABU_RANDOMIZE_ERASE_AMOUNT = optionHelper<double>("--randomize-erase-amount", i, argc, argv);
			if (arg == "--swap-life-factor" || arg == "-swlf")
				ConfigureRCPSP::ADVANCED_TABU_SWAP_LIFE = optionHelper<uint32_t>("--swap-life-factor", i, argc, argv);
			if (arg == "--shift-life-factor" || arg == "-shlf")
				ConfigureRCPSP::ADVANCED_TABU_SHIFT_LIFE = optionHelper<uint32_t>("--shift-life-factor", i, argc, argv);
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
			cout<<"\t\t"<<"The simple version of the tabu list is used."<<endl;
			cout<<"\t"<<"--advanced-tabu-list, -atl"<<endl;
			cout<<"\t\t"<<"More sophisticated version of the tabu list is used."<<endl;
			cout<<"\t"<<"--number-of-iterations ARG, -noi ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Number of iterations after which the search process will be stopped."<<endl;
			cout<<"\t"<<"--max-iter-since-best ARG, -misb ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Maximal number of iterations without improving solution after which diversification is called."<<endl;
			cout<<"\t"<<"--tabu-list-size ARG, -tls ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Size of the simple tabu list. Ignored for the advanced tabu list."<<endl;
			cout<<"\t"<<"--randomize-erase-amount ARG, -rea ARG, ARG=POSITIVE_DOUBLE"<<endl;
			cout<<"\t\t"<<"Relative amount (0-1) of elements will be erased from the advanced tabu list if diversification will be called."<<endl;
			cout<<"\t"<<"--swap-life-factor ARG, -swlf ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Lifetime of the newly added swap move to the advanced tabu list."<<endl;
			cout<<"\t"<<"--shift-life-factor ARG, -shlf ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Lifetime of the newly added shift move to the advanced tabu list."<<endl;
			cout<<"\t"<<"--swap-range ARG, -swr ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Maximal distance between swapped activities."<<endl;
			cout<<"\t"<<"--shift-range ARG, -shr ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Maximal number of activities which moved activity can go through."<<endl;
			cout<<"\t"<<"--diversification-swaps ARG, -ds ARG, ARG=POSITIVE_INTEGER"<<endl;
			cout<<"\t\t"<<"Number of performed swaps for every diversification."<<endl;
			cout<<"\t"<<"--write-makespan-graph, -wmg"<<endl;
			cout<<"\t\t"<<"If you want to write makespan criterion graph (independent variable is number of iterations)"<<endl;
			cout<<"\t\t"<<"then use this switch to enable csv file generation."<<endl;
			cout<<"\t"<<"--write-result-file, -wrf"<<endl;
			cout<<"\t\t"<<"Add this option if you want to write a file with the best schedule."<<endl;
			cout<<"\t\t"<<"This file is binary."<<endl<<endl;
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
			// Solve read instance.	
			string graphFilename = "", resultFilename = "";
			if (ConfigureRCPSP::WRITE_GRAPH == true || ConfigureRCPSP::WRITE_RESULT_FILE == true)	{
				int32_t i;
				for (i = filename.size()-1; i >= 0; --i)	{
					if (filename[i] == '.')
						break;
				}
				if (i > 0)	{
					if (ConfigureRCPSP::WRITE_GRAPH == true)
						graphFilename = string(filename, 0, i) + ".csv";
					if (ConfigureRCPSP::WRITE_RESULT_FILE == true)
						resultFilename = string(filename, 0, i) + ".res";
				}
			}
			solver.solveSchedule(ConfigureRCPSP::NUMBER_OF_ITERATIONS, graphFilename);
			// Print results.
			if (verbose == true)	{
				solver.printBestSchedule();
			}	else	{
				cout<<filename<<": "; 
				solver.printBestSchedule(false);
			}
			// Write the best schedule to file.
			if (!resultFilename.empty())
				solver.writeBestScheduleToFile(resultFilename);
		}
	} catch (exception& e)	{
		cerr<<e.what()<<endl;
		return 2;
	}

	return 0;
}

