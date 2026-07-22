MKDIR      := mkdir
RMDIR      := rm -r
CPY        := cp
CC         := gcc
RES        := resources
LIBS       := C:/Libs
BIN        := ./bin
OBJ        := $(BIN)/obj
INCLUDE    := ./include
SRC        := ./src
CMN_LIB    := ../common
CFLAGS     := -g -std=c99 -I$(INCLUDE) -Wall -Wextra -pedantic -I$(CMN_LIB)/include -L$(CMN_LIB)/bin -lcommon
SDL_LOC    := $(LIBS)/SDL3-3.4.12/x86_64-w64-mingw32
SDLIMG_LOC := $(LIBS)/SDL3_image-3.4.4/x86_64-w64-mingw32
SDLTTF_LOC := $(LIBS)/SDL3_ttf-3.2.0/x86_64-w64-mingw32
SDL_FLAGS  := -I$(SDL_LOC)/include -L$(SDL_LOC)/lib -lSDL3 -I$(SDLIMG_LOC)/include -L$(SDLIMG_LOC)/lib -lSDL3_image -I$(SDLTTF_LOC)/include -L$(SDLTTF_LOC)/lib -lSDL3_ttf
ENET_LOC   := $(LIBS)/enet-1.3.18
ENET_FLAGS := -I$(ENET_LOC)/include -L$(ENET_LOC) -lenet64 -lws2_32 -lwinmm
CLAY_LOC   := $(LIBS)/clay
CLAY_FLAGS := -isystem$(CLAY_LOC) -isystem$(CLAY_LOC)/renderers/SDL3
SRCS       := $(wildcard $(SRC)/*.c)
OBJS       := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
EXE        := $(BIN)/client.exe

.PHONY: all run clean

all: $(EXE)

$(EXE): $(OBJS) | $(BIN)
	$(CC) $^ -o $@ $(CFLAGS) $(SDL_FLAGS) $(ENET_FLAGS) $(CLAY_FLAGS)

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) $(SDL_FLAGS) $(ENET_FLAGS) $(CLAY_FLAGS) -c $< -o $@

$(OBJ): | $(BIN)
	$(MKDIR) $@

$(BIN) $(OBJ):
	$(MKDIR) $@
	$(CPY) -r $(RES) $(BIN)/
	$(CPY) *.dll $(BIN)/

run: $(EXE)
	$<

clean:
	$(RMDIR) $(OBJ) $(BIN)
