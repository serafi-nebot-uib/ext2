/**************************************************************************
* FILENAME: logging.h
* DESCRIPTION: Contiene la definición de múltiples macros, estas se
               utilizan para la impresión de los mensajes de DEBUG
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#ifndef __LOGGING_H__
#define __LOGGING_H__

#include "colors.h"

#ifndef DEBUG_OUTPUT
#define DEBUG_OUTPUT stderr
#endif

#ifndef DEBUG_LVL
#define DEBUG_LVL 1
#endif

#if DEBUG_LVL == 0
#define DEBUG(lvl, ...) ({})
#define DEBUG_RAW(lvl, ...) ({})
#else
#define DEBUG_RAW(lvl, ...) ({                                                  \
    do {                                                                        \
        if (lvl <= DEBUG_LVL) {                                                 \
            fprintf(DEBUG_OUTPUT, __VA_ARGS__);                                 \
        }                                                                       \
    } while (0);                                                                \
})

#define DEBUG(lvl, ...) ({                                                      \
    do {                                                                        \
        if (lvl <= DEBUG_LVL) {                                                 \
            fprintf(DEBUG_OUTPUT, GRAY "%s\t%d: " RESET, __func__, __LINE__);   \
            fprintf(DEBUG_OUTPUT, __VA_ARGS__);                                 \
            fprintf(DEBUG_OUTPUT, "\n");                                        \
        }                                                                       \
    } while (0);                                                                \
})
#endif

#define ERROR(...) ({                                                           \
    do {                                                                        \
        fprintf(stderr, RED "error: " RESET);                                   \
        fprintf(stderr, __VA_ARGS__);                                           \
        fprintf(stderr, "\n");                                                  \
    } while (0);                                                                \
})

#define ERRSYS(name) ({                                                         \
    do {                                                                        \
        fprintf(stderr, BOLD RED "%s→" name "(): " RESET RED "%s\n" RESET,      \
                __func__, strerror(errno));                                     \
    } while (0);                                                                \
})

// devuelve un string con el valor binario de n, se usa en los mensajes de debug para imprimir un número en formato binario
#define BIN_STR8(n) ({ \
        unsigned char num = (n) & 0xFF; \
        char binario[9] = "\0"; \
        for (int i = 0; i < 8; i++) binario[i] = (num & (1 << (7 - i))) ? '1' : '0'; \
        binario; \
    })

#endif
