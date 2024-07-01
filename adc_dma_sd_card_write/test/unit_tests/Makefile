# Steps needed (windows only):
# - install cygwin
# - install gtest
# - run this Makefile from the cygwin terminal

# TODO this is very quick and dirty just to get something working fast, we need to integrate tests in with the rest of the system in a reasonable way

BUILD_DIR = ./build/
TEST_EXECUTABLE = $(BUILD_DIR)test.a
TEST_REPORT = test_results.xml

# run from cygwin terminal

# the default command priots only a brief summary plus any failing tests
all: $(BUILD_DIR)
	g++ -o $(TEST_EXECUTABLE) test_data_converters.cpp test_wav_header.cpp ../../data_converters.c ../../wav_header.c test_helpers.cpp -I header_overrides/ -I../../ -lgtest -lgtest_main -lgmock -Wno-narrowing
	./$(TEST_EXECUTABLE) --gtest_brief=1

# the verbose version prints out info about all the tests, not just failing ones
verbose: $(BUILD_DIR)
	g++ -o $(TEST_EXECUTABLE) test_data_converters.cpp test_wav_header.cpp ../../data_converters.c ../../wav_header.c test_helpers.cpp -I header_overrides/ -I../../ -lgtest -lgtest_main -lgmock -Wno-narrowing
	./$(TEST_EXECUTABLE)

# the file command generates XML output of the test results
file: $(BUILD_DIR)
	g++ -o $(TEST_EXECUTABLE) test_data_converters.cpp test_wav_header.cpp ../../data_converters.c ../../wav_header.c test_helpers.cpp -I header_overrides/ -I../../ -lgtest -lgtest_main -lgmock -Wno-narrowing
	./$(TEST_EXECUTABLE) --gtest_brief=1 --gtest_output=xml:$(TEST_REPORT)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

clean:
	rm $(TEST_REPORT)
	rm -rf $(BUILD_DIR)
