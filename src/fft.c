#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bin.h"
#include "fft.h"
#include "game.h"

#define SECTOR_HEADER_SIZE (24)
#define SECTOR_SIZE        (2048)
#define SECTOR_SIZE_RAW    (2352)

#define GNS_FILE_MAX_SIZE (2388)

#define SCENARIO_DATA_OFFSET (0x10938)
#define SCENARIO_SIZE        (24)
#define SCENARIO_TOTAL_COUNT (488)

#define EVENT_FILE_SECTOR (3707)
#define EVENT_FILE_SIZE   (4096000)
#define EVENT_SIZE        (8192)
#define EVENT_COUNT       (500)

#define ATTACK_FILE_SECTOR (2448)
#define ATTACK_FILE_SIZE   (125956)

static vec3s read_position(file_t*);
static vec3s read_normal(file_t*);
static vec4s read_rgb8(file_t*);
static vec4s read_rgb15(file_t*);
static float read_light_color(file_t*);

static record_t read_record(file_t*);
static int read_records(file_t* f, record_t* out_records);

static geometry_t read_geometry(file_t*);
static texture_t read_texture(file_t*);
static palette_t read_palette(file_t*);
static lighting_t read_lighting(file_t*);

static mesh_t read_mesh(file_t*);
static void read_map_data(int num, map_data_t* map_data_out);

// Utility functions
static void merge_meshes(mesh_t*, mesh_t*);
static vec2s process_tex_coords(float u, float v, uint8_t page);
static vec3s vertices_centered(vertices_t* vertices);
vertices_t geometry_to_vertices(geometry_t*);

static void load_events(void);
static void load_scenarios(void);

void fft_init(void)
{
    g.bin = fopen("../fft.bin", "rb");
    assert(g.bin != NULL);
    g.bin_loaded = true;

    load_events();
    load_scenarios();
}

void fft_shutdown(void)
{
    free(g.fft.scenarios);
    free(g.fft.events);
    fclose(g.bin);
}

static void load_events(void)
{
    file_t event_file = read_file(g.bin, EVENT_FILE_SECTOR, EVENT_FILE_SIZE);
    event_t* events = calloc(EVENT_COUNT, sizeof(event_t));

    for (int i = 0; i < EVENT_COUNT; i++) {
        uint8_t bytes[EVENT_SIZE];
        read_bytes(&event_file, EVENT_SIZE, bytes);

        uint32_t text_offset = (uint32_t)((uint32_t)(bytes[0] & 0xFF)
            | ((uint32_t)(bytes[1] & 0xFF) << 8)
            | ((uint32_t)(bytes[2] & 0xFF) << 16)
            | ((uint32_t)(bytes[3] & 0xFF) << 24));

        events[i].valid = text_offset != 0xF2F2F2F2;

        if (events[i].valid) {
            int text_size = 8192 - text_offset;
            int code_size = text_offset - 4;

            memcpy(events[i].text, bytes + text_offset, text_size);
            memcpy(events[i].code, bytes + 4, code_size);
        }
    }

    g.fft.events = events;

    free(event_file.data);
}

static void load_scenarios(void)
{
    file_t attack_out_file = read_file(g.bin, ATTACK_FILE_SECTOR, ATTACK_FILE_SIZE);
    attack_out_file.offset = SCENARIO_DATA_OFFSET;

    int index = 0;

    scenario_t* scenarios = calloc(SCENARIO_USABLE_COUNT, sizeof(scenario_t));

    for (int i = 0; i < SCENARIO_TOTAL_COUNT; i++) {
        uint8_t bytes[SCENARIO_SIZE];
        read_bytes(&attack_out_file, SCENARIO_SIZE, bytes);

        int event_id = bytes[0] | (bytes[1] << 8);

        event_t event = g.fft.events[event_id];
        if (!event.valid) {
            continue;
        }

        // Use a different index because some scenarios are skipped.
        scenarios[index].id = event_id;
        scenarios[index].map_id = bytes[2];
        scenarios[index].weather = bytes[3];
        scenarios[index].time = bytes[4];
        scenarios[index].entd_id = bytes[7] | (bytes[8] << 8);
        scenarios[index].next_scenario_id = bytes[18] | (bytes[19] << 8);
        index++;
    }

    g.fft.scenarios = scenarios;

    free(attack_out_file.data);
}

void fft_read_map(int num, map_state_t map_state, map_t* map_out)
{
    map_data_t* map_data = calloc(1, sizeof(map_data_t));
    read_map_data(num, map_data);

    map_out->map_state = map_state;
    map_out->map_data = map_data;

    if (map_data->primary_mesh.valid) {
        map_out->mesh = map_data->primary_mesh;
    } else {
        map_out->mesh = map_data->override_mesh;
    }

    for (int i = 0; i < map_data->alt_mesh_count; i++) {
        mesh_t alt_mesh = map_data->alt_meshes[i];
        if (alt_mesh.valid && fft_map_state_eq(alt_mesh.map_state, map_state)) {
            merge_meshes(&map_out->mesh, &alt_mesh);
            break;
        }
    }

    for (int i = 0; i < map_data->texture_count; i++) {
        texture_t texture = map_data->textures[i];

        if (texture.valid && fft_map_state_eq(texture.map_state, map_state)) {
            map_out->texture = texture;
            break;
        }
        if (texture.valid && fft_map_state_default(texture.map_state)) {
            if (!map_out->texture.valid) {
                map_out->texture = texture;
            }
        }
    }

    map_out->vertices = geometry_to_vertices(&map_out->mesh.geometry);
    map_out->centered_translation = vertices_centered(&map_out->vertices);

    assert(map_out->mesh.valid);
    assert(map_out->texture.valid);
}

