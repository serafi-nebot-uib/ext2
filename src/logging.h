/**************************************************************************
* FILENAME: logging.h
* DESCRIPTION: Contiene la definición de múltiples MACROs, estas se 
               utilizan para la impresión de los mensajes de DEBUG
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#ifndef __LOGGING_H__
#define __LOGGING_H__

// #define DEBUG_LVL  0
//
// #define DEBUG(lvl, ...) { \
//     if (DEBUG_LVL >= (lvl)) { \
//         fprintf(stderr, GRAY "%s\t%s\t%d: " RESET, __FILE__, __func__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); \
//         fprintf(stderr, __VA_ARGS__); \
//         fprintf(stderr, "\n"); \
//     } \
// }

#ifndef DEBUG_EN
#define DEBUG_EN 0
#endif

#ifndef DEBUG_FILE_NAME
#define DEBUG_FILE_NAME 0
#endif

#if DEBUG_EN
#if DEBUG_FILE_NAME
#define DEBUG(...) { fprintf(stderr, GRAY "%s\t%s\t%d: " RESET, __FILE__, __func__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }
#else
#define DEBUG(...) { fprintf(stdout, GRAY "%s\t%d: " RESET, __func__, __LINE__); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); }
#endif
#else
#define DEBUG(...) {}
#endif

#define ERROR(...) { fprintf(stderr, RED "error: " RESET); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }
// macro que formatea y simplifica la impresión de errno
#define ERRSYS(name) { fprintf(stderr, BOLD RED "%s→" name "(): " RESET RED "%s\n" RESET,  __func__, strerror(errno)); }

// devuelve un string con el valor binario de n, se usa en los mensajes de debug para imprimir un número en formato binario
#define BIN_STR8(n) ({ \
    unsigned char num = (n) & 0xFF; \
    char binario[9] = "\0"; \
    for (int i = 0; i < 8; i++) binario[i] = (num & (1 << (7 - i))) ? '1' : '0'; \
    binario;\
})

#endif
