#ifndef HLIDAC_PES_SOURCES_LOAD_H
#define HLIDAC_PES_SOURCES_LOAD_H

#define DEBUG_SOURCES 0

#include <iostream>
#include <stdint.h>

#if DEBUG_SOURCES == 1
#include <map>
#include <vector>
#endif

class SourcesLoad {
	public:
		SourcesLoad(const uint32_t& numRes, const uint32_t * const& capRes);

		uint32_t getEarliestStartTime(const uint32_t * const& activityResourceRequirement) const;
		void addActivity(const uint32_t& activityStart, const uint32_t& activityStop, const uint32_t * const& activityRequirement);
		void printCurrentState(std::ostream& OUT = std::cout) const;

		~SourcesLoad();

	private:

		/* COPY OF OBJECT IS FORBIDDEN */

		SourcesLoad(const SourcesLoad&);
		SourcesLoad& operator=(const SourcesLoad&);


		// Total number of resources.
		const uint32_t numberOfResources;
		// Capacity of resources.
		const uint32_t * const capacityOfResources;
		// Current state of resources.
		uint32_t **resourcesLoad;
		// Helper arrays.
		uint32_t *startValues, *reqItems;

		#if DEBUG_SOURCES == 1
		std::map<uint32_t,int32_t*> peaks;
		#endif
};

#endif

