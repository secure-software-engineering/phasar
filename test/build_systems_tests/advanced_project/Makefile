######
# Philipp D. Schubert
######

###### section variables

EXE := exe
MAIN_FILE := main.c
LIB_1 := lib1/
LIB_2 := lib2/
OBJ_1 := obj1/
OBJ_2 := obj2/

CC := gcc # Benutzer Compiler
FLAGS := -std=c11 # C-Standard
FLAGS += -O3 # optimiert Code so stark wie moeglich
FLAGS += -march=native # optimiert auf vorhandenen Prozessortyp
FLAGS += -Wall # fuer eine handvoll Warnungen
FLAGS += -Wextra # fuer eine paar Warnugen mehr

##### section targets

.PHONY: clean

all: $(EXE)

$(EXE): $(OBJ_1) $(OBJ_2)
	$(CC) $(FLAGS) -I$(LIB_1) -I$(LIB_2) $(OBJ_1)*.o $(OBJ_2)*.o $(MAIN_FILE) -o $(EXE)

$(OBJ_1): $(LIB_1)*.c $(LIB_1)*.h
	mkdir $(OBJ_1); \
	cd $(LIB_1); \
	$(CC) $(FLAGS) -c *.c; \
	mv *.o ../$(OBJ_1); \
	cd -; \

$(OBJ_2): $(LIB_2)*.c $(LIB_2)*.h
	mkdir $(OBJ_2); \
	cd $(LIB_2); \
	$(CC) $(FLAGS) -c *.c; \
	mv *.o ../$(OBJ_2); \
	cd -; \

clean:
	rm -r $(OBJ_1)
	rm -r $(OBJ_2)
	rm $(EXE)
