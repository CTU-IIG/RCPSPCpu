CPP = g++

PROGRAM = RCPSP

INST_PATH = /usr/local/bin/

OBJ = InputReader.o RCPSP.o SimpleTabuList.o ScheduleSolver.o SourcesLoad.o TabuList.o
INC = InputReader.h SimpleTabuList.h ScheduleSolver.h SourcesLoad.h ConfigureRCPSP.h TabuList.h
SRC = RCPSP.cpp InputReader.cpp SimpleTabuList.cpp ScheduleSolver.cpp SourcesLoad.cpp TabuList.cpp

# Pokud chcete ladit vykonost, pouzijte volbu -pg (gprof).
ifdef DEBUG
OPT = -O0 -g -std=c++0x
else
OPT = -fopenmp -std=c++0x -pedantic -Wall -O4 -pipe -funsafe-math-optimizations
endif

# Linkování knihoven.. (-static-libstdc++)
#LIB_SMP = -fopenmp -std=c++0x

.PHONY: build
.PHONY: install
.PHONY: uninstall
.PHONY: clean
.PHONY: distrib

# Defaultní volba pro příkaz make.
build: $(PROGRAM)

# Zkompiluje program.
$(PROGRAM): $(OBJ)
	$(CPP) $(OBJ) -o $(PROGRAM) $(OPT)

# Vytvoří objekty pro sestavení programů.
%.o: %.cpp
	$(CPP) -c -o $@ $(OPT) $<

# Nainstaluje program.
install: build
	cp $(PROGRAM) $(INST_PATH)

# Smaže soubory a program po překladu.
clean:
	rm -f *.o

# Odinstaluje program.
uninstall:
	rm -f $(INST_PATH)$(PROGRAM)

# Vytvoří zabalený balíček se zdrojovými kódy.
distrib:
	tar -c $(SRC) $(INC) Makefile > $(PROGRAM).tar; \
    bzip2 $(PROGRAM).tar

# Závislost všech objektových souborů na hlavičkách.
${OBJ}: ${INC}

