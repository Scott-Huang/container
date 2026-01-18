all: btree_test run

DEPS := $(shell find ../vtl)

btree_test: btree_test.cpp $(shell find ../vtl)
	g++ ${CXXFLAGS} -I $(CURDIR)/../ btree_test.cpp -o test

.PHONY: btree_test
run: btree_test
	./test

clean:
	rm -f test
