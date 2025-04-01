/**************************************************************************
* FILENAME: helper.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "helper.h"

void hexdump_raw(const void * const buff, size_t offset, const size_t size, const uint32_t addr_start, const uint16_t col_cnt, const uint16_t col_sep, FILE *file) {
    size_t idx = offset;
    size_t idx_end = offset + size;
    uint8_t * const b = (uint8_t *) buff;
    const uint32_t addr_off = addr_start % col_cnt;
    const uint32_t addr_base = addr_start - addr_off;
    const uint32_t addr_end = addr_start + size;
    uint32_t addr = addr_base;
    char * const str = (char *) malloc(col_cnt + 1);
    if (str == NULL) return;
    str[col_cnt] = 0;

    while (addr < addr_end) {
        fprintf(file, "%08x  ", addr);
        for (uint32_t i = 0; i < col_cnt; i++) {
            if (i % col_sep == 0) fprintf(file, " ");
            if ((addr == addr_base && i < addr_off) || idx >= idx_end) {
                fprintf(file, "   ");
                str[i] = '.';
            } else {
                fprintf(file, "%02x ", b[idx]);
                str[i] = isprint(b[idx]) ? b[idx] : '.';
                idx++;
            }
        }
        fprintf(file, " ▏%s▕\n", str);
        addr += col_cnt;
    }

    free(str);
}

void hexdump_col(const void * const buff, size_t offset, const size_t size, const uint32_t addr_start, const uint16_t col_cnt, const uint16_t col_sep) {
    hexdump_raw(buff, offset, size, addr_start, col_cnt, col_sep, stdout);
}

void hexdump(const void * const buff, size_t offset, const size_t size, const uint32_t addr_start) {
    hexdump_raw(buff, offset, size, addr_start, 16, 8, stdout);
}
