/**
 * @file chronos.h
 * @brief Declarações referentes ao Sistema Executivo CronOS.
 */

#ifndef __CHRONOSH__
#define __CHRONOSH__

typedef enum {
   FALSE,
   TRUE
} bool_t;
#define byte_t unsigned char
#define int8_t char
#define uint8_t char

#include "list.h"

#define disable() asm volatile ("di");
#define enable()  asm volatile ("ei");
#define word_t unsigned int
#define uint32_t unsigned int
#define uint16_t unsigned short
#define int32_t int
#define int16_t short

#define __interrupt __attribute__((interrupt))

typedef union {
   uint32_t d;
   uint16_t w[2];
   byte_t b[4];
} _uint32_t;

#define bit(X) unsigned X: 1
#define _asm   asm
#define _PACKED __attribute__((packed))

#define LOW(x) ((x) & 0xff)
#define HIGH(x) ((x) >> 8)
#define WORDOF(x, y) (((uint16_t)(x) << 8) | ((uint16_t)(y) & 0x00ff))

#define LOWORD(x) ((uint16_t)((x) & 0xffff))
#define HIWORD(x) ((uint16_t)((x) >> 16))

#define set_bit(X, Y)	 X |= (1 << (Y))
#define clear_bit(X, Y)  X &= (~(1 << (Y)))
#define toggle_bit(X, Y) X ^= (1 << (Y)) 
#define test_bit(X, Y)  (X & (1 << (Y)))

#define buffer_t        byte_t *
#define string_t        char *

/**
 * estrutura de controle dos threads
 */
typedef struct {
   list_item_t list;                               ///< Lista ligada de threads.
   union {
      struct {
         bit(f_nice);                              ///< [bit 0] thread foi executado e liberou uso da CPU com yield
         bit(f_time_pending);                      ///< [bit 1] temporização ativa
         bit(f_waiting);                           ///< [bit 2] espera um sinal
         bit(f_semaphore);                         ///< [bit 3] espera um semáforo
         bit(f_suspend);                           ///< [bit 4] thread suspenso
         bit( );                                   ///  [bit 5] não utilizado
         bit(f_timeout);                           ///< [bit 6] valor de retorno (timeout)
         bit(f_terminate);                         ///< [bit 7] requisição de término
         unsigned prio:3;                          ///< [bits 8-10] prioridade do thread
      };
      uint16_t flags;
   };
   word_t data;                                    ///< Identificação de sinal ou semáforo
   word_t sp0;                                     ///< Valor inicial do stack-pointer
   word_t sp;                                      ///< Stack-pointer corrente
   word_t timer;                                   ///< contador de tempo de espera
} thread_t;

#define mutex_t            byte_t
extern volatile uint32_t ticks;

// --------
// Serviços
// --------
#define SV_YIELD                0
#define SV_SLEEP                1
#define SV_SETTIMEOUT           2
#define SV_WAIT                 3
#define SV_SIGNAL               4
#define SV_LOCK                 5
#define SV_UNLOCK               6
#define SV_END                  9

// ---------
// callbacks
// ---------
typedef struct {
   list_item_t list;
   void *function;                                 ///< Função a ser acionada.
   void *param;                                    ///< Parâmetro para a função.
   word_t timer;                                   ///< Tempo de acionamento.
} callback_t;
extern volatile callback_t *_callbacks;

#define unreachable()         for(;;)

// ----------
// cronos API
// ----------
void delay(uint32_t cycles);
void kernel_init(uint32_t pclock);
#define kernel_run()                for(;;) { scheduler(); }
thread_t *thread_create(void (*thr)(void), uint16_t stack_size);
void thread_priority(uint8_t prio);
void thread_kill(thread_t *th);
void thread_terminate(thread_t *th);
void thread_suspend(thread_t *th);
void thread_release(thread_t *th);
void callback_fire(void *fn, void *par, word_t time);
void callback_refire(void *fn, void *par, word_t time);
void callback_cancel(void *fn);
void scheduler(void);
void thread_signal(void *ptr);
void thread_force(thread_t *th);
void thread_unlock(void *ptr);
bool_t kernel_call(uint16_t func, word_t arg);
bool_t thread_not_terminated(void);
bool_t thread_is_running(thread_t *th);
#define thread_yield()              kernel_call(SV_YIELD, 0)
#define thread_sleep(X)             kernel_call(SV_SLEEP, X)
#define thread_set_timeout(X)       kernel_call(SV_SETTIMEOUT, X)
#define thread_wait(X)              kernel_call(SV_WAIT, (word_t)X)
#define thread_lock(X)              kernel_call(SV_LOCK, (word_t)X)
#define thread_end()                kernel_call(SV_END, 0)
#endif
