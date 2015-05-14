/*
	This file is part of the RCPSPCpu program.

	RCPSPCpu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	RCPSPCpu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with RCPSPCpu. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef HLIDAC_PES_INPUT_READER_H
#define HLIDAC_PES_INPUT_READER_H

/*!
 * \file InputReader.h
 * \author Libor Bukata
 * \brief RCPSP instance reader.
 */

#include <iostream>
#include <string>
#include <stdint.h>

/*!
 * \class InputReader
 * \brief Read instance file and create required data structures.
 */
class InputReader {
	public:
		//! Default constructor. Initialise variables.
		InputReader() : numberOfActivities(0), totalNumberOfResources(0), activitiesDuration(NULL), activitiesRequiredResources(NULL),
						activitiesSuccessors(NULL), activitiesNumberOfSuccessors(NULL), capacityOfResources(NULL) { }

		/*!
		 * \param filename Name of the file.
		 * \exception runtime_error Invalid process of the file.
		 * \brief Project data are read from the file and instance data (structures) are filled.
		 */
		void readFromFile(const std::string& filename);
		/*!
		 * \param IN Input stream.
		 * \exception runtime_error Invalid format of the file.
		 * \brief Project data are read from the input stream.
		 */
		void readFromStream(std::istream& IN);

		//! Return number of the activities.
		uint32_t getNumberOfActivities() const { return numberOfActivities; }
		//! Return number of the resources.
		uint32_t getNumberOfResources() const { return totalNumberOfResources; }
		//! Return duration of the activities.
		uint32_t* getActivitiesDuration() const { return activitiesDuration; }
		//! Return requirements of the activities.
		uint32_t** getActivitiesResources() const { return activitiesRequiredResources; }
		//! Return successors of the activities.
		uint32_t** getActivitiesSuccessors() const { return activitiesSuccessors; }
		//! Return number of successors.
		uint32_t* getActivitiesNumberOfSuccessors() const { return activitiesNumberOfSuccessors; }
		//! Return capacities of the resources.
		uint32_t* getCapacityOfResources() const { return capacityOfResources; }

		/*!
		 * \param output Output stream.
		 * \brief Print read project data to output stream.
		 */
		void printInstance(std::ostream& output = std::cout)	const;

		//! Free project data structures.
		~InputReader() { freeInstanceData(); }

	private:

		//! Helper function which allocates some arrays.
		void allocateBaseArrays();
		/*!
		 * \param number String number.
		 * \return Integer number.
		 * \brief Convert number from the string format to integer.
		 */
		uint32_t strToNumber(const std::string& number)	const;
		/*!
		 * \param number Integer number.
		 * \return String number.
		 * \brief Convert integer number to string format.
		 */
		std::string numberToStr(const uint32_t& number) const;
		//! Free allocated memory.
		void freeInstanceData();

		//! Copy constructor is forbidden.
		InputReader(const InputReader&);
		//! Assignment operator is forbidden.
		InputReader& operator=(const InputReader&);

		/* INSTANCE DATA */

		//! Number of activities.
		uint32_t numberOfActivities;
		//! Number of resources.
		uint32_t totalNumberOfResources;
		//! Duration of the activities.
		uint32_t *activitiesDuration;
		//! Activities resources requirements.
		uint32_t **activitiesRequiredResources;
		//! Activities successors.
		uint32_t **activitiesSuccessors;
		//! Number of successors.
		uint32_t *activitiesNumberOfSuccessors;
		//! Capacities of the resources.
		uint32_t *capacityOfResources;
};

#endif

