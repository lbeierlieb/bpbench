#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

unsigned char code[] = {
    0xc3 // `ret` instruction (just returns immediately)
};

int main() {
  // Step 1: Determine page size
  size_t pagesize = sysconf(_SC_PAGESIZE);

  // Step 2: Allocate executable memory using mmap
  void *exec_mem =
      mmap(NULL,     // Let the kernel choose the address
           pagesize, // Allocate at least one page
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

  // Step 3: Copy the machine code to the allocated memory
  memcpy(exec_mem, code, sizeof(code));

  // Step 4: Execute the code
  printf("Executing shellcode...\n");
  void (*func)() = exec_mem;
  func();

  // Step 5: Free the memory
  if (munmap(exec_mem, pagesize) != 0) {
    perror("munmap failed");
    return 1;
  }

  return 0;
}
