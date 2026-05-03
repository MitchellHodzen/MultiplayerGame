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
SDL_LOC    := $(LIBS)/SDL3-3.4.4/x86_64-w64-mingw32
SDLIMG_LOC := $(LIBS)/SDL3_image-3.4.2/x86_64-w64-mingw32
SDL_FLAGS  := -I$(SDL_LOC)/include -L$(SDL_LOC)/lib -I$(SDLIMG_LOC)/include -L$(SDLIMG_LOC)/lib -lSDL3 -lSDL3_image
SRCS       := $(wildcard $(SRC)/*.c)
OBJS       := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
EXE        := $(BIN)/client.exe

.PHONY: all run clean

all: $(EXE)

$(EXE): $(OBJS) | $(BIN)
	$(CC) $^ -o $@ $(CFLAGS) $(SDL_FLAGS) 

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) $(SDL_FLAGS) -c $< -o $@

$(OBJ): | $(BIN)
	$(MKDIR) $@

$(BIN) $(OBJ):
	$(MKDIR) $@

run: $(EXE)
	$<

clean:
	$(RMDIR) $(OBJ) $(BIN)
