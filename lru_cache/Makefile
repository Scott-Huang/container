all: test run

test: test.cpp ../definition.h ../list/list.h ../list/concurrent_list.h lru_cache.h concurrent_lru_cache.h concurrent_lru_array_cache.h
	g++ ${CXXFLAGS} test.cpp -o test

.PHONY: test
run: test
	./test

clean:
	rm -f test