static geometry_t read_geometry(file_t* f)
{
    geometry_t geometry = { 0 };

    // 0x40 is always the location of the primary mesh pointer.
    // 0xC4 is always the primary mesh pointer.
    f->offset = 0x40;
    f->offset = read_u32(f);
    if (f->offset == 0) {
        return geometry;
    }

    // The number of each type of polygon.
    int N = read_u16(f); // Textured triangles
    int P = read_u16(f); // Textured quads
    int Q = read_u16(f); // Untextured triangles
    int R = read_u16(f); // Untextured quads

    // Validate maximum values
    assert(N < MESH_MAX_TEX_TRIS && P < MESH_MAX_TEX_QUADS && Q < MESH_MAX_UNTEX_TRIS && R < MESH_MAX_TEX_QUADS);

    geometry.tex_tri_count = N;
    geometry.tex_quad_count = P;
    geometry.untex_tri_count = Q;
    geometry.untex_quad_count = R;
    geometry.vertex_count = N * 3 + (P * 6) + Q * 3 + (R * 6);

    // Textured triangle
    for (int i = 0; i < N; i++) {
        geometry.tex_tris[i].a.position = read_position(f);
        geometry.tex_tris[i].b.position = read_position(f);
        geometry.tex_tris[i].c.position = read_position(f);
    }

    // Textured quads
    for (int i = 0; i < P; i++) {
        geometry.tex_quads[i].a.position = read_position(f);
        geometry.tex_quads[i].b.position = read_position(f);
        geometry.tex_quads[i].c.position = read_position(f);
        geometry.tex_quads[i].d.position = read_position(f);
    }

    // Untextured triangle
    for (int i = 0; i < Q; i++) {
        geometry.untex_tris[i].a.position = read_position(f);
        geometry.untex_tris[i].b.position = read_position(f);
        geometry.untex_tris[i].c.position = read_position(f);
    }

    // Untextured quads
    for (int i = 0; i < R; i++) {
        geometry.untex_quads[i].a.position = read_position(f);
        geometry.untex_quads[i].b.position = read_position(f);
        geometry.untex_quads[i].c.position = read_position(f);
        geometry.untex_quads[i].d.position = read_position(f);
    }

    // Triangle normals
    for (int i = 0; i < N; i++) {
        geometry.tex_tris[i].a.normal = read_normal(f);
        geometry.tex_tris[i].b.normal = read_normal(f);
        geometry.tex_tris[i].c.normal = read_normal(f);
    };

    // Quad normals
    for (int i = 0; i < P; i++) {
        geometry.tex_quads[i].a.normal = read_normal(f);
        geometry.tex_quads[i].b.normal = read_normal(f);
        geometry.tex_quads[i].c.normal = read_normal(f);
        geometry.tex_quads[i].d.normal = read_normal(f);
    };

    // Triangle UV
    for (int i = 0; i < N; i++) {
        float au = read_u8(f);
        float av = read_u8(f);
        float palette = read_u8(f);
        (void)read_u8(f); // padding
        float bu = read_u8(f);
        float bv = read_u8(f);
        float page = (read_u8(f) & 0x03); // 0b00000011
        (void)read_u8(f);                 // padding
        float cu = read_u8(f);
        float cv = read_u8(f);

        vec2s a = process_tex_coords(au, av, page);
        vec2s b = process_tex_coords(bu, bv, page);
        vec2s c = process_tex_coords(cu, cv, page);

        geometry.tex_tris[i].a.uv = a;
        geometry.tex_tris[i].a.palette_index = palette;
        geometry.tex_tris[i].a.is_textured = 1.0f;
        geometry.tex_tris[i].b.uv = b;
        geometry.tex_tris[i].b.palette_index = palette;
        geometry.tex_tris[i].b.is_textured = 1.0f;
        geometry.tex_tris[i].c.uv = c;
        geometry.tex_tris[i].c.palette_index = palette;
        geometry.tex_tris[i].c.is_textured = 1.0f;
    }

    // Quad UV
    for (int i = 0; i < P; i++) {
        float au = read_u8(f);
        float av = read_u8(f);
        float palette = read_u8(f);
        (void)read_u8(f); // padding
        float bu = read_u8(f);
        float bv = read_u8(f);
        float page = (read_u8(f) & 0x03); // 0b00000011
        (void)read_u8(f);                 // padding
        float cu = read_u8(f);
        float cv = read_u8(f);
        float du = read_u8(f);
        float dv = read_u8(f);

        vec2s a = process_tex_coords(au, av, page);
        vec2s b = process_tex_coords(bu, bv, page);
        vec2s c = process_tex_coords(cu, cv, page);
        vec2s d = process_tex_coords(du, dv, page);

        geometry.tex_quads[i].a.uv = a;
        geometry.tex_quads[i].a.palette_index = palette;
        geometry.tex_quads[i].a.is_textured = 1.0f;
        geometry.tex_quads[i].b.uv = b;
        geometry.tex_quads[i].b.palette_index = palette;
        geometry.tex_quads[i].b.is_textured = 1.0f;
        geometry.tex_quads[i].c.uv = c;
        geometry.tex_quads[i].c.palette_index = palette;
        geometry.tex_quads[i].c.is_textured = 1.0f;
        geometry.tex_quads[i].d.uv = d;
        geometry.tex_quads[i].d.palette_index = palette;
        geometry.tex_quads[i].d.is_textured = 1.0f;
    }

    geometry.valid = true;
    return geometry;
}

static texture_t read_texture(file_t* f)
{
    const int TEXTURE_ON_DISK_SIZE = (TEXTURE_SIZE / 2); // Each pixel stored as 1/2 a byte

    texture_t texture = { 0 };

    for (int i = 0; i < TEXTURE_ON_DISK_SIZE * 8; i += 8) {
        uint8_t raw_pixel = read_u8(f);
        uint8_t right = ((raw_pixel & 0x0F));
        uint8_t left = ((raw_pixel & 0xF0) >> 4);
        texture.data[i + 0] = right;
        texture.data[i + 1] = right;
        texture.data[i + 2] = right;
        texture.data[i + 3] = right;
        texture.data[i + 4] = left;
        texture.data[i + 5] = left;
        texture.data[i + 6] = left;
        texture.data[i + 7] = left;
    }

    texture.valid = true;
    return texture;
}

static palette_t read_palette(file_t* f)
{
    palette_t palette = { 0 };

    f->offset = 0x44;
    uint32_t intra_file_ptr = read_u32(f);
    if (intra_file_ptr == 0) {
        return palette;
    }

    f->offset = intra_file_ptr;

    for (int i = 0; i < 16 * 16 * 4; i = i + 4) {
        vec4s c = read_rgb15(f);
        palette.data[i + 0] = c.x;
        palette.data[i + 1] = c.y;
        palette.data[i + 2] = c.z;
        palette.data[i + 3] = c.w;
    }

    palette.valid = true;
    return palette;
}

// read_light_color clamps the value between 0.0 and 1.0. These unclamped values
// are used to affect the lighting model but it isn't understood yet.
// https://ffhacktics.com/wiki/Maps/Mesh#Light_colors_and_positions.2C_background_gradient_colors
static lighting_t read_lighting(file_t* f)
{
    lighting_t lighting = { 0 };

    f->offset = 0x64;
    uint32_t intra_file_ptr = read_u32(f);
    if (intra_file_ptr == 0) {
        return lighting;
    }

    f->offset = intra_file_ptr;

    vec4s a_color = { .w = 1.0f };
    vec4s b_color = { .w = 1.0f };
    vec4s c_color = { .w = 1.0f };

    a_color.x = read_light_color(f);
    b_color.x = read_light_color(f);
    c_color.x = read_light_color(f);
    a_color.y = read_light_color(f);
    b_color.y = read_light_color(f);
    c_color.y = read_light_color(f);
    a_color.z = read_light_color(f);
    b_color.z = read_light_color(f);
    c_color.z = read_light_color(f);

    bool a_valid = a_color.r + a_color.g + a_color.b > 0.0f;
    bool b_valid = b_color.r + b_color.g + b_color.b > 0.0f;
    bool c_valid = c_color.r + c_color.g + c_color.b > 0.0f;

    vec3s a_pos = read_position(f);
    vec3s b_pos = read_position(f);
    vec3s c_pos = read_position(f);

    lighting.lights[0] = (light_t) { .color = a_color, .direction = a_pos, .valid = a_valid };
    lighting.lights[1] = (light_t) { .color = b_color, .direction = b_pos, .valid = b_valid };
    lighting.lights[2] = (light_t) { .color = c_color, .direction = c_pos, .valid = c_valid };

    lighting.ambient_color = read_rgb8(f);
    lighting.ambient_strength = 2.0f;

    lighting.bg_top = read_rgb8(f);
    lighting.bg_bottom = read_rgb8(f);

    lighting.valid = true;
    return lighting;
}

mesh_t read_mesh(file_t* f)
{
    mesh_t mesh = { 0 };

    mesh.geometry = read_geometry(f);
    mesh.palette = read_palette(f);
    mesh.lighting = read_lighting(f);

    bool is_valid = mesh.geometry.valid || mesh.palette.valid || mesh.lighting.valid;
    mesh.valid = is_valid;

    return mesh;
}

static vec3s read_position(file_t* f)
{
    float x = read_i16(f);
    float y = read_i16(f);
    float z = read_i16(f);

    y = -y;
    z = -z;

    return (vec3s) { { x, y, z } };
}

static vec3s read_normal(file_t* f)
{
    float x = read_f1x3x12(f);
    float y = read_f1x3x12(f);
    float z = read_f1x3x12(f);

    y = -y;
    z = -z;

    return (vec3s) { { x, y, z } };
}

static vec4s read_rgb8(file_t* f)
{
    float red = read_u8(f) / 255.0f;
    float green = read_u8(f) / 255.0f;
    float blue = read_u8(f) / 255.0f;
    return (vec4s) { { red, green, blue, 1.0f } };
}

static vec4s read_rgb15(file_t* f)
{
    uint16_t val = read_u16(f);
    uint8_t a = val == 0 ? 0x00 : 0xFF;
    uint8_t blue = (val & 0x7C00) >> 7;  // 0b0111110000000000
    uint8_t green = (val & 0x03E0) >> 2; // 0b0000001111100000
    uint8_t red = (val & 0x001F) << 3;   // 0b0000000000011111
    return (vec4s) { { red, green, blue, a } };
}

static float read_light_color(file_t* f)
{
    float val = read_f1x3x12(f);
    return MIN(MAX(0.0f, val), 1.0f);
}

