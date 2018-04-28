#
# Shadow_Talkie
#
# @ Illinois, 2018
#

EXE_ROUTER = router
EXE_TALKIE = talkie
EXE_ALL = $(EXE_ROUTER) $(EXE_TALKIE)

DEPENDS = format.o lib.o window.o
OBJS_ROUTER = $(EXE_ROUTER).o $(DEPENDS) 
OBJS_TALKIE = $(EXE_TALKIE).o $(DEPENDS)

# c compiler flags
CC = clang
CFLAGS = -pthread -Wall -lncurses

# linker flags
LD = clang
LDFLAGS = -pthread -Wall# -lncurses

# phony: all
.PHONY: all
all: $(EXE_ALL)

# Compiling
$(EXE_ROUTER): $(OBJS_ROUTER)
	$(CC) $(CFLAGS) -o $(EXE_ROUTER) $(OBJS_ROUTER)

$(EXE_TALKIE): $(OBJS_TALKIE)
	$(CC) $(CFLAGS) -o $(EXE_TALKIE) $(OBJS_TALKIE)

# Linking
router.o: router.c includes/router.h includes/format.h includes/lib.h
	$(LD) $(LDFLAGS) -c router.c

talkie.o: talkie.c includes/talkie.h includes/format.h includes/lib.h includes/window.h
	$(LD) $(LDFLAGS) -c talkie.c

format.o: format.c includes/format.h includes/lib.h includes/window.h
	$(LD) $(LDFLAGS) -c format.c

lib.o: lib.c includes/lib.h includes/window.h
	$(LD) $(LDFLAGS) -c lib.c

window.o: window.c includes/window.h includes/lib.h
	$(LD) $(LDFLAGS) -c window.c

# phony: clean
.PHONY: clean
clean:
	-rm -rf *.o $(EXE_ALL)