/****************************************************************************
 * apps/testing/sig_sp_test/sig_sp_test_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test: Verify that modifying SP (REG_R13) in the saved register context
 * is honored on context restore (exception return path).
 *
 * Scenario (simulates runtime unwinding past a trampoline):
 *   1. Push values 1 and 2 onto the stack (simulating a trampoline push)
 *   2. Busy-wait for a timer signal (SIGALRM)
 *   3. In the handler, "emulate a pop" by advancing SP by 4
 *      (simulating a runtime deciding the top frame was a stub)
 *   4. Redirect PC to resume_after_signal
 *   5. After signal return, pop a value — it should be 1
 *
 * This exercises:
 *   - The SP relocation fix in arm_exception.S
 *   - The backward sliding copy (new SP > old SP, stack shrinks)
 *   - A realistic use case: unwinding past a trampoline frame
 *
 * Requirements:
 *   - Flat build only (accesses nxsched_self() for saved_regs)
 *   - ARM architecture (uses ARMv7-M register layout)
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <nuttx/arch.h>
#include <nuttx/sched.h>
#include <arch/irq.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile int g_result = -1;
static volatile int g_ready = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: verify_result
 *
 * Description:
 *   Called with the popped value in r0. Verifies it equals 1.
 ****************************************************************************/

void __attribute__((noinline, used)) verify_result(uint32_t value)
{
  printf("sig_sp_test: popped value = %lu (expected 1)\n",
         (unsigned long)value);

  if (value == 1)
    {
      printf("sig_sp_test: PASS\n");
      g_result = 0;
    }
  else
    {
      printf("sig_sp_test: FAIL - expected 1, got %lu\n",
             (unsigned long)value);
      g_result = 1;
    }

  exit(g_result);
}

/****************************************************************************
 * Name: resume_after_signal
 *
 * Description:
 *   We land here after the signal handler adjusts SP and PC.
 *   Stack now has [SP] = 1 (the '2' was skipped by SP += 4).
 *   Pop one value and verify it equals 1.
 ****************************************************************************/

static void __attribute__((naked, used)) resume_after_signal(void)
{
  __asm__ __volatile__(
    "pop  {r0}\n\t"
    "b    verify_result\n\t"
  );
}

/****************************************************************************
 * Name: sigalrm_handler
 *
 * Description:
 *   Called from the timer interrupt path (async signal delivery).
 *   Accesses saved_regs via nxsched_self() to modify the context
 *   that will be restored after signal return.
 ****************************************************************************/

static void sigalrm_handler(int signo, siginfo_t *info, void *ucontext)
{
  struct tcb_s *rtcb = nxsched_self();
  uint32_t *regs;

  (void)signo;
  (void)info;
  (void)ucontext;

  regs = rtcb->xcp.saved_regs;
  if (regs == NULL)
    {
      printf("sig_sp_test: ERROR - saved_regs is NULL\n");
      exit(2);
    }

  printf("sig_sp_test: handler - PC=0x%08lx SP=0x%08lx\n",
         (unsigned long)regs[REG_R15],
         (unsigned long)regs[REG_R13]);

  /* Only act when the main code is ready (in the asm loop) */

  if (!g_ready)
    {
      alarm(1);
      return;
    }

  /* Emulate a pop: advance SP by 4 (skip top-of-stack value '2') */

  regs[REG_R13] += 4;

  /* Redirect execution to resume_after_signal */

  regs[REG_R15] = ((uint32_t)(uintptr_t)resume_after_signal) & ~1u;
  regs[REG_XPSR] |= (1 << 24);

  printf("sig_sp_test: handler - new SP=0x%08lx PC=0x%08lx\n",
         (unsigned long)regs[REG_R13],
         (unsigned long)regs[REG_R15]);
}

/****************************************************************************
 * Name: wait_with_values_on_stack
 *
 * Description:
 *   Push 1 and 2 on the stack, set g_ready, then loop forever.
 *   The alarm handler will redirect us out of the loop.
 *   Stack after pushes: [SP] = 2, [SP+4] = 1
 ****************************************************************************/

static void __attribute__((naked, used)) wait_with_values_on_stack(void)
{
  __asm__ __volatile__(
    "mov  r0, #1\n\t"
    "push {r0}\n\t"
    "mov  r0, #2\n\t"
    "push {r0}\n\t"
    "ldr  r0, =g_ready\n\t"
    "mov  r1, #1\n\t"
    "str  r1, [r0]\n\t"
    "1: nop\n\t"
    "b    1b\n\t"
  );
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * sig_sp_test_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct sigaction sa;

  (void)argc;
  (void)argv;

  printf("sig_sp_test: Signal SP restore test\n");
  printf("sig_sp_test: push 1, push 2, alarm, handler SP+=4, pop => 1\n");

  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = sigalrm_handler;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGALRM, &sa, NULL) < 0)
    {
      printf("sig_sp_test: ERROR sigaction failed\n");
      return 1;
    }

  alarm(1);
  wait_with_values_on_stack();

  /* Should never reach here */

  printf("sig_sp_test: ERROR - returned from wait\n");
  return 1;
}
