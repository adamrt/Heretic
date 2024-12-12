#pragma once

#include "cglm/types-struct.h"
#include "lighting.h"
#include "map.h"
#include "sokol_gfx.h"

#define GFX_DISPLAY_WIDTH  (640 * 3)
#define GFX_DISPLAY_HEIGHT (480 * 3)

typedef struct {
    vec3s translation;
    vec3s rotation;
    vec3s scale;

    vec3s centered_translation;
} transform_t;

// model_t represents a renderable model
typedef struct {
    sg_buffer vbuf;
    sg_buffer ibuf;
    sg_image texture;
    sg_image palette;
    transform_t transform;
    int vertex_count;
} model_t;

void gfx_init(void);
void gfx_shutdown(void);
void gfx_render_begin(void);
void gfx_render_background(vec4s, vec4s);
void gfx_render_model(model_t*, lighting_t*);
void gfx_render_end(void);
void gfx_scale_change(void);
model_t gfx_map_to_model(map_t*);

sg_sampler gfx_get_sampler(void);
int gfx_get_scale_divisor(void);
void gfx_set_scale_divisor(int);

typedef struct {
    sg_sampler sampler;

    struct {
        sg_pipeline pipeline;
        sg_image color_image;
        sg_image depth_image;
        sg_attachments attachments;
    } offscreen;

    struct {
        sg_pipeline pipeline;
        sg_bindings bindings;
    } background;

    struct {
        int width;
        int height;
        int scale_divisor;
        sg_pipeline pipeline;
        sg_bindings bindings;
    } display;
} gfx_t;
