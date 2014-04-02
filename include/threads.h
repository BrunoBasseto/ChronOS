
/**
 * @file kernel.h
 * @brief Definições internas do sistema cronOS.
 */

#ifndef __KERNEL__
#define __KERNEL__

//                                76543210
#define MASK_WAIT               0b00011110
#define MASK_TIMEOUT            0b00001100
#define MASK_TERMINATE          0b11000000

// ------------------------
// informações do escalador
// ------------------------
extern volatile thread_t *_threads[MAX_PRIO];
extern uint16_t _thrd;
extern volatile thread_t *_thrp;
#endif
