# test:Test.cpp base64.cpp
# 	g++ -o $@ $^ -std=c++17 -lpthread -lstdc++fs -ljsoncpp -lbundle -levent 
# gdb_test:Test.cpp
# 	g++ -g -o $@ $^ -std=c++17 -lpthread -lstdc++fs -ljsoncpp  -lbundle -levent
# .PHONY:clean
# clean:
# 	rm -rf test gdb_test ./deep_storage ./low_storage ./logfile storage.data


# BUNDLE_PATH := $(HOME)/bundle

# test: Test.cpp base64.cpp
# 	g++ -o $@ $^ -std=c++17 -I$(BUNDLE_PATH) -L$(BUNDLE_PATH) -Wl,-rpath=$(BUNDLE_PATH) -lpthread -lstdc++fs -ljsoncpp -lbundle -levent

# gdb_test: Test.cpp base64.cpp
# 	g++ -g -o $@ $^ -std=c++17 -I$(BUNDLE_PATH) -L$(BUNDLE_PATH) -Wl,-rpath=$(BUNDLE_PATH) -lpthread -lstdc++fs -ljsoncpp -lbundle -levent

# clean:
# 	rm -rf test gdb_test ./deep_storage ./low_storage ./logfile storage.data




# 我这里bundle的路径：不要忘记
BUNDLE_PATH := $(HOME)/bundle

all: test

test: Test.cpp base64.cpp CliBackupLog.cpp
	g++ -o $@ $^ -std=c++17 -I. -I$(BUNDLE_PATH) -L$(BUNDLE_PATH) -Wl,-rpath=$(BUNDLE_PATH) -lpthread -lstdc++fs -ljsoncpp -lbundle -levent

gdb_test: Test.cpp base64.cpp 
	g++ -g -o $@ $^ -std=c++17 -I. -I$(BUNDLE_PATH) -L$(BUNDLE_PATH) -Wl,-rpath=$(BUNDLE_PATH) -lpthread -lstdc++fs -ljsoncpp -lbundle -levent

perf_test: performance_test.cpp CliBackupLog.cpp
	g++ -O2 -o $@ $^ -std=c++17 -I. -lpthread -ljsoncpp
clean:
	rm -rf test gdb_test perf_test ./deep_storage ./low_storage ./logfile ./perftest_log storage.data storage.dat

.PHONY: all clean test gdb_test perf_test