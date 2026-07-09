MKDIR      := mkdir
RMDIR      := rm -r
CC         := gcc
BIN        := ./bin
OBJ        := $(BIN)/obj
INCLUDE    := ./include
SRC        := ./src
CFLAGS     := -g -std=c99 -I$(INCLUDE) -Wall -Wextra -pedantic
SRCS       := $(wildcard $(SRC)/*.c)
OBJS       := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
LIBOUT     := $(BIN)/libcommon.a

.PHONY: all run clean

all: $(LIBOUT)

$(LIBOUT): $(OBJS) | $(BIN)
	ar rsc $@ -o $^

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ): | $(BIN)
	$(MKDIR) $@

$(BIN) $(OBJ):
	$(MKDIR) $@

run: $(EXE)
	$<

clean:
	$(RMDIR) $(OBJ) $(BIN)
