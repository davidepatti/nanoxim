TARGET_ARCH = macosx
CC     = g++
OPT    = -O2 # -O3
DEBUG  = -g
OTHER  = -Wall -Wno-deprecated -arch i386 
CFLAGS = $(OPT) $(OTHER)
#CFLAGS = $(DEBUG) $(OPT) $(OTHER)

MODULE = nanoxim
SRCS = TNet.cpp TRouter.cpp TProcessingElement.cpp TBuffer.cpp \
	TReservationTable.cpp CmdLineParser.cpp DiSR.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)

include ./Makefile.defs
