TARGET_ARCH = linux
CC     = g++
OPT    = -O2 # -O3
DEBUG  = -g
OTHER  = -Wall -Wno-deprecated
CFLAGS = $(OPT) $(OTHER)
#CFLAGS = $(DEBUG) $(OPT) $(OTHER)

MODULE = nanoxim
SRCS = TNet.cpp TRouter.cpp TProcessingElement.cpp TBuffer.cpp \
	TReservationTable.cpp CmdLineParser.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)

include ./Makefile.defs
