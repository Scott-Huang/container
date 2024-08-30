all: test run

test: test.cpp ../definition.h list.h concurrent_list.h
	g++ ${CXXFLAGS} test.cpp -o test

.PHONY: test
run: test
	./test

clean:
	rm -f test
