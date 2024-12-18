#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

unsigned char code[] = {
    0x48, 0xc7, 0xc0, 0x05, 0x00, 0x00, 0x00, // mov rax, 5
    0xc3                                      // ret
};

int main() {
  // Determine page size
  size_t pagesize = sysconf(_SC_PAGESIZE);
  if (pagesize != 4096) {
    fprintf(stderr, "Unsupported pagesize: %zu, expected 4096\n", pagesize);
    return 1;
  }

  // Allocate executable memory using mmap
  // mmap's allocations are page-aligned by default
  void *exec_mem =
      mmap(NULL,     // Let the kernel choose the address
           pagesize, // Allocate (at least) one page
           PROT_READ | PROT_WRITE |
               PROT_EXEC,               // Read, write, and execute permissions
           MAP_PRIVATE | MAP_ANONYMOUS, // No file backing, anonymous memory
           -1,                          // No file descriptor
           0 // Offset (not applicable for anonymous mappings)
      );

  if (exec_mem == MAP_FAILED) {
    perror("mmap failed");
    return 1;
  }

  printf("Memory allocated at %p\n", exec_mem);

  // Copy the machine code to the allocated memory
  memcpy(exec_mem, code, sizeof(code));

  // Execute the code and measure execution time
  printf("Executing page...\n");
  struct timespec start, end;
  unsigned long (*func)() = exec_mem;
  clock_gettime(CLOCK_MONOTONIC, &start);
  unsigned long return_value = func();
  clock_gettime(CLOCK_MONOTONIC, &end);
  long seconds = end.tv_sec - start.tv_sec;
  long nanoseconds = end.tv_nsec - start.tv_nsec;
  long elapsed = seconds * 1e9 + nanoseconds;
  printf("Code returned %lu\n", return_value);
  printf("Execution time: %luns\n", elapsed);

  // Free the memory
  if (munmap(exec_mem, pagesize) != 0) {
    perror("munmap failed");
    return 1;
  }

  return 0;
}
