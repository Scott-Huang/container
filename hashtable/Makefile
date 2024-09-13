all: test run

test: test.cpp ../definition.h ../vector/vector.h hashtable.h
	g++ ${CXXFLAGS} test.cpp -o test

.PHONY: test
run: test
	./test

clean:
	rm -f test
