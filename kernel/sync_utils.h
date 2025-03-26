#ifndef _SYNC_UTILS_H_
#define _SYNC_UTILS_H_

static inline void sync_barrier(volatile int *counter, int all) {

  int local;

  asm volatile("amoadd.w %0, %2, (%1)\n"
               : "=r"(local)
               : "r"(counter), "r"(1)
               : "memory");

  if (local + 1 < all) {
    do {
      asm volatile("lw %0, (%1)\n" : "=r"(local) : "r"(counter) : "memory");
    } while (local < all);
  }
}

static inline void sync_acquirelock(volatile int *lock)
 {
   int local;  // 0 unlock 1 lock
   do {
     asm volatile("amoswap.w %0, %2, (%1)\n"
               : "=r"(local)
               : "r"(lock), "r"(1)
               : "memory");
   }while(local != 0);  // if locked by other thread then keep acquiring
   
 }
 
 static inline void sync_releaselock(volatile int *lock)
 {
   int local;
   asm volatile("amoswap.w %0, %2, (%1)\n"
               : "=r"(local)
               : "r"(lock), "r"(0)
               : "memory");
 }

#endif