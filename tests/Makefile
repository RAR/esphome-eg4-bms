# Host-side tests for the shared Modbus framing code. Plain g++, no ESPHome.
CXX ?= g++
CXXFLAGS ?= -std=c++17 -O1 -g -Wall -Wextra -Werror

all: run

passive_link_test: passive_link_test.cpp ../components/eg4_modbus/eg4_modbus_frame.h
	$(CXX) $(CXXFLAGS) -o $@ $<

run: passive_link_test
	./passive_link_test eg4_link_frames.txt

clean:
	rm -f passive_link_test

.PHONY: all run clean
