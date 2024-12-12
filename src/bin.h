#ifndef BIN_H_
#define BIN_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "defines.h"

// Simple buffer struct to allow incremental reading of different types.
// Typically useful for reading files from the original PSX bin.
typedef struct {
    u8* data;
    usize offset;
    usize size;
} buffer_t;

void bin_init(void);
void bin_shutdown(void);
bool bin_is_loaded(void);

buffer_t read_file(usize, usize);

void read_bytes(buffer_t*, usize, u8*);
u8 read_u8(buffer_t*);
u16 read_u16(buffer_t*);
u32 read_u32(buffer_t*);
i8 read_i8(buffer_t*);
i16 read_i16(buffer_t*);
i32 read_i32(buffer_t*);
f32 read_f1x3x12(buffer_t*);

buffer_t read_file_test_evt(void);
buffer_t read_file_attack_out(void);

#endif // BIN_H_
