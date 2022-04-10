override CFLAGS := -Wall  -std=gnu99 -pedantic -O0 -g -pthread $(CFLAGS)
#-Wall -Werror
override LDLIBS := -pthread $(LDLIBS)

tls.o: tls.c

tls_test_1.o: tls_test_1.c tls.h

tls_test_1: tls_test_1.o tls.o
	$(CC) $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

tls_test_2: tls_test_2.o tls.o
	$(CC) $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

tls_test_4: tls_test_4.o tls.o
	$(CC) $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

clone_test: clone_test.o tls.o
	$(CC) $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

test_many: test_many.o tls.o
	$(CC) $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

test_more: test_more.o tls.o
	$(CC) $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

test_files=tls_test_1 tls_test_2 clone_test tls_test_4 test_more

# Run the test programs
check: $(test_files) 
	bash run_tests.sh $(test_files) 

clean:
	rm -f *.o $(test_files) *~

# .PHONY: clean

# clean:
# 	rm -f tls.o
