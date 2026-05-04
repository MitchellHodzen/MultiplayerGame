MKDIR      := mkdir
RMDIR      := rm -r
CC         := gcc
LIBS       := C:/Libs
BIN        := ./bin
OBJ        := $(BIN)/obj
INCLUDE    := ./include
SRC        := ./src
CMN_LIB    := ../common
CFLAGS     := -I$(INCLUDE) -Wall -Wextra -pedantic -I$(CMN_LIB)/include -L$(CMN_LIB)/bin -lcommon
ENET_LOC   := $(LIBS)/enet-1.3.18
ENET_FLAGS := -I$(ENET_LOC)/include -L$(ENET_LOC) -lenet64 -lws2_32 -lwinmm
SRCS       := $(wildcard $(SRC)/*.c)
OBJS       := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
EXE        := $(BIN)/server.exe

.PHONY: all run clean

all: $(EXE)

$(EXE): $(OBJS) | $(BIN)
	$(CC) $^ -o $@ $(CFLAGS) $(ENET_FLAGS) 

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) $(ENET_FLAGS) -c $< -o $@

$(OBJ): | $(BIN)
	$(MKDIR) $@

$(BIN) $(OBJ):
	$(MKDIR) $@

run: $(EXE)
	$<

clean:
	$(RMDIR) $(OBJ) $(BIN)