static record_t read_record(file_t* f)
{
    uint8_t bytes[GNS_RECORD_SIZE];
    read_bytes(f, GNS_RECORD_SIZE, bytes);
    int sector = bytes[8] | bytes[9] << 8;
    uint64_t length = (uint32_t)(bytes[12]) | ((uint32_t)(bytes[13]) << 8) | ((uint32_t)(bytes[14]) << 16) | ((uint32_t)(bytes[15]) << 24);
    filetype_e type = (bytes[4] | (bytes[5] << 8));
    time_e time = (bytes[3] >> 7) & 0x1;
    weather_e weather = (bytes[3] >> 4) & 0x7;
    int layout = bytes[2];

    record_t record = {
        .sector = sector,
        .length = length,
        .type = type,
        .state = {
            .time = time,
            .weather = weather,
            .layout = layout,
        },
    };
    memcpy(record.data, bytes, GNS_RECORD_SIZE);
    return record;
}

static int read_records(file_t* f, record_t* out_records)
{
    int count = 0;
    while (true) {
        record_t record = read_record(f);
        if (record.type == FILETYPE_END) {
            break;
        }
        out_records[count] = record;
        count++;
    }
    return count;
}

void merge_meshes(mesh_t* dst, mesh_t* src)
{
    assert(dst != NULL);
    assert(src != NULL);

    if (src->geometry.valid) {
        dst->geometry = src->geometry;
    }

    if (src->palette.valid) {
        dst->palette = src->palette;
    }

    if (src->lighting.valid) {
        for (int i = 0; i < MESH_MAX_LIGHTS; i++) {
            if (src->lighting.lights[i].valid) {
                for (int j = 0; j < MESH_MAX_LIGHTS; j++) {
                    dst->lighting.lights[j] = src->lighting.lights[j];
                }
                break;
            }
        }

        dst->lighting.ambient_strength = src->lighting.ambient_strength;
        dst->lighting.ambient_color = src->lighting.ambient_color;
        dst->lighting.bg_top = src->lighting.bg_top;
        dst->lighting.bg_bottom = src->lighting.bg_bottom;
    }
}

vertices_t geometry_to_vertices(geometry_t* geometry)
{
    vertices_t vertices = { 0 };

    // 10 triangles = 30 offset
    int vcount = 0;

    for (int i = 0; i < geometry->tex_tri_count; i++) {
        vertices.vertices[vcount++] = geometry->tex_tris[i].a;
        vertices.vertices[vcount++] = geometry->tex_tris[i].b;
        vertices.vertices[vcount++] = geometry->tex_tris[i].c;
    }

    for (int i = 0; i < geometry->tex_quad_count; i++) {
        vertices.vertices[vcount++] = geometry->tex_quads[i].a;
        vertices.vertices[vcount++] = geometry->tex_quads[i].b;
        vertices.vertices[vcount++] = geometry->tex_quads[i].c;

        vertices.vertices[vcount++] = geometry->tex_quads[i].b;
        vertices.vertices[vcount++] = geometry->tex_quads[i].d;
        vertices.vertices[vcount++] = geometry->tex_quads[i].c;
    }

    for (int i = 0; i < geometry->untex_tri_count; i++) {
        vertices.vertices[vcount++] = geometry->untex_tris[i].a;
        vertices.vertices[vcount++] = geometry->untex_tris[i].b;
        vertices.vertices[vcount++] = geometry->untex_tris[i].c;
    }

    for (int i = 0; i < geometry->untex_quad_count; i++) {
        vertices.vertices[vcount++] = geometry->untex_quads[i].a;
        vertices.vertices[vcount++] = geometry->untex_quads[i].b;
        vertices.vertices[vcount++] = geometry->untex_quads[i].c;

        vertices.vertices[vcount++] = geometry->untex_quads[i].b;
        vertices.vertices[vcount++] = geometry->untex_quads[i].d;
        vertices.vertices[vcount++] = geometry->untex_quads[i].c;
    }

    vertices.count = vcount;

    return vertices;
}

// 16 palettes of 16 colors of 4 bytes
// process_tex_coords has two functions:
//
// 1. Update the v coordinate to the specific page of the texture. FFT
//    Textures have 4 pages (256x1024) and the original V specifies
//    the pixel on one of the 4 pages. Multiply the page by the height
//    of a single page (256).
// 2. Normalize the coordinates that can be U:0-255 and V:0-1023. Just
//    divide them by their max to get a 0.0-1.0 value.
static vec2s process_tex_coords(float u, float v, uint8_t page)
{
    u = u / 255.0f;
    v = (v + (page * 256)) / 1023.0f;
    return (vec2s) { { u, v } };
}

void fft_time_str(time_e value, char out[static 8])
{
    switch (value) {
    case TIME_DAY:
        strcpy(out, "Day");
        break;
    case TIME_NIGHT:
        strcpy(out, "Night");
        break;
    default:
        strcpy(out, "Unknown");
        break;
    }
}

void fft_weather_str(weather_e value, char out[static 12])
{
    switch (value) {
    case WEATHER_NONE:
        strcpy(out, "None");
        break;
    case WEATHER_NONE_ALT:
        strcpy(out, "NoneAlt");
        break;
    case WEATHER_NORMAL:
        strcpy(out, "Normal");
        break;
    case WEATHER_STRONG:
        strcpy(out, "Strong");
        break;
    case WEATHER_VERY_STRONG:
        strcpy(out, "VeryStrong");
        break;
    default:
        strcpy(out, "Unknown");
        break;
    }
}

void fft_filetype_str(filetype_e value, char* out)
{
    switch (value) {
    case FILETYPE_MESH_PRIMARY:
        strcpy(out, "Primary");
        break;
    case FILETYPE_MESH_ALT:
        strcpy(out, "Alt");
        break;
    case FILETYPE_MESH_OVERRIDE:
        strcpy(out, "Override");
        break;
    case FILETYPE_TEXTURE:
        strcpy(out, "Texture");
        break;
    case FILETYPE_END:
        strcpy(out, "End");
        break;
    default:
        strcpy(out, "Unknown");
        break;
    }
}

bool fft_map_state_default(map_state_t a)
{
    return a.time == TIME_DAY && a.weather == WEATHER_NONE && a.layout == 0;
}

bool fft_map_state_eq(map_state_t a, map_state_t b)
{
    return a.time == b.time && a.weather == b.weather && a.layout == b.layout;
}

bool fft_map_state_unique(record_t* unique_records, int unique_record_count, record_t record)
{
    for (int i = 0; i < unique_record_count; i++) {
        if (fft_map_state_eq(unique_records[i].state, record.state)) {
            return false;
        }
    }
    return true;
}

static void read_map_data(int num, map_data_t* map_data_out)
{
    file_t gns_file = read_file(g.bin, fft_map_list[num].sector, GNS_FILE_MAX_SIZE);
    map_data_out->record_count = read_records(&gns_file, map_data_out->records);

    free(gns_file.data);

    for (int i = 0; i < map_data_out->record_count; i++) {
        record_t record = map_data_out->records[i];
        file_t file = read_file(g.bin, record.sector, record.length);

        switch (record.type) {
        case FILETYPE_TEXTURE: {
            texture_t texture = read_texture(&file);
            texture.map_state = record.state;
            map_data_out->textures[map_data_out->texture_count++] = texture;
            break;
        }
        case FILETYPE_MESH_PRIMARY:
            // There always only one primary mesh file and it uses default state.
            assert(fft_map_state_default(record.state));

            map_data_out->primary_mesh = read_mesh(&file);
            assert(map_data_out->primary_mesh.valid);
            break;

        case FILETYPE_MESH_ALT: {
            mesh_t alt_mesh = read_mesh(&file);
            alt_mesh.map_state = record.state;
            map_data_out->alt_meshes[map_data_out->alt_mesh_count++] = alt_mesh;
            break;
        }

        case FILETYPE_MESH_OVERRIDE: {
            // If there is an override file, there is only one and it uses default state.
            assert(fft_map_state_default(record.state));

            map_data_out->override_mesh = read_mesh(&file);
            break;
        }

        default:
            free(file.data);
            assert(false);
        }

        free(file.data);
    }
}

static vec3s vertices_centered(vertices_t* vertices)
{
    float min_x = FLT_MAX;
    float max_x = -FLT_MAX;
    float min_y = FLT_MAX;
    float max_y = -FLT_MAX;
    float min_z = FLT_MAX;
    float max_z = -FLT_MAX;

    for (int i = 0; i < vertices->count; i++) {
        const vertex_t vertex = vertices->vertices[i];

        min_x = MIN(vertex.position.x, min_x);
        min_y = MIN(vertex.position.y, min_y);
        min_z = MIN(vertex.position.z, min_z);

        max_x = MAX(vertex.position.x, max_x);
        max_y = MAX(vertex.position.y, max_y);
        max_z = MAX(vertex.position.z, max_z);
    }

    float x = -(max_x + min_x) / 2.0f;
    float y = -(max_y + min_y) / 2.0f;
    float z = -(max_z + min_z) / 2.0f;

    return (vec3s) { { x, y, z } };
}

