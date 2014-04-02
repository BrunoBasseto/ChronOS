/**
 * @file timer.h
 * @brief Declarações específicas para o processador PIC32 / compilador C32 para configuração do timer.
 */
   
#if CRONOS_TIMER == 2
#define IRQ                      8
#define TMRx                    TMR2
#define PRx                     PR2
#define TxIF                    IFS0bits.T2IF
#define TxIE                    IEC0bits.T2IE
#define TxIS                    IPC2bits.T2IS
#define TxIP                    IPC2bits.T2IP
#define TxON                    T2CONbits.TON
#define TxCON                   T2CON
#endif
#if CRONOS_TIMER == 4
#define IRQ                     8
#define TMRx                    TMR4
#define PRx                     PR4
#define TxIF                    IFS0bits.T4IF
#define TxIE                    IEC0bits.T4IE
#define TxIS                    IPC4bits.T4IS
#define TxIP                    IPC4bits.T4IP
#define TxON                    T4CONbits.TON
#define TxCON                   T4CON
#endif

/**
 * Macro para configuração do timer para base de tempo do sistema CronOS.
 */
#define INIT_TIMER(x)                           \
    TxCON = 0b0000000001110000;                 \
    TMRx = 0;                                   \
    PRx = x;                                    \
    TxIF = 0;                                   \
    TxIP = 2;                                   \
    TxIS = 3;                                   \
    TxIE = 1;                                   \
    TxON = 1;

/**
 * Limpa a interrupção do timer.
 */
#define CLEAR_IRQ()							         \
	TxIF = 0;
