#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bin.h"

#define SECTOR_HEADER_SIZE (24)
#define SECTOR_SIZE        (2048)
#define SECTOR_SIZE_RAW    (2352)

#define FILE_SIZE_MAX (131072)

static FILE* _bin_file = NULL;

void bin_init(void)
{
    _bin_file = fopen("../fft.bin", "rb");
    assert(_bin_file != NULL);
}

bool bin_is_loaded(void)
{
    return _bin_file != NULL;
}

void bin_shutdown(void)
{
    fclose(_bin_file);
    _bin_file = NULL;
}

// Forward declarations
static void _read_sector(int32_t sector_num, uint8_t out_sector[static SECTOR_SIZE]);

file_t read_file(int sector_num, int size)
{
    file_t file = { .size = size };
    file.data = calloc(1, size);

    int offset = 0;
    uint32_t occupied_sectors = ceil((double)size / (double)SECTOR_SIZE);

    for (uint32_t i = 0; i < occupied_sectors; i++) {
        uint8_t sector[SECTOR_SIZE];
        _read_sector(sector_num + i, sector);

        int remaining_size = size - offset;
        int bytes_to_copy = (remaining_size < SECTOR_SIZE) ? remaining_size : SECTOR_SIZE;

        memcpy(file.data + offset, sector, bytes_to_copy);
        offset += bytes_to_copy;
    }

    return file;
}

void read_bytes(file_t* f, int size, uint8_t* out_bytes)
{
    assert(size < FILE_SIZE_MAX);
    for (int i = 0; i < size; i++) {
        out_bytes[i] = read_u8(f);
    }
    return;
}

uint8_t read_u8(file_t* f)
{
    uint8_t value;
    memcpy(&value, &f->data[f->offset], sizeof(uint8_t));
    f->offset += sizeof(uint8_t);
    return value;
}

uint16_t read_u16(file_t* f)
{
    uint16_t value;
    memcpy(&value, &f->data[f->offset], sizeof(uint16_t));
    f->offset += sizeof(uint16_t);
    return value;
}

uint32_t read_u32(file_t* f)
{
    uint32_t value;
    memcpy(&value, &f->data[f->offset], sizeof(uint32_t));
    f->offset += sizeof(uint32_t);
    return value;
}

int8_t read_i8(file_t* f)
{
    int8_t value;
    memcpy(&value, &f->data[f->offset], sizeof(int8_t));
    f->offset += sizeof(int8_t);
    return value;
}

int16_t read_i16(file_t* f)
{
    int16_t value;
    memcpy(&value, &f->data[f->offset], sizeof(int16_t));
    f->offset += sizeof(int16_t);
    return value;
}

int32_t read_i32(file_t* f)
{
    int32_t value;
    memcpy(&value, &f->data[f->offset], sizeof(int32_t));
    f->offset += sizeof(int32_t);
    return value;
}

float read_f1x3x12(file_t* f)
{
    float value = read_i16(f);
    return value / 4096.0f;
}

static void _read_sector(int32_t sector_num, uint8_t out_sector[static SECTOR_SIZE])
{
    int32_t seek_to = (sector_num * SECTOR_SIZE_RAW) + SECTOR_HEADER_SIZE;
    if (fseek(_bin_file, seek_to, SEEK_SET) != 0) {
        assert(false);
    }

    size_t n = fread(out_sector, sizeof(uint8_t), SECTOR_SIZE, _bin_file);
    assert(n == SECTOR_SIZE);
    return;
}
