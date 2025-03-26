/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"
#include "pmm.h"
#include "vmm.h"
#include "spike_interface/spike_utils.h"

#define MAX_num 4096
uint64 st_addr[MAX_num] = {0};
int size[MAX_num] = {0}, free[MAX_num] = {0}, next[MAX_num] = {-1};
int st = 0, idx = 0;

void assign_value(uint64 st, int si, int fr, int ne)
{
  st_addr[idx] = st;
  size[idx] = si;
  free[idx] = fr;
  next[idx] = ne;
}

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  // buf is now an address in user space of the given app's user stack,
  // so we have to transfer it into phisical address (kernel is running in direct mapping).
  assert( current );
  char* pa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)buf);
  sprint(pa);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process). 
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}

//
// maybe, the simplest implementation of malloc in the world ... added @lab2_2
//
uint64 sys_user_allocate_page(uint64 size_space) {
  uint64 size_tmp = size_space;
  int p = st, pre = st;
  uint64 va = 0;
  // sprint("\n\nbefore the alloc!\n");
  // while(p != -1)
  // {
  //   sprint("idx = %d\nst_adr = %x\nsize = %d\nfree = %d\n--------------\n", p, st_addr[p], size[p], free[p]);
  //   p = next[p];
  // }
  // p = st;
  while(p != -1 && size_tmp > 0)
  { // 找到一个合适的大空间
    if(free[p] == 1 && size[p] >= size_tmp)
    {
      assign_value(st_addr[p] + size_tmp, size[p] - size_tmp, 1, next[p]);
      size[p] = size_tmp;
      free[p] = 0;
      next[p] = idx ++;
      return st_addr[p];
    }
    pre = p;
    p = next[p];
  }

  p = pre;
  uint64 va_st = 0;
  if(size_tmp > 0)
  {
    if(free[p] == 1 && size[p] > 0)  // 末尾空闲空间加以利用
    {
      free[p] = 0;
      size_tmp -= size[p];
      va_st = st_addr[p];
    }
    void* pa = alloc_page();
    va = g_ufree_page;
    g_ufree_page += PGSIZE;
    user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
    prot_to_type(PROT_WRITE | PROT_READ, 1));

    assign_value(va, size_tmp, 0, -1);
    next[p] = idx ++;
    p = next[p];
    assign_value(va + size_tmp, 4096 - size_tmp, 1, -1);
    next[p] = idx ++;

    if(va_st != 0) va = va_st;
  }

  return va;
}

//
// reclaim a page, indicated by "va". added @lab2_2
//
uint64 sys_user_free_page(uint64 va) {
  int p = st;
  // sprint("\n\nbefore the free!\n");
  // while(p != -1)
  // {
  //   sprint("idx = %d\nst_adr = %x\nsize = %d\nfree = %d\n--------------\n", p, st_addr[p], size[p], free[p]);
  //   p = next[p];
  // }
  // p = st;
  while(p != -1)
  {
    if(st_addr[p] == va && free[p] == 0)
    {
      free[p] = 1;
      if(free[next[p]] == 1)  // 合并两个连续的空闲空间
      {
        size[p] += size[next[p]];
        next[p] = next[next[p]];
      }
    }
    p = next[p];
  }
  // p = st;
  // sprint("\n\nafter the free!\n");
  // while(p != -1)
  // {
  //   sprint("idx = %d\nst_adr = %x\nsize = %d\nfree = %d\n--------------\n", p, st_addr[p], size[p], free[p]);
  //   p = next[p];
  // }
  p = st;
  int flag = 1;
  while(p != -1)
  {
    if(free[p] == 0) flag = 0;
    p = next[p];
  }
  if(flag == 1) user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
  return 0;
}



//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
    switch (a0) {
        case SYS_user_print:
            return sys_user_print((const char*)a1, a2);
        case SYS_user_exit:
            return sys_user_exit(a1);
        case SYS_user_allocate_page:
            // 支持更精细的内存分配
            return sys_user_allocate_page(a1); // a1 传入需要分配的大小
        case SYS_user_free_page:
            // 支持更精细的内存释放
            return sys_user_free_page(a1); // a1 传入需要释放的地址
        default:
            panic("Unknown syscall %ld \n", a0);
    }
}
