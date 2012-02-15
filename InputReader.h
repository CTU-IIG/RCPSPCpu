#ifndef HLIDAC_PES_INPUT_READER_H
#define HLIDAC_PES_INPUT_READER_H

#include <iostream>
#include <string>
#include <stdint.h>

class InputReader {

	public:
		InputReader() : numberOfActivities(0), totalNumberOfResources(0), activitiesDuration(NULL), activitiesRequiredResources(NULL),
						activitiesSuccessors(NULL), activitiesNumberOfSuccessors(NULL), capacityOfResources(NULL) { }

		void readFromFile(const std::string& filename);
		void readFromStream(std::istream& IN);

		uint32_t getNumberOfActivities() const { return numberOfActivities; }
		uint32_t getNumberOfResources() const { return totalNumberOfResources; }
		uint32_t* getActivitiesDuration() const { return activitiesDuration; }
		uint32_t** getActivitiesResources() const { return activitiesRequiredResources; }
		uint32_t** getActivitiesSuccessors() const { return activitiesSuccessors; }
		uint32_t* getActivitiesNumberOfSuccessors() const { return activitiesNumberOfSuccessors; }
		uint32_t* getCapacityOfResources() const { return capacityOfResources; }

		void printInstance(std::ostream& OUT = std::cout)	const;

		~InputReader() { freeInstanceData(); }

	private:

		void allocateBaseArrays();
		uint32_t strToNumber(const std::string& number)	const;
		std::string numberToStr(const uint32_t& number) const;
		void freeInstanceData();

		InputReader(const InputReader&);
		InputReader& operator=(const InputReader&);

		/* INSTANCE DATA */
		uint32_t numberOfActivities;
		uint32_t totalNumberOfResources;
		uint32_t *activitiesDuration;
		uint32_t **activitiesRequiredResources;
		uint32_t **activitiesSuccessors;
		uint32_t *activitiesNumberOfSuccessors;
		uint32_t *capacityOfResources;
};

#endif
