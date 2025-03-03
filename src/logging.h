#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifndef DEBUG_EN
#define DEBUG_EN 1
#endif

#if DEBUG_EN
#define DEBUG(...) { fprintf(stderr, GRAY "%s\t%s\t%d: " RESET, __FILE__, __func__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }
#else
#define DEBUG(...) {}
#endif

#define ERROR(...) { fprintf(stderr, RED "error: " RESET); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }
// Macro que formatea y simplifica la impresión de errno
#define ERRSYS(name) { fprintf(stderr, BOLD RED "%s→" name "(): " RESET RED "%s\n" RESET,  __func__, strerror(errno)); }

#endif
