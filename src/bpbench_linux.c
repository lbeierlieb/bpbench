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

long time_execution(void *addr) {
  struct timespec start, end;
  void (*func)() = addr;
  clock_gettime(CLOCK_MONOTONIC, &start);
  func();
  clock_gettime(CLOCK_MONOTONIC, &end);
  long seconds = end.tv_sec - start.tv_sec;
  long nanoseconds = end.tv_nsec - start.tv_nsec;
  long elapsed = seconds * 1e9 + nanoseconds;

  return elapsed;
}

void bench_exec(void *addr) {
  unsigned int repetitions = 100000;
  long times[repetitions];
  for (int i = 0; i < repetitions; i++) {
    times[i] = time_execution(addr);
  }
  long sum = times[0];
  long min = times[0];
  long max = times[0];
  for (int i = 1; i < repetitions; i++) {
    long time = times[i];
    sum += time;
    if (time < min) {
      min = time;
    } else if (time > max) {
      max = time;
    }
  }
  double avg = (double)sum / repetitions;
  printf("repetitions : %u\n", repetitions);
  printf("average time: %.2f\n", avg);
  printf("min time    : %ld\n", min);
  printf("max time    : %ld\n", max);
}

long time_read_qword(void *addr) {
  struct timespec start, end;
  long val;
  long volatile *ptr = addr;
  clock_gettime(CLOCK_MONOTONIC, &start);
  val = *ptr;
  clock_gettime(CLOCK_MONOTONIC, &end);
  long seconds = end.tv_sec - start.tv_sec;
  long nanoseconds = end.tv_nsec - start.tv_nsec;
  long elapsed = seconds * 1e9 + nanoseconds;

  return elapsed;
}

void bench_read_qword(void *addr) {
  unsigned int repetitions = 100000;
  long times[repetitions];
  for (int i = 0; i < repetitions; i++) {
    times[i] = time_read_qword(addr);
  }
  long sum = times[0];
  long min = times[0];
  long max = times[0];
  for (int i = 1; i < repetitions; i++) {
    long time = times[i];
    sum += time;
    if (time < min) {
      min = time;
    } else if (time > max) {
      max = time;
    }
  }
  double avg = (double)sum / repetitions;
  printf("repetitions : %u\n", repetitions);
  printf("average time: %.2f\n", avg);
  printf("min time    : %ld\n", min);
  printf("max time    : %ld\n", max);
}

long time_read_page(void *addr) {
  struct timespec start, end;
  long val;
  long volatile *ptr = addr;
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (int i = 0; i < PAGE_SIZE / sizeof(long); i++) {
    val = ptr[i];
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  long seconds = end.tv_sec - start.tv_sec;
  long nanoseconds = end.tv_nsec - start.tv_nsec;
  long elapsed = seconds * 1e9 + nanoseconds;

  return elapsed;
}

void bench_read_page(void *addr) {
  unsigned int repetitions = 100000;
  long times[repetitions];
  for (int i = 0; i < repetitions; i++) {
    times[i] = time_read_page(addr);
  }
  long sum = times[0];
  long min = times[0];
  long max = times[0];
  for (int i = 1; i < repetitions; i++) {
    long time = times[i];
    sum += time;
    if (time < min) {
      min = time;
    } else if (time > max) {
      max = time;
    }
  }
  double avg = (double)sum / repetitions;
  printf("repetitions : %u\n", repetitions);
  printf("average time: %.2f\n", avg);
  printf("min time    : %ld\n", min);
  printf("max time    : %ld\n", max);
}

int main() {
  check_system_page_size();

  void *exec_mem = alloc_exec_page();
  write_code_to_page(exec_mem);
  void *breakpoint_location = exec_mem + PAGE_SIZE - 1;

  pid_t pid = getpid();

  printf("Memory page start address: %p\n", exec_mem);
  printf("Place the breakpoint here: %p\n", breakpoint_location);
  printf("Process ID               : %d\n", pid);

  printf("Executing on page...\n");
  bench_exec(breakpoint_location);

  printf("Reading from page...\n");
  bench_read_qword(breakpoint_location);

  printf("Reading whole page...\n");
  bench_read_page(exec_mem);

  // Free the memory
  if (munmap(exec_mem, PAGE_SIZE) != 0) {
    perror("munmap failed");
    return 1;
  }

  return 0;
}