// Thanks to FFTPAtcher for the scenario name list.
// https://github.com/Glain/FFTPatcher/blob/master/EntryEdit/EntryData/PSX/ScenarioNames.xml
scenario_desc_t fft_scenario_list[500] = {
    { 0x0000, "Unusable" },
    { 0x0001, "Orbonne Prayer (Setup)" },
    { 0x0002, "Orbonne Prayer" },
    { 0x0003, "Orbonne Battle (Setup)" },
    { 0x0004, "Orbonne Battle" },
    { 0x0005, "Orbonne Battle (Gafgarion and Agrias chat)" },
    { 0x0006, "Orbonne Battle (Abducting the Princess)" },
    { 0x0007, "Military Academy (Setup)" },
    { 0x0008, "Military Academy" },
    { 0x0009, "Gariland Fight (Setup)" },
    { 0x000A, "Gariland Fight" },
    { 0x000B, "Gariland Fight (Ramza, Delita, Thief Chat)" },
    { 0x000C, "Gariland Fight (Ramza talking about honest lives)" },
    { 0x000D, "Balbanes's Death (Setup)" },
    { 0x000E, "Balbanes's Death" },
    { 0x000F, "Mandalia Plains (Setup)" },
    { 0x0010, "Mandalia Plains (Options Given)" },
    { 0x0011, "Mandalia Plains (Destroy Corps Chosen)" },
    { 0x0012, "Mandalia Plains (Save Algus Chosen)" },
    { 0x0013, "Mandalia Plains (Algus First Turn)" },
    { 0x0014, "Mandalia Plains (Algus KO'd, Destroy Chosen)" },
    { 0x0015, "Mandalia Plains (Algus KO'd, Save Chosen)" },
    { 0x0016, "Mandalia Plains (Victory, Algus KO'd)" },
    { 0x0017, "Mandalia Plains (Victory, Algus Alive)" },
    { 0x0018, "Introducing Algus (Setup)" },
    { 0x0019, "Introducing Algus" },
    { 0x001A, "Returning to Igros (Setup)" },
    { 0x001B, "Returning to Igros" },
    { 0x001C, "Family Meeting (Setup)" },
    { 0x001D, "Family Meeting" },
    { 0x001E, "Sweegy Woods (Setup)" },
    { 0x001F, "Sweegy Woods" },
    { 0x0020, "Sweegy Woods (Victory)" },
    { 0x0021, "Dorter Trade City1 (Setup)" },
    { 0x0022, "Dorter Trade City1" },
    { 0x0023, "Dorter Trade City1 (Algus and Delita talk)" },
    { 0x0024, "Dorter Trade City1 (Victory)" },
    { 0x0025, "Interrogation (Setup)" },
    { 0x0026, "Interrogation" },
    { 0x0027, "Sand Rat Cellar (Setup)" },
    { 0x0028, "Sand Rat Cellar" },
    { 0x0029, "Sand Rat Cellar (Victory)" },
    { 0x002A, "Gustav vs. Wiegraf (Setup)" },
    { 0x002B, "Gustav vs. Wiegraf" },
    { 0x002C, "Larg's Praise (Setup)" },
    { 0x002D, "Larg's Praise" },
    { 0x002E, "Miluda1 (Setup)" },
    { 0x002F, "Miluda1" },
    { 0x0030, "Miluda1 (Miluda and Algus arguing)" },
    { 0x0031, "Miluda1 (Delita talking)" },
    { 0x0032, "Miluda1 (Victory)" },
    { 0x0033, "Releasing Miluda (Setup)" },
    { 0x0034, "Releasing Miluda" },
    { 0x0035, "Attack on the Beoulves (Setup)" },
    { 0x0036, "Attack on the Beoulves" },
    { 0x0037, "Meeting with bedridden Dycedarg (Setup)" },
    { 0x0038, "Meeting with bedridden Dycedarg" },
    { 0x0039, "Expelling Algus (Setup)" },
    { 0x003A, "Expelling Algus" },
    { 0x003B, "Reed Whistle (Setup)" },
    { 0x003C, "Reed Whistle" },
    { 0x003D, "Miluda2 (Setup)" },
    { 0x003E, "Miluda2" },
    { 0x003F, "Miluda2 (Delita talking with Miluda)" },
    { 0x0040, "Miluda2 (Miluda half HP)" },
    { 0x0041, "Miluda2 (Ramza debating with Miluda)" },
    { 0x0042, "Miluda2 (Ramza pleading with Miluda)" },
    { 0x0043, "Miluda2 (Miluda's Death)" },
    { 0x0044, "Wiegraf berating Golagros (Setup)" },
    { 0x0045, "Wiegraf berating Golagros" },
    { 0x0046, "Wiegraf1 (Setup)" },
    { 0x0047, "Wiegraf1" },
    { 0x0048, "Wiegraf1 (Delita, Ramza, Wiegraf talk)" },
    { 0x0049, "Wiegraf1 (Ramza and Wiegraf debate)" },
    { 0x004A, "Wiegraf1 (Ramza and Wiegraf talk)" },
    { 0x004B, "Wiegraf1 (Victory)" },
    { 0x004C, "Finding Teta Missing (Setup)" },
    { 0x004D, "Finding Teta Missing" },
    { 0x004E, "Fort Zeakden (Setup)" },
    { 0x004F, "Fort Zeakden" },
    { 0x0050, "Fort Zeakden (Algus, Ramza round 1)" },
    { 0x0051, "Fort Zeakden (Algus, Ramza round 2)" },
    { 0x0052, "Fort Zeakden (Algus, Ramza round 3)" },
    { 0x0053, "Fort Zeakden (Destroy Chosen at Mandalia)" },
    { 0x0054, "Fort Zeakden (Save Chosen at Mandalia)" },
    { 0x0055, "Fort Zeakden (Delita's First Turn)" },
    { 0x0056, "Fort Zeakden (Algus, Delita round 1)" },
    { 0x0057, "Fort Zeakden (Ramza, Delita talking)" },
    { 0x0058, "Fort Zeakden (Victory)" },
    { 0x0059, "Partings (Setup)" },
    { 0x005A, "Partings" },
    { 0x005B, "Deep Dungeon NOGIAS (Setup)" },
    { 0x005C, "Deep Dungeon NOGIAS (Battle)" },
    { 0x005D, "Deep Dungeon Panel Found" },
    { 0x005E, "Deep Dungeon (Victory - Used for all Floors)" },
    { 0x005F, "Deep Dungeon TERMINATE(Setup)" },
    { 0x0060, "Deep Dungeon TERMINATE (Battle)" },
    { 0x0061, "Deep Dungeon DELTA (Setup)" },
    { 0x0062, "Deep Dungeon DELTA (Battle)" },
    { 0x0063, "Deep Dungeon VALKYRIES (Setup)" },
    { 0x0064, "Deep Dungeon VALKYRIES (Battle)" },
    { 0x0065, "Deep Dungeon MLAPAN (Setup)" },
    { 0x0066, "Deep Dungeon MLAPAN (Battle)" },
    { 0x0067, "Deep Dungeon TIGER (Setup)" },
    { 0x0068, "Deep Dungeon TIGER (Battle)" },
    { 0x0069, "Deep Dungeon BRIDGE (Setup)" },
    { 0x006A, "Deep Dungeon BRIDGE (Battle)" },
    { 0x006B, "Deep Dungeon VOYAGE (Setup)" },
    { 0x006C, "Deep Dungeon VOYAGE (Battle)" },
    { 0x006D, "Deep Dungeon HORROR (Setup)" },
    { 0x006E, "Deep Dungeon HORROR (Battle)" },
    { 0x006F, "Elidibs (Setup)" },
    { 0x0070, "Elidibs" },
    { 0x0071, "Elidibs (Victory)" },
    { 0x0072, "Deep Dungeon END (Setup)" },
    { 0x0073, "Deep Dungeon END (Battle)" },
    { 0x0074, "Chapter 2 Start (Setup)" },
    { 0x0075, "Chapter 2 Start" },
    { 0x0076, "Dorter2 (Setup)" },
    { 0x0077, "Dorter2" },
    { 0x0078, "Dorter2 (Victory)" },
    { 0x0079, "Araguay Woods (Setup)" },
    { 0x007A, "Araguay Woods (Options Given)" },
    { 0x007B, "Araguay Woods (Kill Enemies Chosen)" },
    { 0x007C, "Araguay Woods (Save Boco Chosen)" },
    { 0x007D, "Araguay Woods (Boco KO'd, Kill Enemies Chosen)" },
    { 0x007E, "Araguay Woods (Boco KO'd, Save Boco Chosen)" },
    { 0x007F, "Araguay Woods (Victory)" },
    { 0x0080, "Zirekile Falls (Setup)" },
    { 0x0081, "Zirekile Falls" },
    { 0x0082, "Zirekile Falls (Gafgarion and Agrias talk)" },
    { 0x0083, "Zirekile Falls (Gafgarion, Ramza, Delita, talk)" },
    { 0x0084, "Zirekile Falls (Delita, Ovelia talk)" },
    { 0x0085, "Zirekile Falls (Ovelia's Death)" },
    { 0x0086, "Zirekile Falls (Gafgarion and Ramza arguing)" },
    { 0x0087, "Zirekile Falls (Gafgarion retreat)" },
    { 0x0088, "Zirekile Falls (Victory)" },
    { 0x0089, "Ovelia Joins (Setup)" },
    { 0x008A, "Ovelia Joins" },
    { 0x008B, "Zalamd Fort City (Setup)" },
    { 0x008C, "Zaland Fort City (Options Given)" },
    { 0x008D, "Zaland Fort City (Kill Enemies Chosen)" },
    { 0x008E, "Zaland Fort City (Save Mustadio Chosen)" },
    { 0x008F, "Zaland Fort City (Mustadio KO'd, Kill Chosen)" },
    { 0x0090, "Zaland Fort City (Mustadio KO'd, Save Chosen)" },
    { 0x0091, "Zaland Fort City (Victory)" },
    { 0x0092, "Ramza, Mustadio, Agrias and Ovelia meeting (Setup)" },
    { 0x0093, "Ramza, Mustadio, Agrias and Ovelia meeting" },
    { 0x0094, "Ruins of Zaland (Setup)" },
    { 0x0095, "Ruins of Zaland" },
    { 0x0096, "Bariaus Hill (Setup)" },
    { 0x0097, "Bariaus Hill" },
    { 0x0098, "Bariaus Hill (Victory)" },
    { 0x0099, "Dycedarg and Gafgarion Reunion (Setup)" },
    { 0x009A, "Dycedarg and Gafgarion Reunion" },
    { 0x009B, "Gate of Lionel Castle (Setup)" },
    { 0x009C, "Gate of Lionel Castle" },
    { 0x009D, "Meeting with Draclay (Setup)" },
    { 0x009E, "Meeting with Draclau" },
    { 0x009F, "Besrodio Kidnapped (Setup)" },
    { 0x00A0, "Besrodio Kidnapped" },
    { 0x00A1, "Zigolis Swamp (Setup)" },
    { 0x00A2, "Zigolis Swamp" },
    { 0x00A3, "Zigolis Swamp (Victory)" },
    { 0x00A4, "Goug Machine City Town (Setup)" },
    { 0x00A5, "Goug Machine City Town" },
    { 0x00A6, "Goug Machine City (Setup)" },
    { 0x00A7, "Goug Machine City" },
    { 0x00A8, "Goug Machine City (Victory)" },
    { 0x00A9, "Besrodio Saved (Setup)" },
    { 0x00AA, "Besrodio Saved" },
    { 0x00AB, "Warjilis Port (Setup)" },
    { 0x00AC, "Warjilis Port" },
    { 0x00AD, "Draclau hires Gafgarion (Setup)" },
    { 0x00AE, "Draclau hires Gafgarion" },
    { 0x00AF, "Bariaus Valley (Setup)" },
    { 0x00B0, "Bariaus Valley" },
    { 0x00B1, "Bariaus Valley (Agrias and Ramza talk)" },
    { 0x00B2, "Bariaus Valley (Agrias Death)" },
    { 0x00B3, "Bariaus Valley (Victory)" },
    { 0x00B4, "Golgorand Execution Site (Setup)" },
    { 0x00B5, "Golgorand Execution Site" },
    { 0x00B6, "Golgorand Execution Site (Gafgarion and Agrias talk)" },
    { 0x00B7, "Golgorand Execution Site (Gafgarion and Ramza talk first part)" },
    { 0x00B8, "Golgorand Execution Site (Gafgarion and Ramza talk second part)" },
    { 0x00B9, "Golgorand Execution Site (Gafgarion and Ramza talk third part)" },
    { 0x00BA, "Golgorand Execution Site (Gafgarion, Agrias and Ramza talk)" },
    { 0x00BB, "Golgorand Execution Site (Gafgarion retreats)" },
    { 0x00BC, "Golgorand Execution Site (Victory)" },
    { 0x00BD, "Substitute (Setup)" },
    { 0x00BE, "Substitute" },
    { 0x00BF, "Lionel Castle Gate (Setup)" },
    { 0x00C0, "Lionel Castle Gate" },
    { 0x00C1, "Lionel Castle Gate (Ramza opens the gate)" },
    { 0x00C2, "Lionel Castle Gate (Gafgarion Death)" },
    { 0x00C3, "Lionel Castle Gate (Victory)" },
    { 0x00C4, "Inside of Lionel Castle (Setup)" },
    { 0x00C5, "Inside of Lionel Castle" },
    { 0x00C6, "Inside of Lionel Castle (Queklain and Ramza talk)" },
    { 0x00C7, "Inside of Lionel Castle (Victory)" },
    { 0x00C8, "The Lion War Outbreak (Setup)" },
    { 0x00C9, "The Lion War Outbreak" },
    { 0x00CA, "Chapter 3 Start (Setup)" },
    { 0x00CB, "Chapter 3 Start" },
    { 0x00CC, "Goland Coal City (Setup)" },
    { 0x00CD, "Goland Coal City" },
    { 0x00CE, "Goland Coal City (Olan Death)" },
    { 0x00CF, "Goland Coal City (Victory)" },
    { 0x00D0, "Goland Coal City post battle (setup)" },
    { 0x00D1, "Goland Coal City post battle" },
    { 0x00D2, "Steel Ball Found! (Setup)" },
    { 0x00D3, "Steel Ball Found!" },
    { 0x00D4, "Worker 8 Activated (setup)" },
    { 0x00D5, "Worker 8 Activated" },
    { 0x00D6, "Summoning Machine Found! (Setup)" },
    { 0x00D7, "Summoning Machine Found!" },
    { 0x00D8, "Cloud Summoned (Setup)" },
    { 0x00D9, "Cloud Summoned" },
    { 0x00DA, "Zarghidas (Setup)" },
    { 0x00DB, "Zarghidas" },
    { 0x00DC, "Zarghidas (Cloud freaking out)" },
    { 0x00DD, "Zarghidas (Cloud Death)" },
    { 0x00DE, "Zarghidas (Victory)" },
    { 0x00DF, "Talk with Zalbag in Lesalia (Setup)" },
    { 0x00E0, "Talk with Zalbag in Lesalia" },
    { 0x00E1, "Outside Castle Gate in Lesalia Zalmo 1 (Setup)" },
    { 0x00E2, "Outside Castle Gate in Lesalia Zalmo 1" },
    { 0x00E3, "Outside Castle Gate in Lesalia Zalmo 1 (Zalmo and Ramza talk)" },
    { 0x00E4, "Outside Castle Gate in Lesalia Zalmo 1 (Alma and Ramza talk)" },
    { 0x00E5, "Outside Castle Gate in Lesalia Zalmo 1 (Victory)" },
    { 0x00E6, "Outside Castle Gate in Lesalia Talk with Alma (Setup)" },
    { 0x00E7, "Outside Castle Gate in Lesalia Talk with Alma" },
    { 0x00E8, "Orbonne Monastery (Setup)" },
    { 0x00E9, "Orbonne Monastery" },
    { 0x00EA, "Underground Book Storage Second Floor (Setup)" },
    { 0x00EB, "Underground Book Storage Second Floor" },
    { 0x00EC, "Underground Book Storage Second Floor (Victory)" },
    { 0x00ED, "Underground Book Storage Third Floor (Setup)" },
    { 0x00EE, "Underground Book Storage Third Floor" },
    { 0x00EF, "Underground Book Storage Third Floor (Izlude, Ramza talk first)" },
    { 0x00F0, "Underground Book Storage Third Floor (Izlude, Ramza talk second)" },
    { 0x00F1, "Underground Book Storage Third Floor (Victory)" },
    { 0x00F2, "Underground Book Storage First Floor (Setup)" },
    { 0x00F3, "Underground Book Storage First Floor" },
    { 0x00F4, "Underground Book Storage First Floor (Wiegraf talk)" },
    { 0x00F5, "Underground Book Storage First Floor (Wiegraf, Ramza talk first)" },
    { 0x00F6, "Underground Book Storage First Floor (Wiegraf, Ramza talk second)" },
    { 0x00F7, "Underground Book Storage First Floor (Victory)" },
    { 0x00F8, "Meet Velius (Setup)" },
    { 0x00F9, "Meet Velius" },
    { 0x00FA, "Malak and the Scriptures (Setup)" },
    { 0x00FB, "Malak and the Scriptures (Options Given)" },
    { 0x00FC, "Malak and the Scriptures (Yes Chosen)" },
    { 0x00FD, "Malak and the Scriptures (No Chosen)" },
    { 0x00FE, "Delita swears allegiance to Ovelia (Setup)" },
    { 0x00FF, "Delita swears allegiance to Ovelia" },
    { 0x0100, "Grog Hill (Setup)" },
    { 0x0101, "Grog Hill" },
    { 0x0102, "Grog Hill (Victory)" },
    { 0x0103, "Meet Again with Olan (Setup)" },
    { 0x0104, "Meet again with Olan" },
    { 0x0105, "Rescue Rafa (Setup)" },
    { 0x0106, "Rescue Rafa" },
    { 0x0107, "Rescue Rafa (Malak and Ramza talk)" },
    { 0x0108, "Rescue Rafa (Malak, Ninja and Ramza talk)" },
    { 0x0109, "Rescue Rafa (Malak Retreat)" },
    { 0x010A, "Rescue Rafa (Rafa Death, Malak Present)" },
    { 0x010B, "Rescue Rafa (Rafa Death, Malak Retreated)" },
    { 0x010C, "Rescue Rafa (Victory)" },
    { 0x010D, "Exploding Frog (Setup)" },
    { 0x010E, "Exploding Frog" },
    { 0x010F, "Yuguo Woods (Setup)" },
    { 0x0110, "Yuguo Woods" },
    { 0x0111, "Yuguo Woods (Victory)" },
    { 0x0112, "Barinten threatens Vormav (Setup)" },
    { 0x0113, "Barinten threatens Vormav" },
    { 0x0114, "Riovanes Castle Entrance (Setup)" },
    { 0x0115, "Riovanes Castle Entrance" },
    { 0x0116, "Riovanes Castle Entrance (Rafa, Malak and Ramza talk)" },
    { 0x0117, "Riovanes Castle Entrance (Malak Defeated)" },
    { 0x0118, "Riovanes Castle Entrance (Rafa Defeated)" },
    { 0x0119, "Riovanes Castle Entrance (Victory)" },
    { 0x011A, "Escaping Alma (Setup)" },
    { 0x011B, "Escaping Alma" },
    { 0x011C, "Inside of Riovanes Castle (Setup)" },
    { 0x011D, "Inside of Riovanes Castle" },
    { 0x011E, "Inside of Riovanes Castle (Wiegraf and Ramza talk)" },
    { 0x011F, "Inside of Riovanes Castle (Here comes Velius)" },
    { 0x0120, "Inside of Riovanes Castle (Victory)" },
    { 0x0121, "Ajora's vessel (Setup)" },
    { 0x0122, "Ajora's vessel" },
    { 0x0123, "Rooftop of Riovanes Castle (Setup)" },
    { 0x0124, "Rooftop of Riovanes Castle" },
    { 0x0125, "Rooftop of Riovanes Castle (Rafa Death)" },
    { 0x0126, "Rooftop of Riovanes Castle (Victory)" },
    { 0x0127, "Reviving Malak (Setup)" },
    { 0x0128, "Reviving Malak" },
    { 0x0129, "Searching for Alma (Setup)" },
    { 0x012A, "Searching for Alma" },
    { 0x012B, "Things Obtained (Setup)" },
    { 0x012C, "Things Obtained" },
    { 0x012D, "Underground Book Storage Fourth Floor (Setup)" },
    { 0x012E, "Underground Book Storage Fourth Floor" },
    { 0x012F, "Underground Book Storage Fourth Floor (Victory)" },
    { 0x0130, "Underground Book Storage Fifth Floor (Setup)" },
    { 0x0131, "Underground Book Storage Fifth Floor" },
    { 0x0132, "Underground Book Storage Fifth Floor (Rofel and Ramza talk)" },
    { 0x0133, "Underground Book Storage Fifth Floor (Victory)" },
    { 0x0134, "Entrance to the other world (Setup)" },
    { 0x0135, "Entrance to the other world" },
    { 0x0136, "Murond Death City (Setup)" },
    { 0x0137, "Murond Death City" },
    { 0x0138, "Murond Death City (Kletian and Ramza talk)" },
    { 0x0139, "Murond Death City (Victory)" },
    { 0x013A, "Lost Sacred Precincts (Setup)" },
    { 0x013B, "Lost Sacred Precincts" },
    { 0x013C, "Lost Sacred Precincts (Balk and Ramza talk)" },
    { 0x013D, "Lost Sacred Precincts (Victory)" },
    { 0x013E, "Graveyard of Airships (Setup)" },
    { 0x013F, "Graveyard of Airships" },
    { 0x0140, "Graveyard of Airships (Hashmalum and Ramza talk)" },
    { 0x0141, "Graveyard of Airships (Victory)" },
    { 0x0142, "Graveyard of Airships (Setup)" },
    { 0x0143, "Graveyard of Airships" },
    { 0x0144, "Graveyard of Airships (Here comes Altima 2)" },
    { 0x0145, "Graveyard of Airships (Victory)" },
    { 0x0146, "Reunion and Beyond" },
    { 0x0147, "Reunion and beyond" },
    { 0x0148, "Those Who Squirm In Darkness (Setup)" },
    { 0x0149, "Those Who Squirm in Darkness" },
    { 0x014A, "A Man with the Holy Stone (Setup)" },
    { 0x014B, "A Man with the Holy Stone" },
    { 0x014C, "Doguola Pass (Setup)" },
    { 0x014D, "Doguola Pass" },
    { 0x014E, "Doguola Pass (Victory)" },
    { 0x014F, "Bervenia Free City (Setup)" },
    { 0x0150, "Bervenia Free City" },
    { 0x0151, "Bervenia Free City (Meliadoul and Ramza talk first part)" },
    { 0x0152, "Bervenia Free City (Meliadoul and Ramza talk second part)" },
    { 0x0153, "Bervenia Free City (Meliadoul and Ramza talk third part)" },
    { 0x0154, "Bervenia Free City (Victory)" },
    { 0x0155, "Finath River (Setup)" },
    { 0x0156, "Finath River" },
    { 0x0157, "Finath River (Victory)" },
    { 0x0158, "Delita's Thoughts (Setup)" },
    { 0x0159, "Delita's Thoughts" },
    { 0x015A, "Zalmo II (Setup)" },
    { 0x015B, "Zalmo II" },
    { 0x015C, "Zalmo II (Zalmo and Delita talk)" },
    { 0x015D, "Zalmo II (Zalmo and Ramza talk)" },
    { 0x015E, "Zalmo II (Victory)" },
    { 0x015F, "Unstoppable Cog (Setup)" },
    { 0x0160, "Unstoppable Cog" },
    { 0x0161, "Balk I (Setup)" },
    { 0x0162, "Balk I" },
    { 0x0163, "Balk I (Balk and Ramza talk)" },
    { 0x0164, "Balk I (Victory)" },
    { 0x0165, "Seized T" },
    { 0x0166, "Seized T" },
    { 0x0167, "South Wall of Bethla Garrison (Setup)" },
    { 0x0168, "South Wall of Bethla Garrison" },
    { 0x0169, "South Wall of Bethla Garrison (Victory)" },
    { 0x016A, "North Wall of Bethla Garrison (Setup)" },
    { 0x016B, "North Wall of Bethla Garrison" },
    { 0x016C, "North Wall of Bethla Garrison (Victory)" },
    { 0x016D, "Assassination of Prince Larg (Setup)" },
    { 0x016E, "Assassination of Prince Larg" },
    { 0x016F, "Bethla Sluice (Setup)" },
    { 0x0170, "Bethla Sluice" },
    { 0x0171, "Bethla Sluice (First lever left)" },
    { 0x0172, "Bethla Sluice (Second lever left)" },
    { 0x0173, "Bethla Sluice (First lever right)" },
    { 0x0174, "Bethla Sluice (Second lever right)" },
    { 0x0175, "Rescue of Cid (Setup)" },
    { 0x0176, "Rescue of Cid" },
    { 0x0177, "Prince Goltana's Final Moments (Setup)" },
    { 0x0178, "Prince Goltana's Final Moments" },
    { 0x0179, "Germinas Peak (Setup)" },
    { 0x017A, "Germinas Peak" },
    { 0x017B, "Germinas Peak (Victory)" },
    { 0x017C, "Poeskas Lake (Setup)" },
    { 0x017D, "Poeskas Lake" },
    { 0x017E, "Poeskas Lake (Victory)" },
    { 0x017F, "Ambition of Dycedarg (Setup)" },
    { 0x0180, "Ambition of Dycedarg" },
    { 0x0181, "Outside of Limberry Castle (Setup)" },
    { 0x0182, "Outside of Limberry Castle" },
    { 0x0183, "Outside of Limberry Castle (Victory)" },
    { 0x0184, "Men of Odd Appearance (Setup)" },
    { 0x0185, "Men of Odd Appearance" },
    { 0x0186, "Elmdor II (Setup)" },
    { 0x0187, "Elmdor II" },
    { 0x0188, "Elmdor II (Ultima Demon Celia)" },
    { 0x0189, "Elmdor II (Ultima Demon Lede)" },
    { 0x018A, "Elmdor II (Victory)" },
    { 0x018B, "Zalera (Setup)" },
    { 0x018C, "Zalera" },
    { 0x018D, "Zalera (Zalera, Meliadoul and Ramza talk)" },
    { 0x018E, "Zalera (Meliadoul and Ramza talk)" },
    { 0x018F, "Zalera (Victory)" },
    { 0x0190, "Random Battle Template (Setup)" },
    { 0x0191, "Random Battle Template (Initiate)" },
    { 0x0192, "Random Battle Template (Victory)" },
    { 0x0193, "Empty" },
    { 0x0194, "Game Over Event (Plays automatically upon Game Over)" },
    { 0x0195, "Empty" },
    { 0x0196, "Empty" },
    { 0x0197, "Empty" },
    { 0x0198, "Empty" },
    { 0x0199, "Empty" },
    { 0x019A, "Tutorial - (Battlefield Control) (Setup)" },
    { 0x019B, "Tutorial - (Battlefield Control)" },
    { 0x019C, "Tutorial - (Battle) (Setup)" },
    { 0x019D, "Tutorial - (Battle)" },
    { 0x019E, "Tutorial - (Move and Act) (Setup)" },
    { 0x019F, "Tutorial - (Move and Act)" },
    { 0x01A0, "Tutorial - (Charge Time Battle) (Setup)" },
    { 0x01A1, "Tutorial - (Charge Time Battle)" },
    { 0x01A2, "Tutorial - (How to Cast Spells) (Setup)" },
    { 0x01A3, "Tutorial - (How to Cast Spells)" },
    { 0x01A4, "Tutorial - (Abnormal Status) (Setup)" },
    { 0x01A5, "Tutorial - (Abnormal Status)" },
    { 0x01A6, "Tutorial - (On-Line Help) (Setup)" },
    { 0x01A7, "Tutorial - (On-Line Help)" },
    { 0x01A8, "Tutorial - (Options) (Setup)" },
    { 0x01A9, "Tutorial - (Options)" },
    { 0x01AA, "The Mystery of Lucavi (Setup)" },
    { 0x01AB, "The Mystery of Lucavi" },
    { 0x01AC, "Delita's Betrayal (Setup)" },
    { 0x01AD, "Delita's Betrayal" },
    { 0x01AE, "Delita's Betrayal" },
    { 0x01AF, "Mosfungus (Setup)" },
    { 0x01B0, "Mosfungus" },
    { 0x01B1, "At the Gate of the Beoulve Castle (Setup)" },
    { 0x01B2, "At the Gate of the Beoulve Castle" },
    { 0x01B3, "Adramelk (Setup)" },
    { 0x01B4, "Adramelk" },
    { 0x01B5, "Adramelk (Zalbag and Ramza talk)" },
    { 0x01B6, "Adramelk (Dycedarg and Zalbag talk)" },
    { 0x01B7, "Adramelk (Here comes Adramelk)" },
    { 0x01B8, "Adramelk (Victory)" },
    { 0x01B9, "Funeral's Final Moments (Setup)" },
    { 0x01BA, "Funeral's Final Moments" },
    { 0x01BB, "St. Murond Temple (Setup)" },
    { 0x01BC, "St. Murond Temple" },
    { 0x01BD, "St. Murond Temple (Victory)" },
    { 0x01BE, "Hall of St. Murond Temple (Setup)" },
    { 0x01BF, "Hall of St. Murond Temple" },
    { 0x01C0, "Hall of St. Murond Temple (Vormav and Meliadoul talk)" },
    { 0x01C1, "Hall of St. Murond Temple (Vormav and Ramza talk)" },
    { 0x01C2, "Hall of St. Murond Temple (Victory)" },
    { 0x01C3, "Chapel of St. Murond Temple (Setup)" },
    { 0x01C4, "Chapel of St. Murond Temple" },
    { 0x01C5, "Chapel of St. Murond Temple (Zalbag, Ramza first turn)" },
    { 0x01C6, "Chapel of St. Murond Temple (Ramza, Zalbag 50% HP talk)" },
    { 0x01C7, "Chapel of St. Murond Temple (Victory)" },
    { 0x01C8, "Requiem (Setup)" },
    { 0x01C9, "Requiem" },
    { 0x01CA, "Zarghidas (Setup)" },
    { 0x01CB, "Zarghidas (Options Given)" },
    { 0x01CC, "Zarghidas (Don't Buy Flower Chosen)" },
    { 0x01CD, "Zarghidas (Buy Flower Chosen)" },
    { 0x01CE, "Bar - Deep Dungeon (Setup)" },
    { 0x01CF, "Bar - Deep Dungeon" },
    { 0x01D0, "Bar - Goland Coal City (Setup)" },
    { 0x01D1, "Bar - Goland Coal City (Options Given)" },
    { 0x01D2, "Bar - Goland Coal City (Refuse Beowulf's Invitation Chosen)" },
    { 0x01D3, "Bar - Goland Coal City (Accept Beowulf's invitation Chosen)" },
    { 0x01D4, "Colliery Underground - Third Floor (Setup)" },
    { 0x01D5, "Colliery Underground - Third Floor (Battle)" },
    { 0x01D6, "Colliery Underground - Third Floor (Victory)" },
    { 0x01D7, "Colliery Underground - Second Floor (Setup)" },
    { 0x01D8, "Colliery Underground - Second Floor (Battle)" },
    { 0x01D9, "Colliery Underground - Second Floor (Victory)" },
    { 0x01DA, "Colliery Underground - First Floor (Setup)" },
    { 0x01DB, "Colliery Underground - First Floor (Battle)" },
    { 0x01DC, "Colliery Underground - First Floor (Victory)" },
    { 0x01DD, "Underground Passage in Goland (Setup)" },
    { 0x01DE, "Underground Passage in Goland (Battle)" },
    { 0x01DF, "Underground Passage in Goland (Reis's Death, Beowulf Alive)" },
    { 0x01E0, "Underground Passage in Goland (Reis's Death, Beowulf KO'd)" },
    { 0x01E1, "Underground Passage in Goland (Victory)" },
    { 0x01E2, "Underground Passage in Goland (Setup)" },
    { 0x01E3, "Underground Passage in Goland (Post-Battle)" },
    { 0x01E4, "Nelveska Temple (Setup)" },
    { 0x01E5, "Nelveska Temple" },
    { 0x01E6, "Nelveska Temple (Worker 7 recharging)" },
    { 0x01E7, "Nelveska Temple (Victory)" },
    { 0x01E8, "Reis Curse (Setup)" },
    { 0x01E9, "Reis Curse" },
    { 0x01EA, "Bethla Sluice (Late add-in Ramza hint)" },
    { 0x01EB, "Empty" },
    { 0x01EC, "Empty" },
    { 0x01ED, "Empty" },
    { 0x01EE, "Empty" },
    { 0x01EF, "Empty" },
    { 0x01F0, "Empty" },
    { 0x01F1, "Empty" },
    { 0x01F2, "Empty" },
    { 0x01F3, "Empty" },
};

