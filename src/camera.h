#ifndef CAMERA_H_
#define CAMERA_H_

#include <stdbool.h>

#include "cglm/types-struct.h"

#define CAMERA_DIST_MIN  (0.01f)
#define CAMERA_DIST_MAX  (1000.0f)
#define CAMERA_ZNEAR_MIN (0.01f)
#define CAMERA_ZFAR_MAX  (1000.0f)

typedef enum {
    TRANS_DIR_LEFT,
    TRANS_DIR_RIGHT,
    TRANS_DIR_UP,
    TRANS_DIR_DOWN,
} transition_dir_e;

typedef struct {
    transition_dir_e direction;

    float start_degrees;
    float end_degrees;

    float current_frame;
    float total_frames;

    bool valid;
} transition_t;

typedef struct {
    mat4s proj_mat, view_mat;
    vec3s eye, target;
    float azimuth, elevation; // In degrees
    float znear, zfar;
    float distance;
    bool use_perspective;
    transition_t transition;
} camera_t;

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

void camera_init(void);
void camera_update(void);

void camera_orbit(float, float);
void camera_zoom(float);
void camera_left(void);
void camera_right(void);
void camera_up(void);
void camera_down(void);

mat4s camera_get_view(void);
mat4s camera_get_proj(void);

// This is a helper function to get the internal camera struct. This is strictly
// used for the GUI.
camera_t* camera_get_internals(void);

cardinal_e camera_cardinal(void);
const char* camera_cardinal_str(void);

#endif // CAMERA_H_
