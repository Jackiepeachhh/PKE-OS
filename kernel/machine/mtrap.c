#include "kernel/riscv.h"
#include "kernel/process.h"
#include "spike_interface/spike_utils.h"

#include "util/string.h"

static void handle_instruction_access_fault() { panic("Instruction access fault!"); }

static void handle_load_access_fault() { panic("Load access fault!"); }

static void handle_store_access_fault() { panic("Store/AMO access fault!"); }

static void handle_illegal_instruction() { panic("Illegal instruction!"); }

static void handle_misaligned_load() { panic("Misaligned Load!"); }

static void handle_misaligned_store() { panic("Misaligned AMO!"); }

// added @lab1_3
static void handle_timer() {
  int cpuid = 0;
  // setup the timer fired at next time (TIMER_INTERVAL from now)
  *(uint64*)CLINT_MTIMECMP(cpuid) = *(uint64*)CLINT_MTIMECMP(cpuid) + TIMER_INTERVAL;

  // setup a soft interrupt in sip (S-mode Interrupt Pending) to be handled in S-mode
  write_csr(sip, SIP_SSIP);
}

//
// handle_mtrap calls a handling function according to the type of a machine mode interrupt (trap).
//
void handle_mtrap() {
  uint64 mcause = read_csr(mcause);
  switch (mcause) {
    case CAUSE_MTIMER:
      handle_timer();
      break;
    case CAUSE_FETCH_ACCESS:
      handle_instruction_access_fault();
      break;
    case CAUSE_LOAD_ACCESS:
      handle_load_access_fault();
    case CAUSE_STORE_ACCESS:
      handle_store_access_fault();
      break;
    case CAUSE_ILLEGAL_INSTRUCTION:
      // TODO (lab1_2): call handle_illegal_instruction to implement illegal instruction
      // interception, and finish lab1_2.

      // errorline print
      {
        uint64 mepc = read_csr(mepc);
        for(int i = 0; i < current->line_ind; i ++)
          if(current->line[i].addr == mepc)
          {
            // read line、file、dir array and print
            int lnum = current->line[i].line;
            int findex = current->line[i].file;
            int dindex = current->file[findex].dir;
            sprint("Runtime error at %s/%s:%d\n", (char *)(current->dir)[dindex], current->file[findex].file, lnum);

            // open source code file
            char filename[100];
            strcpy(filename, (char *)(current->dir)[dindex]);
            filename[strlen((char *)(current->dir)[dindex])] = '/';
            strcpy(filename + strlen((char *)(current->dir)[dindex]) + 1, current->file[findex].file);
            spike_file_t* f = spike_file_open(filename, O_RDONLY, 0);

            // print source code file error line
            char source_code_string[1000];
            lnum --;
            spike_file_read(f, source_code_string, sizeof source_code_string);
            char * p = source_code_string;
            while(*(p ++))
            {
              if(*p == '\n' && lnum) lnum --;
              else if(lnum == 0)
              {
                while(*(p) != '\n') sprint("%c", *(p++));
                sprint("\n");
                break;
              }
            }
            break;
          }
        handle_illegal_instruction();
      }

      break;
    case CAUSE_MISALIGNED_LOAD:
      handle_misaligned_load();
      break;
    case CAUSE_MISALIGNED_STORE:
      handle_misaligned_store();
      break;

    default:
      sprint("machine trap(): unexpected mscause %p\n", mcause);
      sprint("            mepc=%p mtval=%p\n", read_csr(mepc), read_csr(mtval));
      panic( "unexpected exception happened in M-mode.\n" );
      break;
  }
}