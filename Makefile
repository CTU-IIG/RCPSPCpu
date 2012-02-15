CPP = g++

PROGRAM = RCPSP

INST_PATH = /usr/local/bin/

OBJ = InputReader.o RCPSP.o SimpleTabuList.o ScheduleSolver.o SourcesLoad.o AdvanceTabuList.o
INC = InputReader.h SimpleTabuList.h ScheduleSolver.h SourcesLoad.h DefaultConfigureRCPSP.h ConfigureRCPSP.h AdvanceTabuList.h TabuList.h ConstantsRCPSP.h
SRC = RCPSP.cpp InputReader.cpp SimpleTabuList.cpp ScheduleSolver.cpp SourcesLoad.cpp AdvanceTabuList.cpp

# Pokud chcete ladit vykonost, pouzijte volbu -pg (gprof). Linkování knihoven (-static-libstdc++).
ifdef DEBUG
OPT = -O0 -g
LIBS = -std=c++0x
else
OPT = -pedantic -Wall -march=native -O3 -pipe -funsafe-math-optimizations
LIBS = -fopenmp -std=c++0x
endif

.PHONY: build
.PHONY: install
.PHONY: uninstall
.PHONY: clean
.PHONY: distrib

# Defaultní volba pro příkaz make.
build: $(PROGRAM)

# Zkompiluje program.
$(PROGRAM): $(OBJ)
	$(CPP) $(LIBS) $(OPT) -o $(PROGRAM) $(OBJ)

# Vytvoří objekty pro sestavení programů.
%.o: %.cpp
	$(CPP) $(LIBS) $(OPT) -c -o $@ $<

# Nainstaluje program.
install: build
	cp $(PROGRAM) $(INST_PATH)

# Smaže soubory a program po překladu.
clean:
	rm -f *.o $(PROGRAM)

# Odinstaluje program.
uninstall:
	rm -f $(INST_PATH)$(PROGRAM)

# Vytvoří zabalený balíček se zdrojovými kódy.
distrib:
	tar -c $(SRC) $(INC) Makefile > $(PROGRAM).tar; \
    bzip2 $(PROGRAM).tar

# Závislost všech objektových souborů na hlavičkách.
${OBJ}: ${INC}

