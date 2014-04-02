
#
# Chronos makefile
# @author Bruno Basseto (bruno@wise-ware.org)
#
CC = mips-elf-gcc
AR = mips-elf-ar

#
# Library name
#
LIBRARY = chronos.a

#
# Object files
#
OBJECTS = threads.o chronos.o list.o

#
# Architecture and compiler flags.
#
ARCH = -EL -march=m4k
CFLAGS += -msoft-float

#
# Search paths
#
SRC_PATH = ../src
INCLUDE_PATH = ../include ../../../gcc4pic32/include

CFLAGS += $(patsubst %, -I%, $(INCLUDE_PATH))

#
# Targets and rules...
#
all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(AR) -r $(LIBRARY) $(OBJECTS)

%.o: $(SRC_PATH)/%.c      
	$(CC) $(ARCH) $(CFLAGS) -c $<

clean:
	rm -f $(OBJECTS) $(LIBRARY)
