all: test run

test: test.cpp ../definition.h ../bptree/bptree.h ../vector/vector.h interval_set.h interval.h
	g++ ${CXXFLAGS} test.cpp -o test

.PHONY: test
run: test
	./test

clean:
	rm -f test
