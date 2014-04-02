/**
 * @file chronos.c
 * @brief Kernel services and helper functions.
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
#include <string.h>
#include <limits.h>
#include "chronos.h"
#include "config.h"
#include <mx7/interrupt.h>
#include "threads.h"
#include <mx7/sfr.h>
#include "timer.h"

/**
 * List of pending callback functions.
 */
volatile callback_t *_callbacks;

/**
 * System tick counter.
 */
volatile uint32_t ticks;

/**
 * Wait a number of processor cycles.
 * @param cycles Number of clock cycles to wait.
 */
void delay(uint32_t cycles)
{
   _asm("lw $k1, %0" :: "m"(cycles));
   _asm("mfc0 $k0, $9, 0");                     // load COUNT register.
   _asm("addu $k1, $k0, $k1");                  // calculate final COUNT register value into $k1 = COUNT + cycles.
   _asm("_wait_here:");
   _asm("mfc0 $k0, $9, 0");                     // load COUNT register into $k0.
   _asm("sub $v0, $k1, $k0");
   _asm("bgtz $v0, _wait_here");                // wait while $k1 - COUNT >= 0
   _asm("nop");
}

/**
 * Setup the kernel.
 * Must be called (only) during system initialization.
 * @param pclock Peripheral clock frequency (for timer programming) in Hz.
 */
void 
kernel_init
   (uint32_t pclock)
{
   /*
    * Clean all data structures.
    */
   memset(_threads, 0, sizeof(_threads));
   _callbacks = NULL;
   _thrp = NULL;
   ticks = 0;

   /*
    * Configure CPU timer.
    */
   pclock /= 25600;
   INIT_TIMER(pclock);
}

/**
 * Forces a thread to terminate.
 * @param th Thread identifier.
 */
void 
thread_kill
   (thread_t *th)
{
   byte_t prio;
   
   /*
    * Remove thread.
    */
   disable();
   free((void *)(th->sp0));
   prio = th->prio;
   free(th);
   list_remove(&_threads[prio], th);
   enable();
}

/**
 * Asks a thread to terminate itself, setting the flag f_terminate.
 * @param th Thread identifier.
 */
void 
thread_terminate
   (thread_t *th)
{
   /*
    * Put thread into execution with f_terminate on.
    */
   disable();
   th->flags = (th->flags & (~MASK_WAIT)) | MASK_TERMINATE;
   enable();
}

/**
 * Suspends the execution of a thread.
 * @param th Thread identifier.
 */
void 
thread_suspend
   (thread_t *th)
{
   disable();
   th->f_suspend = TRUE;
   enable();
}

/**
 * Releases a thread suspended by thread_suspend().
 * @param th Thread identifier.
 */
void 
thread_release
   (thread_t *th)
{
   disable();
   th->f_suspend = FALSE;
   enable();
}

/**
 * Send a signal, allowing waiting threads to execute.
 * @param ptr Signal (any value).
 */
void 
thread_signal
   (void *ptr)
{
   int prio;
   thread_t *p;
   
   /*
    * Search for waiting threads.
    */
   disable();
   for(prio = MAX_PRIO-1; prio >= 0; prio--) { 
      list_for_each(_threads[prio], p) {
         if(p->f_waiting) {
            if(p->data == (word_t)ptr) {
               /*
                * Thread signaled: release it to execution.
                */
               p->f_waiting = FALSE;
               if(!p->f_time_pending) p->timer = 0;
            }
         }
      }
   }
   enable();
}

/**
 * Force a waiting thread to be released without signaling.
 * thread_wait() will return FALSE.
 * @param th Thread identifier.
 */
void 
thread_force
   (thread_t *th)
{
   disable();
   if(th->f_waiting) {
      /*
       * Release thread with error flag set.
       */
      th->f_waiting = FALSE;
      th->f_timeout = TRUE;
      if(!th->f_time_pending) th->timer = 0;
   }
   enable();
}

/**
 * Unlock a mutex previously locked by the thread, allowing other threads to access it.
 * @param ptr Pointer to the mutex.
 */
void 
thread_unlock
   (void *ptr)
{
   int prio;
   thread_t *p;

   disable();
   if(*(byte_t *)ptr == 0) {
      /*
       * Mutex already free.
       */
      enable();
      return;
   }

   /*
    * Look for other threads waiting for the mutex.
    */
   for(prio = MAX_PRIO-1; prio >= 0; prio--) { 
      list_for_each(_threads[prio], p) {
         if(p->f_semaphore) {
            if(p->data == (word_t)ptr) {
               /*
                * Release thread for execution, keeping mutex locked.
                */
               p->f_semaphore = FALSE;
               if(!p->f_time_pending) p->timer = 0;
               enable();
               return;
            }
         }
      }
   }
   
   /*
    * No more pending threads, unlock mutex.
    */
   *(byte_t *)ptr = 0;
   enable();
}

