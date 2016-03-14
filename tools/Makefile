
# ------------------ Compilation options ------------------------

# Loads math library.  
LIBS = -lm

# Flags for the C compiler:
#   -Wall for strict gcc warnings (requires prototypes for all functions).
#   -g to produce debug data for gdb
#   -O for optimization
CPPFLAGS = -Wall -O2 -std=c++0x -Wno-deprecated-register
#CPPFLAGS += -DFASTLOG
#CPPFLAGS = -g

CC = g++

# --------------------- Code modules ----------------------------

# Object files
OBJ = mdl-performance.o

# Definitions
#DEFS = MDLClassifier.h

# ------------------------ Rules --------------------------------

all: mdl-performance

classify: mdl-performance.o ${OBJ}
	${CC} -o $@ ${CFLAGS} mdl-performance.o ${OBJ} ${LIBS}

clean:
	rm *.o; rm mdl-performance

# Implicit rule used by Gnu Make: $(CC) -c $(CPPFLAGS) $(CFLAGS)
${OBJ}: ${DEFS}
