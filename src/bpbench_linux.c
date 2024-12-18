#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

const size_t PAGE_SIZE = 4096;
const unsigned char NOP = 0x90;
const unsigned char RET = 0xc3;

void check_system_page_size() {
  size_t page_size = sysconf(_SC_PAGESIZE);
  if (page_size != PAGE_SIZE) {
    fprintf(stderr, "Unsupported pagesize: %zu, expected %zu\n", page_size,
            PAGE_SIZE);
    exit(1);
  }
}

void *alloc_exec_page() {
  // Allocate executable memory using mmap
  // mmap's allocations are page-aligned by default
  void *exec_mem =
      mmap(NULL,      // Let the kernel choose the address
           PAGE_SIZE, // Allocate (at least) one page
           PROT_READ | PROT_WRITE |
               PROT_EXEC,               // Read, write, and execute permissions
           MAP_PRIVATE | MAP_ANONYMOUS, // No file backing, anonymous memory
           -1,                          // No file descriptor
           0 // Offset (not applicable for anonymous mappings)
      );

  if (exec_mem == MAP_FAILED) {
    perror("mmap failed");
    exit(1);
  }

  return exec_mem;
}

void write_code_to_page(void *page_addr) {
  unsigned char *write_ptr = (unsigned char *)page_addr;

  for (int i = 0; i < 4095; i++) {
    write_ptr[i] = NOP;
  }
  write_ptr[4095] = RET;
}

int main() {
  check_system_page_size();

  void *exec_mem = alloc_exec_page();
  write_code_to_page(exec_mem);
  void *breakpoint_location = exec_mem + PAGE_SIZE - 1;

  printf("Memory page start address: %p\n", exec_mem);
  printf("Place the breakpoint here: %p\n", breakpoint_location);

  // Execute the code and measure execution time
  printf("Executing page...\n");
  struct timespec start, end;
  void (*func)() = exec_mem;
  clock_gettime(CLOCK_MONOTONIC, &start);
  func();
  clock_gettime(CLOCK_MONOTONIC, &end);
  long seconds = end.tv_sec - start.tv_sec;
  long nanoseconds = end.tv_nsec - start.tv_nsec;
  long elapsed = seconds * 1e9 + nanoseconds;
  printf("Execution time: %luns\n", elapsed);

  // Free the memory
  if (munmap(exec_mem, PAGE_SIZE) != 0) {
    perror("munmap failed");
    return 1;
  }

  return 0;
}
