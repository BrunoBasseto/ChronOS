
#ifndef __CRONOS_CONFIG__
#define __CRONOS_CONFIG__

// ----------------------
// Configuração do Cronos
// ----------------------
#define CRONOS_TIMER             2   // Timer 2
#define MAX_PRIO                 3

// ------------------
// network interfaces
// ------------------
//#define _NETWORK
//#define _ENET
//#define _PPP

#define _ICMP
#define _DHCP
#define _DNS
#define _SMTP

// ------------------
// PPP configurations
// ------------------
//#define PPP_UART                      1       // UART to use
//#define PPP_FLOW_CONTROL                      // use hardware flow control
//#define PPP_MRU                       512     // maximum packet size

// -----------------
// TCP configuration
// -----------------
#define MSS                             512

// -----------------
// ARP configuration
//s -----------------
#define MAX_CACHE_ARP                   8

#endif
