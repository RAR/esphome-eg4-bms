# Host-side tests for the shared Modbus framing and parameter block code.
# Plain g++ and python3, no ESPHome.
CXX ?= g++
CXXFLAGS ?= -std=c++17 -O1 -g -Wall -Wextra -Werror
PYTHON ?= python3

all: run

passive_link_test: passive_link_test.cpp ../components/eg4_modbus/eg4_modbus_frame.h
	$(CXX) $(CXXFLAGS) -o $@ $<

rs232_service_test: rs232_service_test.cpp ../components/eg4_modbus/eg4_modbus_frame.h \
		../components/eg4_bms/eg4_bms_settings.h
	$(CXX) $(CXXFLAGS) -o $@ $<

run: passive_link_test rs232_service_test
	./passive_link_test eg4_link_frames.txt
	./rs232_service_test rs232_service_frames.txt
	$(PYTHON) settings_table_test.py

clean:
	rm -f passive_link_test rs232_service_test

.PHONY: all run clean
