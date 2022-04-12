Copyright Timothy Borunov 2022

PROGRAM: tls.c
// The program is a library of functions which permit the generation of Local Storage Area for each thread.
// The main concept is that this memory storage is unique to each thread and can be accessed only using the 
// commands provided in this library.

FILES: tls.c tls.h 
// Build library object: tls.o by running make in commandline in a directory with the FILES

FUNCTIONS:
  tls_create(int size) //creates a thread local storage of a specified size in bytes
  tls_destroy()    //destroys a thread local storage
  tls_clone(pthread_t tid) // clones LSA, allows reads, only copying on write
  tls_write(offset, length, buffer) // writes the length in buffer to offset in LSA
  tls_read(offset, length, buffer) // reads the length at offset in LSA to buffer

No resources besides the man pages were used for this assignment. It was more straightforward than the previous
and required no really new concepts but utilized those from previous assignments. Overall made me feel more confident
and solidified my understanding with previous topics we covered.