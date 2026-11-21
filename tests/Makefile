CXX ?= c++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -Werror -pedantic

.PHONY: test

test:
	@set -eu; \
	build_dir=$$(mktemp -d); \
	trap 'rm -rf "$$build_dir"' EXIT; \
	$(CXX) $(CXXFLAGS) \
		../Firmware/Light_Clock_Firmware/Clock_Command.cpp \
		Clock_Command_Test.cpp \
		-o "$$build_dir/clock-command-test"; \
	"$$build_dir/clock-command-test"
