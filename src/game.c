#include "sokol_gfx.h"

#include "cglm/struct.h"
#include "shader.glsl.h"

#include "bin.h"
#include "camera.h"
#include "game.h"
#include "gfx.h"
#include "gui.h"

game_t g = {
    .scene = {
        .center_model = true,
        .current_scenario = 1,
    },
    .camera = {
        .zoom_factor = 256.0f,
        .znear = 0.001f,
        .zfar = 1000.0f,
    },
};

// Forward declarations
static void state_update(void);
static void scenario_prev(void);
static void scenario_next(void);
static void map_load(int num, resource_key_t);
static void map_unload(void);

void game_init(void)
{
    g.bin = fopen("/Users/adam/sync/emu/fft.bin", "rb");
    assert(g.bin != NULL);

    bin_load_global_data();

    // Initialize sub systems
    gfx_init();
    camera_init();
    gui_init();

    game_load_scenario(g.scene.current_scenario);
}

void game_input(const sapp_event* event)
{
    bool handled_by_ui = gui_input(event);
    bool is_mouse_event = event->type == SAPP_EVENTTYPE_MOUSE_MOVE
        || event->type == SAPP_EVENTTYPE_MOUSE_SCROLL
        || event->type == SAPP_EVENTTYPE_MOUSE_DOWN
        || event->type == SAPP_EVENTTYPE_MOUSE_UP;

    if (handled_by_ui && is_mouse_event) {
        return;
    }

    switch (event->type) {
    case SAPP_EVENTTYPE_KEY_DOWN:
        switch (event->key_code) {
        case SAPP_KEYCODE_ESCAPE:
            sapp_request_quit();
        case SAPP_KEYCODE_K:
            scenario_next();
            break;
        case SAPP_KEYCODE_J:
            scenario_prev();
            break;
        default:
            break;
        }

    case SAPP_EVENTTYPE_MOUSE_DOWN:
    case SAPP_EVENTTYPE_MOUSE_UP: {
        bool is_down = (event->type == SAPP_EVENTTYPE_MOUSE_DOWN);
        if (event->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
            sapp_lock_mouse(is_down);
            g.input.mouse_left = is_down;
        }
        if (event->mouse_button == SAPP_MOUSEBUTTON_RIGHT) {
            sapp_lock_mouse(is_down);
            g.input.mouse_right = is_down;
        }
        break;
    }

    case SAPP_EVENTTYPE_MOUSE_MOVE:
        if (g.input.mouse_left) {
            camera_orbit(event->mouse_dx, event->mouse_dy);
        }
        break;

    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        camera_zoom(event->scroll_y);
        break;

    default:
        break;
    }
}

void game_update(void)
{
    state_update();
    camera_update();
    gfx_update();
}

void game_load_scenario(int num)
{
    scenario_t scenario = g.fft.scenarios[num];
    resource_key_t scenario_key = { .time = scenario.time, .weather = scenario.weather, .layout = 0 };
    map_load(scenario.map_id, scenario_key);
}

void game_shutdown(void)
{
    fclose(g.bin);
    map_unload();

    bin_free_global_data();

    gui_shutdown();
    gfx_shutdown();
}

static void state_update(void)
{
    model_t* model = &g.scene.model;
    if (g.scene.center_model) {
        model->transform.translation = model->centered_translation;
    } else {
        model->transform.translation = (vec3s) { { 0.0f, 0.0f, 0.0f } };
    }

    mat4s model_matrix = glms_mat4_identity();
    model_matrix = glms_translate(model_matrix, model->transform.translation);
    model_matrix = glms_rotate_x(model_matrix, model->transform.rotation.x);
    model_matrix = glms_rotate_y(model_matrix, model->transform.rotation.y);
    model_matrix = glms_rotate_z(model_matrix, model->transform.rotation.z);
    model_matrix = glms_scale(model_matrix, model->transform.scale);

    model->model_matrix = model_matrix;
}

static void scenario_next(void)
{
    g.scene.current_scenario++;
    game_load_scenario(g.scene.current_scenario);
}

static void scenario_prev(void)
{
    g.scene.current_scenario--;
    game_load_scenario(g.scene.current_scenario);
}

static void map_load(int num, resource_key_t resource_key)
{
    map_unload();

    model_t model = read_scenario(num, resource_key);

    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc) {
        .data = SG_RANGE(model.mesh.geometry.vertices),
        .label = "mesh-vertices",
    });

    sg_image texture = sg_make_image(&(sg_image_desc) {
        .width = TEXTURE_WIDTH,
        .height = TEXTURE_HEIGHT,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.subimage[0][0] = SG_RANGE(model.mesh.texture.data),
    });

    sg_image palette = sg_make_image(&(sg_image_desc) {
        .width = PALETTE_WIDTH,
        .height = PALETTE_HEIGHT,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .data.subimage[0][0] = SG_RANGE(model.mesh.palette.data),
    });

    model.texture = texture;
    model.palette = palette;
    model.vbuffer = vbuf;

    model.bindings = (sg_bindings) {
        .vertex_buffers[0] = vbuf,
        .fs = {
            .images[SLOT_u_texture] = texture,
            .images[SLOT_u_palette] = palette,
            .samplers[SLOT_u_sampler] = gfx.sampler,
        },
    };

    g.scene.model = model;
}

static void map_unload(void)
{
    sg_destroy_image(g.scene.model.texture);
    sg_destroy_image(g.scene.model.palette);
    sg_destroy_buffer(g.scene.model.vbuffer);
}
