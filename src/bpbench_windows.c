#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

const LPVOID PAGE_ADDR = (void *)0x2091a4f0000; // use one that has been used
                                                // randomly by Windows earlier
const size_t PAGE_SIZE = 4096;
const unsigned char NOP = 0x90;
const unsigned char RET = 0xC3;

void check_system_page_size() {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  if (si.dwPageSize != PAGE_SIZE) {
    fprintf(stderr, "Unsupported pagesize: %lu, expected %zu\n", si.dwPageSize,
            PAGE_SIZE);
    exit(1);
  }
}

void *alloc_exec_page() {
  void *exec_mem = VirtualAlloc(
      PAGE_ADDR,                // Use fixed address
      PAGE_SIZE,                // Allocate one page
      MEM_COMMIT | MEM_RESERVE, // Commit and reserve memory
      PAGE_EXECUTE_READWRITE    // Read, write, and execute permissions
  );

  if (!exec_mem) {
    fprintf(stderr, "VirtualAlloc failed with error %lu\n", GetLastError());
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

LONGLONG time_execution(void *addr) {
  LARGE_INTEGER start, end, frequency;
  void (*func)() = addr;

  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&start);
  func();
  QueryPerformanceCounter(&end);

  return (end.QuadPart - start.QuadPart) * 1e9 / frequency.QuadPart;
}

void bench_exec(void *addr) {
  unsigned int repetitions = 100000;
  LONGLONG times[repetitions];
  for (int i = 0; i < repetitions; i++) {
    times[i] = time_execution(addr);
  }

  LONGLONG sum = times[0];
  LONGLONG min = times[0];
  LONGLONG max = times[0];
  for (int i = 1; i < repetitions; i++) {
    LONGLONG time = times[i];
    sum += time;
    if (time < min) {
      min = time;
    } else if (time > max) {
      max = time;
    }
  }
  double avg = (double)sum / repetitions;
  printf("repetitions : %u\n", repetitions);
  printf("average time: %.2f ns\n", avg);
  printf("min time    : %lld ns\n", min);
  printf("max time    : %lld ns\n", max);
}

int main() {
  check_system_page_size();

  void *exec_mem = alloc_exec_page();
  write_code_to_page(exec_mem);
  void *breakpoint_location = (void *)((uintptr_t)exec_mem + PAGE_SIZE - 1);

  DWORD pid = GetCurrentProcessId();

  getchar();

  printf("Memory page start address: %p\n", exec_mem);
  printf("Place the breakpoint here: %p\n", breakpoint_location);
  printf("Process ID               : %lu\n", pid);

  printf("Executing on page...\n");
  bench_exec(breakpoint_location);

  printf("Executing whole page...\n");
  bench_exec(exec_mem);

  // Free the memory
  if (!VirtualFree(exec_mem, 0, MEM_RELEASE)) {
    fprintf(stderr, "VirtualFree failed with error %lu\n", GetLastError());
    return 1;
  }

  return 0;
}
