#ifndef SCENE_H_
#define SCENE_H_

#include <stdbool.h>

#include "fft.h"
#include "gfx.h"

typedef struct {
    map_t* map;
    model_t model;
    bool center_model;
    int current_scenario;
    int current_map;
} scene_t;

void scene_init(void);
void scene_shutdown(void);

void scene_load_map(int num, map_state_t state);
void scene_load_scenario(int num);

void scene_prev(void);
void scene_next(void);

#endif // SCENE_H_