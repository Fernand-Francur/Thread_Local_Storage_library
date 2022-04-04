override CFLAGS := -Wall -std=gnu99 -pedantic -O0 -g -pthread $(CFLAGS)
#-Wall -Werror
override LDLIBS := -pthread $(LDLIBS)

tls.o: tls.c

tls_test_1.o: tls_test_1.c tls.h

tls_test_1: tls_test_1.o tls.o
	$(CC) $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

test_files=tls_test_1

# Run the test programs
check: $(test_files) 
	bash run_tests.sh $(test_files) 

clean:
	rm -f *.o $(test_files) *~

# .PHONY: clean

# clean:
# 	rm -f tls.o
