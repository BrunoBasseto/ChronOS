/**
 * @file threads.c
 * @brief Thread management.
 *
 * @author Bruno Basseto (bruno@wise-ware.org)
 * @version 4
 */

/********************************************************************************
 ********************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 1995-2014 Bruno Basseto.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ********************************************************************************
 ********************************************************************************/

#include <stdlib.h>
#include "chronos.h"
#include "config.h"
#include "threads.h"

/*
 * Global fields for exchanging information between C and assembler.
 */
volatile word_t _new_sp;
volatile word_t _old_sp;

/**
 * Threads list.
 */
volatile thread_t *_threads[MAX_PRIO];

volatile thread_t *_thrp;                                ///< Current thread.
volatile word_t _main_sp;                                ///< Main thread stack pointer backup.


/**
 * Change the stack-poiter to return to the thread defined by _new_sp.
 * @note Assembly function (inline).
 */
void switch_threads(void);

/*
 * Save registers under the stack.
 */
_asm("switch_threads:");
_asm("di");
_asm("sw $ra, -4($sp)");
_asm("sw $s7, -8($sp)");
_asm("sw $s6, -12($sp)");
_asm("sw $s5, -16($sp)");
_asm("sw $s4, -20($sp)");
_asm("sw $s3, -24($sp)");
_asm("sw $s2, -28($sp)");
_asm("sw $s1, -32($sp)");
_asm("sw $s0, -36($sp)");
_asm("sw $fp, -40($sp)");

/*
 * Setup new stack-pointer.
 */
_asm("lw $sp, _new_sp");

/*
 * Recover registers belonging to the desired thread.
 */
_asm("lw $ra, -4($sp)");
_asm("lw $s7, -8($sp)");
_asm("lw $s6, -12($sp)");
_asm("lw $s5, -16($sp)");
_asm("lw $s4, -20($sp)");
_asm("lw $s3, -24($sp)");
_asm("lw $s2, -28($sp)");
_asm("lw $s1, -32($sp)");
_asm("lw $s0, -36($sp)");
_asm("lw $fp, -40($sp)");
   
/*
 * Returns - processor will go to another thread.
 */
_asm("ei");
_asm("j $ra");
_asm("ei");


/**
 * Creates a new thread.
 * @param thr Thread function entry point.
 * @param stack_size Stack size in byte for the new thread.
 * @return Thread identifier.
 */
thread_t*
thread_create
   (void (*thr)(void), 
   uint16_t stack_size)
{
   thread_t *p;
   _uint32_t addr;
   uint16_t i;
   byte_t *sp;

   /*
    * Create a new thread ID.
    */
   p = malloc(sizeof(thread_t));
   if(p == NULL) return NULL;

   /*
    * Allocate thread stack.
    */
   sp = (byte_t *)malloc(stack_size + 4);
   if(sp == NULL) return NULL;
   memset(sp, 0, stack_size + 4);
   p->sp0 = (uint32_t)sp;
   stack_size &= 0xfffc;
   sp = sp + stack_size;
   sp = (byte_t*)((uint32_t)sp & 0xfffffffc);

   /*
    * Setup thread.
    * Load initial address into the stack.
    */
   disable();
   p->sp = (uint32_t)sp;
   sp--;
   addr.d = (uint32_t)thr;
   *sp-- = addr.b[3];
   *sp-- = addr.b[2];
   *sp-- = addr.b[1];
   *sp-- = addr.b[0];
   
   p->flags = 0;
   list_add(&_threads[0], p);                      // uses the lowest priority at first.
   enable();
   return p;
}

/**
 * Scheduler entry point.
 * Must be called by the main loop.
 */
void 
scheduler
   (void)
{
   int i;
   uint8_t p;
   static void (*c)(void *);
   callback_t *cb;

   /*
    * 1. Execute callbacks.
    */
   disable();
tenta:
   list_for_each(_callbacks, cb) {
      if(cb->timer == 0) {
         /*
          * Callback is ready to be called.
          */
         c = cb->function;
         enable();
         c(cb->param);
         disable();
         free(cb);
         list_remove(&_callbacks, cb);
         goto tenta;
      }
   }
   
   /*
    * 2. Look for the next thread.
    */
   for(i = MAX_PRIO-1; i >= 0; i--) {
      list_for_each(_threads[i], _thrp) {
         if(_thrp->f_nice) continue;                        // other threads allowed to come in.
         if((_thrp->flags & MASK_WAIT) == 0)                // ready?
            goto ready;
      }
      list_for_each(_threads[i], _thrp) {                   // all threads serviced, clears f_nice.
         _thrp->f_nice = FALSE;
      }
   }
   
   /*
    * No threads.
    */
   _thrp = NULL;
   enable();
   return;

ready:
   /*
    * Switch to the chosen thread.
    */
   disable();
   _asm("sw $sp, %0" : "=m"(_main_sp));
   _new_sp = _thrp->sp;
   switch_threads();
}

/**
 * Kernel services entry-point.
 * This function may not return immediately to the calling thread, if it gets suspended.
 * In this case, it returns to main therad.
 * @param func Kernel function code.
 * @param arg Kernel function parameter.
 * @return FALSE in case of failure or timeout.
 */
bool_t 
kernel_call
   (uint16_t func, 
   word_t arg)
{
   if(_thrp == NULL) return FALSE;                // thread main() cannot ask for kernel services.

   /*
    * Identify kernel function.
    */
   disable();
   switch(func) {
      /*
       * thread_yield
       */
      case SV_YIELD:
         _thrp->f_nice = TRUE;
         goto return_to_main;

      /*
       * thread_end
       */
      case SV_END:
         free((void*)_thrp->sp0);
         arg = _thrp->prio;
         free(_thrp);
         list_remove(&_threads[arg], _thrp);
         goto return_to_main;

      /*
       * thread_sleep
       */
      case SV_SLEEP:
         _thrp->timer = arg;
         _thrp->f_time_pending = TRUE;
         goto return_to_main;

      /*
       * thread_wait
       */
      case SV_WAIT:
         _thrp->f_waiting = TRUE;
         _thrp->data = arg;
         goto return_to_main;

      /*
       * thread_set_timeout
       */
      case SV_SETTIMEOUT:
         _thrp->timer = arg;
         _thrp->f_timeout = FALSE;
         goto return_to_thread;

      /*
       * thread_lock
       */
      case SV_LOCK:
         /*
          * Check mutex.
          */
         if(*(byte_t *)arg) {
            /*
             * Already locked, suspend thread.
             */
            _thrp->f_semaphore = TRUE;
            _thrp->data = arg;
            goto return_to_main;
         }
            
         /*
          * Lock and return to thread.
          */
         *(byte_t *)arg = 1;
         goto return_to_thread_no_timeout;
   }

return_to_thread_no_timeout:
   if(!_thrp->f_time_pending)
      _thrp->timer = 0;                      // cancels timeout checking

return_to_thread:
   enable();
   return TRUE;

return_to_main:
   /*
    * Save current thread stack pointer.
    */
   _asm("sw $sp, %0" : "=m"(_old_sp));
   _thrp->sp = _old_sp;

   /*
    * Retorns to thread main().
    */
   _thrp = NULL;
   _new_sp = _main_sp;
   switch_threads();

   /*
    * When a thread is going to be executed again we came from here.
    */
   if(_thrp == NULL) return FALSE;
   if(_thrp->f_timeout) return FALSE;
   return TRUE;
}