map_desc_t fft_map_list[128] = {
    { 0, 10026, false, "Unknown" }, // No texture
    { 1, 11304, true, "At Main Gate of Igros Castle" },
    { 2, 12656, true, "Back Gate of Lesalia Castle" },
    { 3, 12938, true, "Hall of St. Murond Temple" },
    { 4, 13570, true, "Office of Lesalia Castle" },
    { 5, 14239, true, "Roof of Riovanes Castle" },
    { 6, 14751, true, "At the Gate of Riovanes Castle" },
    { 7, 15030, true, "Inside of Riovanes Castle" },
    { 8, 15595, true, "Riovanes Castle" },
    { 9, 16262, true, "Citadel of Igros Castle" },
    { 10, 16347, true, "Inside of Igros Castle" },
    { 11, 16852, true, "Office of Igros Castle" },
    { 12, 17343, true, "At the Gate of Lionel Castle" },
    { 13, 17627, true, "Inside of Lionel Castle" },
    { 14, 18175, true, "Office of Lionel Castle" },
    { 15, 19510, true, "At the Gate of Limberry Castle (1)" },
    { 16, 20075, true, "Inside of Limberry Castle" },
    { 17, 20162, true, "Underground Cemetery of Limberry Castle" },
    { 18, 20745, true, "Office of Limberry Castle" },
    { 19, 21411, true, "At the Gate of Limberry Castle (2)" },
    { 20, 21692, true, "Inside of Zeltennia Castle" },
    { 21, 22270, true, "Zeltennia Castle" },
    { 22, 22938, true, "Magic City Gariland" },
    { 23, 23282, true, "Belouve Residence" },
    { 24, 23557, true, "Military Academy's Auditorium" },
    { 25, 23899, true, "Yardow Fort City" },
    { 26, 23988, true, "Weapon Storage of Yardow" },
    { 27, 24266, true, "Goland Coal City" },
    { 28, 24544, true, "Colliery Underground First Floor" },
    { 29, 24822, true, "Colliery Underground Second Floor" },
    { 30, 25099, true, "Colliery Underground Third Floor" },
    { 31, 25764, true, "Dorter Trade City" },
    { 32, 26042, true, "Slums in Dorter" },
    { 33, 26229, true, "Hospital in Slums" },
    { 34, 26362, true, "Cellar of Sand Mouse" },
    { 35, 27028, true, "Zaland Fort City" },
    { 36, 27643, true, "Church Outside of Town" },
    { 37, 27793, true, "Ruins Outside Zaland" },
    { 38, 28467, true, "Goug Machine City" },
    { 39, 28555, true, "Underground Passage in Goland" },
    { 40, 29165, true, "Slums in Goug" },
    { 41, 29311, true, "Besrodio's House" },
    { 42, 29653, true, "Warjilis Trade City" },
    { 43, 29807, true, "Port of Warjilis" },
    { 44, 30473, true, "Bervenia Free City" },
    { 45, 30622, true, "Ruins of Zeltennia Castle's Church" },
    { 46, 30966, true, "Cemetery of Heavenly Knight, Balbanes" },
    { 47, 31697, true, "Zarghidas Trade City" },
    { 48, 32365, true, "Slums of Zarghidas" },
    { 49, 33032, true, "Fort Zeakden" },
    { 50, 33701, true, "St. Murond Temple" },
    { 51, 34349, true, "St. Murond Temple" },
    { 52, 34440, true, "Chapel of St. Murond Temple" },
    { 53, 34566, true, "Entrance to Death City" },
    { 54, 34647, true, "Lost Sacred Precincts" },
    { 55, 34745, true, "Graveyard of Airships" },
    { 56, 35350, true, "Orbonne Monastery" },
    { 57, 35436, true, "Underground Book Storage First Floor" },
    { 58, 35519, true, "Underground Book Storage Second Floor" },
    { 59, 35603, true, "Underground Book Storage Third Floor" },
    { 60, 35683, true, "Underground Book Storage Fourth Floor" },
    { 61, 35765, true, "Underground Book Storage Fifth Floor" },
    { 62, 36052, true, "Chapel of Orbonne Monastery" },
    { 63, 36394, true, "Golgorand Execution Site" },
    { 64, 36530, true, "In Front of Bethla Garrison's Sluice" },
    { 65, 36612, true, "Granary of Bethla Garrison" },
    { 66, 37214, true, "South Wall of Bethla Garrison" },
    { 67, 37817, true, "North Wall of Bethla Garrison" },
    { 68, 38386, true, "Bethla Garrison" },
    { 69, 38473, true, "Murond Death City" },
    { 70, 38622, true, "Nelveska Temple" },
    { 71, 39288, true, "Dolbodar Swamp" },
    { 72, 39826, true, "Fovoham Plains" },
    { 73, 40120, true, "Inside of Windmill Shed" },
    { 74, 40724, true, "Sweegy Woods" },
    { 75, 41391, true, "Bervenia Volcano" },
    { 76, 41865, true, "Zeklaus Desert" },
    { 77, 42532, true, "Lenalia Plateau" },
    { 78, 43200, true, "Zigolis Swamp" },
    { 79, 43295, true, "Yuguo Woods" },
    { 80, 43901, true, "Araguay Woods" },
    { 81, 44569, true, "Grog Hill" },
    { 82, 45044, true, "Bed Desert" },
    { 83, 45164, true, "Zirekile Falls" },
    { 84, 45829, true, "Bariaus Hill" },
    { 85, 46498, true, "Mandalia Plains" },
    { 86, 47167, true, "Doguola Pass" },
    { 87, 47260, true, "Bariaus Valley" },
    { 88, 47928, true, "Finath River" },
    { 89, 48595, true, "Poeskas Lake" },
    { 90, 49260, true, "Germinas Peak" },
    { 91, 49538, true, "Thieves Fort" },
    { 92, 50108, true, "Igros-Belouve Residence" },
    { 93, 50387, true, "Broke Down Shed-Wooden Building" },
    { 94, 50554, true, "Broke Down Shed-Stone Building" },
    { 95, 51120, true, "Church" },
    { 96, 51416, true, "Pub" },
    { 97, 52082, true, "Inside Castle Gate in Lesalia" },
    { 98, 52749, true, "Outside Castle Gate in Lesalia" },
    { 99, 53414, true, "Main Street of Lesalia" },
    { 100, 53502, true, "Public Cemetery" },
    { 101, 53579, true, "Tutorial (1)" },
    { 102, 53659, true, "Tutorial (2)" },
    { 103, 54273, true, "Windmill Shed" },
    { 104, 54359, true, "Belouve Residence" },
    { 105, 54528, true, "TERMINATE" },
    { 106, 54621, true, "DELTA" },
    { 107, 54716, true, "NOGIAS" },
    { 108, 54812, true, "VOYAGE" },
    { 109, 54909, true, "BRIDGE" },
    { 110, 55004, true, "VALKYRIES" },
    { 111, 55097, true, "MLAPAN" },
    { 112, 55192, true, "TIGER" },
    { 113, 55286, true, "HORROR" },
    { 114, 55383, true, "END" },
    { 115, 56051, true, "Banished Fort" },
    { 116, 56123, true, "Arena" },
    { 117, 56201, true, "Unknown" },
    { 118, 56279, true, "Unknown" },
    { 119, 56356, true, "Unknown" },
    { 120, 0, false, "???" },
    { 121, 0, false, "???" },
    { 122, 0, false, "???" },
    { 123, 0, false, "???" },
    { 124, 0, false, "???" },
    { 125, 56435, true, "Unknown" },
    { 126, 0, false, "???" },
    { 127, 0, false, "???" },
};
