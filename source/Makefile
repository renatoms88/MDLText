
# ------------------ Compilation options ------------------------

# Loads math library.  
#LIBS = -lm -liconv

# Flags for the C compiler:
#   -Wall for strict gcc warnings (requires prototypes for all functions).
#   -g to produce debug data for gdb
#   -O for optimization
CPPFLAGS = -Wall -O2 -std=c++0x #-Wno-deprecated-register -arch x86_64
#CPPFLAGS += -DFASTLOG
#CPPFLAGS = -g

CC = g++

# --------------------- Code modules ----------------------------

# Object files
OBJ = normalizemime.o comptime.o utils.o tokenizer.o database.o featureRelevance.o textNormalization.o mdlClassifier.o

# Definitions
DEFS = mdlClassifier.h

# ------------------------ Rules --------------------------------

all: mdl-classify mdl-train

mdl-classify: mdl-classify.o ${OBJ}
	${CC} -o $@ ${CFLAGS} mdl-classify.o ${OBJ} ${LIBS}

mdl-train: mdl-train.o ${OBJ}
	${CC} -o $@ ${CFLAGS} mdl-train.o ${OBJ} ${LIBS}

clean:
	rm *.o; rm mdl-classify; rm mdl-train

# Implicit rule used by Gnu Make: $(CC) -c $(CPPFLAGS) $(CFLAGS)
${OBJ}: ${DEFS}
