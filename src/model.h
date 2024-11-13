#ifndef MODEL_H_
#define MODEL_H_

#include "cglm/struct.h"
#include "sokol_gfx.h"

// GLOBAL_SCALE is used to scale the vertex data. This might be used later to
// scale camera movement and other positional related instruction data.
//
// We could use the original i16 data and pass those to OpenGL and let it
// normalize, but OpenGL will do it based on the full range of i16, not the max
// value of our vertex data. So we just pic a number here that is reasonable and
// use it everywhere.
#define GLOBAL_SCALE (256.0f)

#define MESH_MAX_VERTICES (7620)
#define MESH_MAX_LIGHTS (3)

#define SCENE_MAX_MODELS (125)

#define TEXTURE_WIDTH (256)
#define TEXTURE_HEIGHT (1024)
#define TEXTURE_SIZE (TEXTURE_WIDTH * TEXTURE_HEIGHT) // 262144
#define TEXTURE_BYTE_SIZE (TEXTURE_SIZE * 4)

#define PALETTE_WIDTH (256)
#define PALETTE_HEIGHT (1)
#define PALETTE_SIZE (PALETTE_WIDTH * PALETTE_HEIGHT)
#define PALETTE_BYTE_SIZE (PALETTE_SIZE * 4)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct {
    vec3s position;
    vec3s normal;
    vec2s uv;
    float palette_index;
    float is_textured;
} vertex_t;

typedef struct {
    vertex_t vertices[MESH_MAX_VERTICES];
    int count;
    bool valid;
} geometry_t;

typedef struct {
    uint8_t data[TEXTURE_BYTE_SIZE];
} texture_t;

typedef struct {
    uint8_t data[PALETTE_BYTE_SIZE];
    bool valid;
} palette_t;

typedef struct {
    vec3s direction;
    vec4s color;
    bool valid;
} light_t;

typedef struct {
    light_t lights[MESH_MAX_LIGHTS];
    vec4s ambient_color;
    float ambient_strength;
    vec4s bg_top;
    vec4s bg_bottom;

    bool valid;
} lighting_t;

typedef struct {
    geometry_t geometry;
    texture_t texture;
    palette_t palette;
    lighting_t lighting;

    bool valid;
} mesh_t;

typedef struct {
    vec3s translation;
    vec3s rotation;
    vec3s scale;
} transform_t;

typedef struct {
    mesh_t mesh;

    transform_t transform;
    mat4s model_matrix;         // computed from transform
    vec3s centered_translation; // computed from geometry

    sg_image texture;
    sg_image palette;
    sg_buffer vbuffer;
    sg_bindings bindings;
} model_t;

typedef struct {
    model_t model;
    bool center_model;
    int current_map;
} scene_t;

vec3s geometry_centered_translation(geometry_t* geometry);
vec3s geometry_normalized_scale(geometry_t* geometry);

#endif // MODEL_H_
