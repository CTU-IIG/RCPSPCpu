CPP = g++

PROGRAM = RCPSP

INST_PATH = /usr/local/bin/

OBJ = InputReader.o RCPSP.o SimpleTabuList.o ScheduleSolver.o AdvancedTabuList.o ConfigureRCPSP.o SourcesLoadTimeResolution.o SourcesLoadCapacityResolution.o
INC = InputReader.h SimpleTabuList.h ScheduleSolver.h DefaultConfigureRCPSP.h ConfigureRCPSP.h AdvancedTabuList.h TabuList.h ConstantsRCPSP.h SourcesLoad.h SourcesLoadTimeResolution.h SourcesLoadCapacityResolution.h
SRC = RCPSP.cpp InputReader.cpp SimpleTabuList.cpp ScheduleSolver.cpp AdvancedTabuList.cpp ConfigureRCPSP.cpp SourcesLoadTimeResolution.cpp SourcesLoadCapacityResolution.cpp

# If yout want to analyse performance then switch -pg (gprof) should be used. Static linkage of standard C++ library (-static-libstdc++).
ifdef DEBUG
GCC_OPTIONS = -O0 -g
LIBS = -std=c++0x
else
# The GCC version 4.7 supports the '-std=c++11' flag.
GCC_OPTIONS = -pedantic -Wall -march=native -O3 -pipe -funsafe-math-optimizations -fopenmp -flto -fwhole-program
LIBS = -std=c++0x
endif

.PHONY: build
.PHONY: install
.PHONY: uninstall
.PHONY: clean
.PHONY: distrib

# Default option for make.
build: $(PROGRAM)

# Generate documentation.
doc: 
	doxygen Documentation/doxyfilelatex; \
	doxygen Documentation/doxyfilehtml

# Compile program.
$(PROGRAM): $(OBJ)
	$(CPP) $(LIBS) $(GCC_OPTIONS) -o $(PROGRAM) $(OBJ)


# Compile .cpp files to objects.
%.o: %.cpp
	$(CPP) $(LIBS) $(GCC_OPTIONS) -c -o $@ $<

# Install program.
install: build
	cp $(PROGRAM) $(INST_PATH)

# Clean temporary files and remove program executable file.
clean:
	rm -f *.o $(PROGRAM)

# Uninstall program.
uninstall:
	rm -f $(INST_PATH)$(PROGRAM)

# Create tarball from the project files.
distrib:
	tar -c $(SRC) $(INC) Makefile > $(PROGRAM).tar; \
    bzip2 $(PROGRAM).tar

# Dependencies among header files and object files.
${OBJ}: ${INC}

