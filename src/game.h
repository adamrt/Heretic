#ifndef GAME_H_
#define GAME_H_

#include "sokol_app.h"
#include "sokol_time.h"

#include "event.h"
#include "scenario.h"

void game_init(void);
void game_update(void);
void game_input(const sapp_event* event);
void game_shutdown(void);

typedef enum mode_e {
    MODE_MAP,
    MODE_SCENARIO,
} mode_e;

typedef struct {
    mode_e mode;

    struct {
        scenario_t* scenarios;
        event_t* events;
    } fft;

} game_t;

extern game_t g;

#endif // GAME_H_
