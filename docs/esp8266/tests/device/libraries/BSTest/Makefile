PYTHON_ENV_DIR=virtualenv
TEST_EXECUTABLE=test/test

all: test

install: $(PYTHON_ENV_DIR)

clean:
	rm -rf $(PYTHON_ENV_DIR)
	rm -rf $(TEST_EXECUTABLE)

$(PYTHON_ENV_DIR):
	virtualenv --no-site-packages $(PYTHON_ENV_DIR)
	source $(PYTHON_ENV_DIR)/bin/activate && pip install -r requirements.txt

test: $(TEST_EXECUTABLE) $(PYTHON_ENV_DIR)
	source $(PYTHON_ENV_DIR)/bin/activate && python runner.py -e $(TEST_EXECUTABLE)

$(TEST_EXECUTABLE): test/test.cpp
	g++ -std=c++11 -Isrc -o $@ test/test.cpp

.PHONY: test clean install all
