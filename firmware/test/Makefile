# Host unit tests (no AVR). Run from firmware/test/.
# Usage: make -C test  or  cd test && make

CC     = gcc
CFLAGS = -Wall -Wextra -I. -I.. -I../protocol -I../utils -I../display -I../control

# Protocol: use test config (no AVR)
PROTOCOL_CFLAGS = -I. -I../protocol
# Utils for host: stubs
UTILS_CFLAGS = -DVTTESTER_HOST_TEST -I../utils
# Display: host build (build functions only)
DISPLAY_CFLAGS = -DVTTESTER_HOST_TEST -I. -I.. -I../utils -I../display
# Control: host build with test config
CONTROL_CFLAGS = -DVTTESTER_HOST_TEST -I. -I.. -I../utils -I../control

OBJS = main.o test_protocol.o test_utils.o test_display.o test_control.o vttester_remote.o utils.o display.o control.o control_harness.o
TARGET = test_runner

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

main.o: main.c test_common.h
	$(CC) $(CFLAGS) -c main.c -o main.o

test_protocol.o: test_protocol.c test_common.h
	$(CC) $(CFLAGS) -c test_protocol.c -o test_protocol.o

test_utils.o: test_utils.c test_common.h
	$(CC) $(CFLAGS) -c test_utils.c -o test_utils.o

# config/config.h is test/config/config.h when -I. is used
vttester_remote.o: ../protocol/vttester_remote.c ../protocol/vttester_remote.h
	$(CC) $(CFLAGS) $(PROTOCOL_CFLAGS) -c ../protocol/vttester_remote.c -o vttester_remote.o

utils.o: ../utils/utils.c ../utils/utils.h
	$(CC) $(CFLAGS) $(UTILS_CFLAGS) -c ../utils/utils.c -o utils.o

test_display.o: test_display.c test_common.h ../display/display.h
	$(CC) $(CFLAGS) -c test_display.c -o test_display.o

display.o: ../display/display.c ../display/display.h ../utils/utils.h
	$(CC) $(CFLAGS) $(DISPLAY_CFLAGS) -c ../display/display.c -o display.o

test_control.o: test_control.c test_common.h ../control/control.h config/config.h
	$(CC) $(CFLAGS) -c test_control.c -o test_control.o

control_harness.o: control_harness.c config/config.h
	$(CC) $(CFLAGS) -c control_harness.c -o control_harness.o

control.o: ../control/control.c ../control/control.h ../utils/utils.h
	$(CC) $(CFLAGS) $(CONTROL_CFLAGS) -c ../control/control.c -o control.o

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
