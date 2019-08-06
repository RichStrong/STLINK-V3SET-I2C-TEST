# Makefile for STLINK-V3SET
VPATH = bridge:common:error

CC := g++
OBJ := DEMO_IIC
CFLAGS := -Wall -g -Ibridge -Icommon -Ierror
OBJ_C := main_example.o bridge.o criticalsectionlock.o stlink_device.o stlink_interface.o ErrLog.o lib/libSTLinkUSBDriver.so

$(OBJ):$(OBJ_C)
	$(CC) -o $@ $^ $(CFLAGS)

%.o:%.cpp
	$(CC) -c $< $(CFLAGS)

clean:
	rm -rf *.o $(OBJ) 

