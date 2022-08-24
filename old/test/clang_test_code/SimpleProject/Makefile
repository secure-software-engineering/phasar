#
# =============================================================================
#
# Philipp Schubert
#
#    Copyright (C) 2017
#    Software Engineering Group
#    Heinz Nixdorf Institut
#    University of Paderborn
#    philipp.schubert@upb.de
#
# =============================================================================
#

# The compiler to use
CC := clang++
EXE := main

# Some useful compiler flags
FLAGS := -std=c++14
FLAGS += -Wall
FLAGS += -Wextra
# FLAGS += -g # use for debugging

# The target to compile the C++ code into an executable file
all:
	$(CC) $(FLAGS) *.cpp -o $(EXE)

# The target to clean the automatically generated file(s)
clean:
	rm -f $(EXE)