/**
 * Add a callback function for execution.
 * @param fn Callback function pointer.
 * @param par Parameter to the callback.
 * @param time Time before call (0 = immediate).
 */
void 
callback_fire
   (void *fn, 
   void *par, 
   word_t time)
{
   callback_t *novo;
   
   if(fn == NULL) return;
 
   novo = malloc(sizeof(callback_t));
   if(novo == NULL) return;
   
   novo->function = fn;
   novo->param = par;
   novo->timer = time; 

   disable();
   list_add(&_callbacks, novo);
   enable();
}

/**
 * Add a callback function for execution or change an existing callback parameters.
 * @param fn Callback function pointer.
 * @param par Parameter to the callback.
 * @param time Time before call (0 = immediate).
 */
void 
callback_refire
   (void *fn, 
   void *par, 
   word_t time)
{
   callback_t *p;
   
   if(fn == NULL) return;
   
   disable();
   
   /*
    * Search for the callback.
    */
   list_for_each(_callbacks, p) {
      if(p->function == fn) {
         /*
          * Found, change it.
          */
         p->timer = time;
         p->param = par;
         enable();
         return;
      }
   }

   /*
    * Not found.
    * Create a new one.
    */   
   p = malloc(sizeof(callback_t));
   if(p == NULL) return;
   
   p->function = fn;
   p->param = par;
   p->timer = time; 

   list_add(&_callbacks, p);
   enable();
}

/**
 * Cancels a callback execution.
 * @param fn Callback function pointer.
 */
void 
callback_cancel
   (void *fn)
{
   callback_t *p;
   
   if(fn == NULL) return;

   disable();
tenta:
   list_for_each(_callbacks, p) {
      if(p->function == fn) {
         free(p);
         list_remove(_callbacks, p);
         goto tenta;
      }
   }
   enable();
}

/**
 * Sets current thread prioriry.
 * @param prio Priority value: 0 = lowest, up to MAX_PRIO-1
 */
void 
thread_priority
   (uint8_t prio)
{
   int old;
   if(_thrp == NULL) return;
   if(prio > MAX_PRIO-1) prio = MAX_PRIO-1;
   if(_thrp->prio == prio) return;

   disable();
   old = _thrp->prio;
   _thrp->prio = prio;
   list_remove(&_threads[old], _thrp);
   list_add(&_threads[prio], _thrp);
   enable();
}

/**
 * Returns TRUE if the current thread is supposed to be terminated.
 */
bool_t
thread_terminated
   (void)
{
   if(_thrp == NULL) return FALSE;
   return (bool_t)_thrp->f_terminate;
}

/**
 * Returns TRUE as long as the current thread is allowed to execute.
 */
bool_t 
thread_not_terminated
   (void)
{
   if(_thrp == NULL) return TRUE;
   return (bool_t)!_thrp->f_terminate;
}

/**
 * Check if a thred is running.
 * @param th Thread identifier.
 * @return TRUE if the thread is running.
 */
bool_t 
thread_is_running
   (thread_t *th)
{
   thread_t *p;
   int i;
   
   /*
    * Search the thread ID.
    */
   for(i=0; i<MAX_PRIO; i++) {
      if(list_contains(_threads[i], th)) return TRUE;
   }
   return FALSE;
}

/**
 * Returns the current number of threads.
 */
uint16_t os_count_threads(void)
{
   register int i, n;
   for(i=0, n=0; i<MAX_PRIO; i++) n += list_length(_threads[i]);
   return n;
}

/**
 * Returns the current number of callbacks.
 */
uint16_t os_count_callbacks(void)
{
   return list_length(_callbacks);
}

/**
 * Returns the current number of running threads.
 */
uint16_t os_count_ready(void)
{
   register int i, n;
   thread_t *p;
   for(i=0, n=0; i<MAX_PRIO; i++) {
      list_for_each(_threads, p) {
         if((p->flags && MASK_WAIT) == 0) n++;
      }
   }
   return n;
}

/**
 * System tick interrupt. Manages timers.
 */
DECLARE_INTERRUPT(IRQ, os_tick);
void __interrupt
os_tick
   (void)
{
   word_t t;
   int i;
   thread_t *p;
   callback_t *c;

   ticks++;

   /*
    * Callback timming.
    */
   list_for_each(_callbacks, c) {
      if(c->timer) c->timer--;
   }

   /*
    * Thread timming.
    */
   for(i=0; i<MAX_PRIO; i++) {
      list_for_each(_threads[i], p) {
         t = p->timer;
         if(t) {
            t--;
            p->timer = t;
            if(t == 0) {
               if(p->flags & MASK_TIMEOUT) {
                  p->flags &= (~MASK_WAIT);
                  p->f_timeout = TRUE;
               }
               p->f_time_pending = FALSE;
            }
         }
      }   
   }

   /*
    * Clear interrupt.
    */
   CLEAR_IRQ();
}
