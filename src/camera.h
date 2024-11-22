#ifndef CAMERA_H_
#define CAMERA_H_

#include "cglm/struct.h"

#define CAM_MIN_DIST  (0.01f)
#define CAM_MAX_DIST  (1000.0f)
#define CAM_MIN_ZNEAR (0.01f)
#define CAM_MAX_ZFAR  (1000.0f)

typedef struct {
    mat4s proj_mat, view_mat;
    vec3s eye, target;
    float azimuth, elevation;
    float znear, zfar;
    float distance;

    float pos_degrees;

    bool use_perspective;
} camera_t;

void camera_init(void);
void camera_update(void);
void camera_orbit(float, float);
void camera_zoom(float);

void camera_left(void);
void camera_right(void);

#endif // CAMERA_H_
