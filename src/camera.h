#pragma once

#include <stdbool.h>

#include "cglm/types-struct.h"

#include "defines.h"

#define CAMERA_DIST_MIN (0.01f)
#define CAMERA_DIST_MAX (1000.0f)

typedef enum {
    TRANS_DIR_LEFT,
    TRANS_DIR_RIGHT,
    TRANS_DIR_UP,
    TRANS_DIR_DOWN,
} transition_dir_e;

typedef struct {
    vec3s start_position;
    vec3s end_position;

    f32 start_azimuth;
    f32 end_azimuth;

    f32 start_elevation;
    f32 end_elevation;

    f32 current_frame;
    f32 total_frames;

    bool valid;
} transition_t;

typedef struct {
    vec3s position;
    vec3s target;
    f32 azimuth, elevation; // In degrees
    f32 distance;
    bool use_perspective;
    transition_t transition;
} camera_t;

void camera_init(void);
void camera_update(void);
mat4s camera_get_view(void);
mat4s camera_get_proj(void);

void camera_mouse_movement(f32, f32);
void camera_mouse_wheel(f32);
void camera_key_left(void);
void camera_key_right(void);
void camera_key_up(void);
void camera_key_down(void);

camera_t* camera_get_internals(void);

// These are listed clockwise to match the FFT data storage. We might change
// later since we use counter-clockwise for the camera since we are RHS. These
// values match the polygon visibility data layout (unless its backwards).
typedef enum {
    CARDINAL_SW = 0x2,
    CARDINAL_NW = 0x3,
    CARDINAL_NE = 0x4,
    CARDINAL_SE = 0x5,
    CARDINAL_SSW = 0x6,
    CARDINAL_WSW = 0x7,
    CARDINAL_WNW = 0x8,
    CARDINAL_NNW = 0x9,
    CARDINAL_NNE = 0xA,
    CARDINAL_ENE = 0xB,
    CARDINAL_ESE = 0xC,
    CARDINAL_SSE = 0xD,

    // These are our own values just to represent all directions.
    CARDINAL_S = 0xF0,
    CARDINAL_W = 0xF1,
    CARDINAL_N = 0xF2,
    CARDINAL_E = 0xF3,
    CARDINAL_UNKNOWN = 0x0,
} cardinal_e;

cardinal_e camera_cardinal(void);
const char* camera_cardinal_str(void);
