TARGET := display
CC     := gcc
LIBS = -lm

OBJ := obj

mkfile_path := $(shell pwd)/$(lastword $(MAKEFILE_LIST))
dir=$(shell dirname $(mkfile_path))
$(shell mkdir -p $(dir)/$(OBJ))

SRCDIRS :=  project/ \
			hardware/rpiInfo \
			hardware/st7735  

SRCS := $(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c))
NOT_DIR :=$(notdir $(SRCS))
OBJS := $(patsubst %.c, $(OBJ)/%.o, $(NOT_DIR)) 

INCLUDE := $(patsubst %, -I %, $(SRCDIRS))

VPATH := $(SRCDIRS)

$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(LIBS)
$(OBJS) : obj/%.o : %.c
	$(CC) -c $(INCLUDE) -o $@ $<


clean:
	sudo rm -rf $(OBJ)
	sudo rm -rf $(TATGET)