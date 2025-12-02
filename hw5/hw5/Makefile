CC := gcc
SRCD := src
TSTD := tests
BLDD := build
BIND := bin
INCD := include
LIBD := lib
UTILD := util

EXEC := mazewar
TEST_EXEC := $(EXEC)_tests

MAIN  := $(BLDD)/main.o
LIB := $(LIBD)/$(EXEC).a
LIB_DB := $(LIBD)/$(EXEC)_debug.a

ALL_SRCF := $(wildcard $(SRCD)/*.c)
ALL_LIBF := $(wildcard $(LIBD)/*.o)
ALL_TESTF := $(wildcard $(TSTD)/*.c)
ALL_OBJF := $(patsubst $(SRCD)/%, $(BLDD)/%, $(ALL_SRCF:.c=.o))
ALL_FUNCF := $(filter-out $(MAIN), $(ALL_OBJF))

INC := -I $(INCD)

CFLAGS := -Wall -Werror -Wno-unused-function -MMD
DFLAGS := -g -DDEBUG -DCOLOR
PRINT_STAMENTS := -DERROR -DSUCCESS -DWARN -DINFO

STD := -std=gnu11
TEST_LIB := -lcriterion
LIBS := $(LIB) -lcurses -lpthread
LIBS_DB := $(LIB_DB) -lcurses -lpthread
EXCLUDES := excludes.h

CFLAGS += $(STD)

.PHONY: clean all setup debug

all: setup $(BIND)/$(EXEC) $(INCD)/$(EXCLUDES) $(BIND)/$(TEST_EXEC)

debug: CFLAGS += $(DFLAGS) $(PRINT_STAMENTS)
debug: LIBS := $(LIBS_DB)
debug: all

setup: $(BIND) $(BLDD)
$(BIND):
	mkdir -p $(BIND)
$(BLDD):
	mkdir -p $(BLDD)

$(BIND)/$(EXEC): $(MAIN) $(ALL_FUNCF) $(LIB)
	$(CC) $(MAIN) $(ALL_FUNCF) -o $(BIND)/$(EXEC) $(LIBS)

$(BIND)/$(TEST_EXEC): $(ALL_FUNCF) $(LIB)
	$(CC) $(CFLAGS) $(INC) $(ALL_TESTF) $(ALL_FUNCF) -o $(BIND)/$(TEST_EXEC) $(TEST_LIB) $(LIBS)

$(BLDD)/%.o: $(SRCD)/%.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -rf $(BLDD) $(BIND)

$(INCD)/$(EXCLUDES): $(BIND)/$(EXEC)
	rm -f $@
	touch $@
	if nm $(BIND)/$(EXEC) | grep -w INSTRUCTOR_CLIENT_REGISTRY > /dev/null; then \
           echo "#define NO_CLIENT_REGISTRY" >> $@; \
        fi
	if nm $(BIND)/$(EXEC) | grep -w INSTRUCTOR_MAIN > /dev/null; then \
           echo "#define NO_MAIN" >> $@; \
        fi
	if nm $(BIND)/$(EXEC) | grep -w INSTRUCTOR_MAZE > /dev/null; then \
           echo "#define NO_MAZE" >> $@; \
        fi
	if nm $(BIND)/$(EXEC) | grep -w INSTRUCTOR_PLAYER > /dev/null; then \
           echo "#define NO_PLAYER" >> $@; \
        fi
	if nm $(BIND)/$(EXEC) | grep -w INSTRUCTOR_PROTOCOL > /dev/null; then \
           echo "#define NO_PROTOCOL" >> $@; \
        fi
	if nm $(BIND)/$(EXEC) | grep -w INSTRUCTOR_SERVER > /dev/null; then \
           echo "#define NO_SERVER" >> $@; \
        fi

.PRECIOUS: $(BLDD)/*.d
-include $(BLDD)/*.d
