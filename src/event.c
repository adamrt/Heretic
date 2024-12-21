#include <string.h>

#include "defines.h"
#include "event.h"
#include "instruction.h"
#include "io.h"
#include "memory.h"
#include "message.h"
#include "span.h"
#include "util.h"

static struct {
    event_t* events;
} _state;

static event_t read_event(span_t*);

void event_init(void) {
    // Allocate for all io resources
    _state.events = memory_allocate(EVENT_COUNT * sizeof(event_t));

    span_t file = io_file_test_evt();
    for (usize i = 0; i < EVENT_COUNT; i++) {
        span_t span = {
            .data = file.data + (i * EVENT_SIZE),
            .size = EVENT_SIZE,
        };
        _state.events[i] = read_event(&span);
    }
}

void event_shutdown(void) {
    for (usize i = 0; i < EVENT_COUNT; i++) {
        memory_free(_state.events[i].messages);
        memory_free(_state.events[i].instructions);
    }
    memory_free(_state.events);
}

event_t event_get_event(int id) {
    ASSERT(id < EVENT_COUNT, "Event id %d out of bounds", id);
    return _state.events[id];
}

static event_t read_event(span_t* span) {
    event_t event = { 0 };

    u32 text_offset = span_read_u32(span);
    bool valid = text_offset != 0xF2F2F2F2;
    if (!valid) {
        return event;
    }

    memcpy(event.data, span->data, EVENT_SIZE);

    usize text_size = EVENT_SIZE - text_offset;
    span_t text_span = {
        .data = event.data + text_offset,
        .size = text_size,
    };
    event.messages = memory_allocate(MESSAGES_LEN);
    event.messages_len = read_messages(&text_span, event.messages);

    usize code_size = text_offset - 4;
    span_t code_span = {
        .data = event.data + 4,
        .size = code_size,
    };
    event.instructions = memory_allocate(INSTRUCTION_MAX * sizeof(instruction_t));
    event.instruction_count = read_instructions(&code_span, event.instructions);

    event.valid = valid;
    return event;
}

// These are event ids, names and if they are usable. For viewing scenarios, we
// only use events that have text and code related to them. We index into this
// list by scenario_t.event_id, but some of these are never indexed by a
// scenario. They are documented below starting at 0x0190. Seems to be the game
// over event and events related to random battles.
//
// Names from: https://github.com/Glain/FFTPatcher/blob/master/EntryEdit/EntryData/PSX/ScenarioNames.xml
event_desc_t event_desc_list[EVENT_COUNT] = {
    { 0x0000, false, "Empty" },
    { 0x0001, false, "Orbonne Prayer (Setup)" },
    { 0x0002, true, "Orbonne Prayer" },
    { 0x0003, false, "Orbonne Battle (Setup)" },
    { 0x0004, true, "Orbonne Battle" },
    { 0x0005, true, "Orbonne Battle (Gafgarion and Agrias chat)" },
    { 0x0006, true, "Orbonne Battle (Abducting the Princess)" },
    { 0x0007, false, "Military Academy (Setup)" },
    { 0x0008, true, "Military Academy" },
    { 0x0009, false, "Gariland Fight (Setup)" },
    { 0x000A, true, "Gariland Fight" },
    { 0x000B, true, "Gariland Fight (Ramza, Delita, Thief Chat)" },
    { 0x000C, true, "Gariland Fight (Ramza talking about honest lives)" },
    { 0x000D, false, "Balbanes's Death (Setup)" },
    { 0x000E, true, "Balbanes's Death" },
    { 0x000F, false, "Mandalia Plains (Setup)" },
    { 0x0010, true, "Mandalia Plains (Options Given)" },
    { 0x0011, true, "Mandalia Plains (Destroy Corps Chosen)" },
    { 0x0012, true, "Mandalia Plains (Save Algus Chosen)" },
    { 0x0013, true, "Mandalia Plains (Algus First Turn)" },
    { 0x0014, true, "Mandalia Plains (Algus KO'd, Destroy Chosen)" },
    { 0x0015, true, "Mandalia Plains (Algus KO'd, Save Chosen)" },
    { 0x0016, true, "Mandalia Plains (Victory, Algus KO'd)" },
    { 0x0017, true, "Mandalia Plains (Victory, Algus Alive)" },
    { 0x0018, false, "Introducing Algus (Setup)" },
    { 0x0019, true, "Introducing Algus" },
    { 0x001A, false, "Returning to Igros (Setup)" },
    { 0x001B, true, "Returning to Igros" },
    { 0x001C, false, "Family Meeting (Setup)" },
    { 0x001D, true, "Family Meeting" },
    { 0x001E, false, "Sweegy Woods (Setup)" },
    { 0x001F, true, "Sweegy Woods" },
    { 0x0020, true, "Sweegy Woods (Victory)" },
    { 0x0021, false, "Dorter Trade City1 (Setup)" },
    { 0x0022, true, "Dorter Trade City1" },
    { 0x0023, true, "Dorter Trade City1 (Algus and Delita talk)" },
    { 0x0024, true, "Dorter Trade City1 (Victory)" },
    { 0x0025, false, "Interrogation (Setup)" },
    { 0x0026, true, "Interrogation" },
    { 0x0027, false, "Sand Rat Cellar (Setup)" },
    { 0x0028, true, "Sand Rat Cellar" },
    { 0x0029, true, "Sand Rat Cellar (Victory)" },
    { 0x002A, false, "Gustav vs. Wiegraf (Setup)" },
    { 0x002B, true, "Gustav vs. Wiegraf" },
    { 0x002C, false, "Larg's Praise (Setup)" },
    { 0x002D, true, "Larg's Praise" },
    { 0x002E, false, "Miluda1 (Setup)" },
    { 0x002F, true, "Miluda1" },
    { 0x0030, true, "Miluda1 (Miluda and Algus arguing)" },
    { 0x0031, true, "Miluda1 (Delita talking)" },
    { 0x0032, true, "Miluda1 (Victory)" },
    { 0x0033, false, "Releasing Miluda (Setup)" },
    { 0x0034, true, "Releasing Miluda" },
    { 0x0035, false, "Attack on the Beoulves (Setup)" },
    { 0x0036, true, "Attack on the Beoulves" },
    { 0x0037, false, "Meeting with bedridden Dycedarg (Setup)" },
    { 0x0038, true, "Meeting with bedridden Dycedarg" },
    { 0x0039, false, "Expelling Algus (Setup)" },
    { 0x003A, true, "Expelling Algus" },
    { 0x003B, false, "Reed Whistle (Setup)" },
    { 0x003C, true, "Reed Whistle" },
    { 0x003D, false, "Miluda2 (Setup)" },
    { 0x003E, true, "Miluda2" },
    { 0x003F, true, "Miluda2 (Delita talking with Miluda)" },
    { 0x0040, true, "Miluda2 (Miluda half HP)" },
    { 0x0041, true, "Miluda2 (Ramza debating with Miluda)" },
    { 0x0042, true, "Miluda2 (Ramza pleading with Miluda)" },
    { 0x0043, true, "Miluda2 (Miluda's Death)" },
    { 0x0044, false, "Wiegraf berating Golagros (Setup)" },
    { 0x0045, true, "Wiegraf berating Golagros" },
    { 0x0046, false, "Wiegraf1 (Setup)" },
    { 0x0047, true, "Wiegraf1" },
    { 0x0048, true, "Wiegraf1 (Delita, Ramza, Wiegraf talk)" },
    { 0x0049, true, "Wiegraf1 (Ramza and Wiegraf debate)" },
    { 0x004A, true, "Wiegraf1 (Ramza and Wiegraf talk)" },
    { 0x004B, true, "Wiegraf1 (Victory)" },
    { 0x004C, false, "Finding Teta Missing (Setup)" },
    { 0x004D, true, "Finding Teta Missing" },
    { 0x004E, false, "Fort Zeakden (Setup)" },
    { 0x004F, true, "Fort Zeakden" },
    { 0x0050, true, "Fort Zeakden (Algus, Ramza round 1)" },
    { 0x0051, true, "Fort Zeakden (Algus, Ramza round 2)" },
    { 0x0052, true, "Fort Zeakden (Algus, Ramza round 3)" },
    { 0x0053, true, "Fort Zeakden (Destroy Chosen at Mandalia)" },
    { 0x0054, true, "Fort Zeakden (Save Chosen at Mandalia)" },
    { 0x0055, true, "Fort Zeakden (Delita's First Turn)" },
    { 0x0056, true, "Fort Zeakden (Algus, Delita round 1)" },
    { 0x0057, true, "Fort Zeakden (Ramza, Delita talking)" },
    { 0x0058, true, "Fort Zeakden (Victory)" },
    { 0x0059, false, "Partings (Setup)" },
    { 0x005A, true, "Partings" },
    { 0x005B, false, "Deep Dungeon NOGIAS (Setup)" },
    { 0x005C, false, "Deep Dungeon NOGIAS (Battle)" },
    { 0x005D, true, "Deep Dungeon Panel Found" },
    { 0x005E, false, "Deep Dungeon (Victory - Used for all Floors)" },
    { 0x005F, false, "Deep Dungeon TERMINATE(Setup)" },
    { 0x0060, false, "Deep Dungeon TERMINATE (Battle)" },
    { 0x0061, false, "Deep Dungeon DELTA (Setup)" },
    { 0x0062, false, "Deep Dungeon DELTA (Battle)" },
    { 0x0063, false, "Deep Dungeon VALKYRIES (Setup)" },
    { 0x0064, false, "Deep Dungeon VALKYRIES (Battle)" },
    { 0x0065, false, "Deep Dungeon MLAPAN (Setup)" },
    { 0x0066, false, "Deep Dungeon MLAPAN (Battle)" },
    { 0x0067, false, "Deep Dungeon TIGER (Setup)" },
    { 0x0068, false, "Deep Dungeon TIGER (Battle)" },
    { 0x0069, false, "Deep Dungeon BRIDGE (Setup)" },
    { 0x006A, false, "Deep Dungeon BRIDGE (Battle)" },
    { 0x006B, false, "Deep Dungeon VOYAGE (Setup)" },
    { 0x006C, false, "Deep Dungeon VOYAGE (Battle)" },
    { 0x006D, false, "Deep Dungeon HORROR (Setup)" },
    { 0x006E, false, "Deep Dungeon HORROR (Battle)" },
    { 0x006F, false, "Elidibs (Setup)" },
    { 0x0070, true, "Elidibs" },
    { 0x0071, true, "Elidibs (Victory)" },
    { 0x0072, false, "Deep Dungeon END (Setup)" },
    { 0x0073, false, "Deep Dungeon END (Battle)" },
    { 0x0074, false, "Chapter 2 Start (Setup)" },
    { 0x0075, true, "Chapter 2 Start" },
    { 0x0076, false, "Dorter2 (Setup)" },
    { 0x0077, true, "Dorter2" },
    { 0x0078, true, "Dorter2 (Victory)" },
    { 0x0079, false, "Araguay Woods (Setup)" },
    { 0x007A, true, "Araguay Woods (Options Given)" },
    { 0x007B, true, "Araguay Woods (Kill Enemies Chosen)" },
    { 0x007C, true, "Araguay Woods (Save Boco Chosen)" },
    { 0x007D, true, "Araguay Woods (Boco KO'd, Kill Enemies Chosen)" },
    { 0x007E, true, "Araguay Woods (Boco KO'd, Save Boco Chosen)" },
    { 0x007F, true, "Araguay Woods (Victory)" },
    { 0x0080, false, "Zirekile Falls (Setup)" },
    { 0x0081, true, "Zirekile Falls" },
    { 0x0082, true, "Zirekile Falls (Gafgarion and Agrias talk)" },
    { 0x0083, true, "Zirekile Falls (Gafgarion, Ramza, Delita, talk)" },
    { 0x0084, true, "Zirekile Falls (Delita, Ovelia talk)" },
    { 0x0085, true, "Zirekile Falls (Ovelia's Death)" },
    { 0x0086, true, "Zirekile Falls (Gafgarion and Ramza arguing)" },
    { 0x0087, true, "Zirekile Falls (Gafgarion retreat)" },
    { 0x0088, false, "Zirekile Falls (Victory)" },
    { 0x0089, false, "Ovelia Joins (Setup)" },
    { 0x008A, true, "Ovelia Joins" },
    { 0x008B, false, "Zalamd Fort City (Setup)" },
    { 0x008C, true, "Zaland Fort City (Options Given)" },
    { 0x008D, true, "Zaland Fort City (Kill Enemies Chosen)" },
    { 0x008E, true, "Zaland Fort City (Save Mustadio Chosen)" },
    { 0x008F, true, "Zaland Fort City (Mustadio KO'd, Kill Chosen)" },
    { 0x0090, true, "Zaland Fort City (Mustadio KO'd, Save Chosen)" },
    { 0x0091, true, "Zaland Fort City (Victory)" },
    { 0x0092, false, "Ramza, Mustadio, Agrias and Ovelia meeting (Setup)" },
    { 0x0093, true, "Ramza, Mustadio, Agrias and Ovelia meeting" },
    { 0x0094, false, "Ruins of Zaland (Setup)" },
    { 0x0095, true, "Ruins of Zaland" },
    { 0x0096, false, "Bariaus Hill (Setup)" },
    { 0x0097, true, "Bariaus Hill" },
    { 0x0098, true, "Bariaus Hill (Victory)" },
    { 0x0099, false, "Dycedarg and Gafgarion Reunion (Setup)" },
    { 0x009A, true, "Dycedarg and Gafgarion Reunion" },
    { 0x009B, false, "Gate of Lionel Castle (Setup)" },
    { 0x009C, true, "Gate of Lionel Castle" },
    { 0x009D, false, "Meeting with Draclay (Setup)" },
    { 0x009E, true, "Meeting with Draclau" },
    { 0x009F, false, "Besrodio Kidnapped (Setup)" },
    { 0x00A0, true, "Besrodio Kidnapped" },
    { 0x00A1, false, "Zigolis Swamp (Setup)" },
    { 0x00A2, true, "Zigolis Swamp" },
    { 0x00A3, true, "Zigolis Swamp (Victory)" },
    { 0x00A4, false, "Goug Machine City Town (Setup)" },
    { 0x00A5, true, "Goug Machine City Town" },
    { 0x00A6, false, "Goug Machine City (Setup)" },
    { 0x00A7, true, "Goug Machine City" },
    { 0x00A8, true, "Goug Machine City (Victory)" },
    { 0x00A9, false, "Besrodio Saved (Setup)" },
    { 0x00AA, true, "Besrodio Saved" },
    { 0x00AB, false, "Warjilis Port (Setup)" },
    { 0x00AC, true, "Warjilis Port" },
    { 0x00AD, false, "Draclau hires Gafgarion (Setup)" },
    { 0x00AE, true, "Draclau hires Gafgarion" },
    { 0x00AF, false, "Bariaus Valley (Setup)" },
    { 0x00B0, true, "Bariaus Valley" },
    { 0x00B1, true, "Bariaus Valley (Agrias and Ramza talk)" },
    { 0x00B2, true, "Bariaus Valley (Agrias Death)" },
    { 0x00B3, true, "Bariaus Valley (Victory)" },
    { 0x00B4, false, "Golgorand Execution Site (Setup)" },
    { 0x00B5, true, "Golgorand Execution Site" },
    { 0x00B6, true, "Golgorand Execution Site (Gafgarion and Agrias talk)" },
    { 0x00B7, true, "Golgorand Execution Site (Gafgarion and Ramza talk first part)" },
    { 0x00B8, true, "Golgorand Execution Site (Gafgarion and Ramza talk second part)" },
    { 0x00B9, true, "Golgorand Execution Site (Gafgarion and Ramza talk third part)" },
    { 0x00BA, true, "Golgorand Execution Site (Gafgarion, Agrias and Ramza talk)" },
    { 0x00BB, true, "Golgorand Execution Site (Gafgarion retreats)" },
    { 0x00BC, true, "Golgorand Execution Site (Victory)" },
    { 0x00BD, false, "Substitute (Setup)" },
    { 0x00BE, true, "Substitute" },
    { 0x00BF, false, "Lionel Castle Gate (Setup)" },
    { 0x00C0, true, "Lionel Castle Gate" },
    { 0x00C1, true, "Lionel Castle Gate (Ramza opens the gate)" },
    { 0x00C2, true, "Lionel Castle Gate (Gafgarion Death)" },
    { 0x00C3, true, "Lionel Castle Gate (Victory)" },
    { 0x00C4, false, "Inside of Lionel Castle (Setup)" },
    { 0x00C5, true, "Inside of Lionel Castle" },
    { 0x00C6, true, "Inside of Lionel Castle (Queklain and Ramza talk)" },
    { 0x00C7, true, "Inside of Lionel Castle (Victory)" },
    { 0x00C8, false, "The Lion War Outbreak (Setup)" },
    { 0x00C9, true, "The Lion War Outbreak" },
    { 0x00CA, false, "Chapter 3 Start (Setup)" },
    { 0x00CB, true, "Chapter 3 Start" },
    { 0x00CC, false, "Goland Coal City (Setup)" },
    { 0x00CD, true, "Goland Coal City" },
    { 0x00CE, true, "Goland Coal City (Olan Death)" },
    { 0x00CF, false, "Goland Coal City (Victory)" },
    { 0x00D0, false, "Goland Coal City post battle (setup)" },
    { 0x00D1, true, "Goland Coal City post battle" },
    { 0x00D2, false, "Steel Ball Found! (Setup)" },
    { 0x00D3, true, "Steel Ball Found!" },
    { 0x00D4, false, "Worker 8 Activated (setup)" },
    { 0x00D5, true, "Worker 8 Activated" },
    { 0x00D6, false, "Summoning Machine Found! (Setup)" },
    { 0x00D7, true, "Summoning Machine Found!" },
    { 0x00D8, false, "Cloud Summoned (Setup)" },
    { 0x00D9, true, "Cloud Summoned" },
    { 0x00DA, false, "Zarghidas (Setup)" },
    { 0x00DB, true, "Zarghidas" },
    { 0x00DC, true, "Zarghidas (Cloud freaking out)" },
    { 0x00DD, true, "Zarghidas (Cloud Death)" },
    { 0x00DE, true, "Zarghidas (Victory)" },
    { 0x00DF, false, "Talk with Zalbag in Lesalia (Setup)" },
    { 0x00E0, true, "Talk with Zalbag in Lesalia" },
    { 0x00E1, false, "Outside Castle Gate in Lesalia Zalmo 1 (Setup)" },
    { 0x00E2, true, "Outside Castle Gate in Lesalia Zalmo 1" },
    { 0x00E3, true, "Outside Castle Gate in Lesalia Zalmo 1 (Zalmo and Ramza talk)" },
    { 0x00E4, true, "Outside Castle Gate in Lesalia Zalmo 1 (Alma and Ramza talk)" },
    { 0x00E5, true, "Outside Castle Gate in Lesalia Zalmo 1 (Victory)" },
    { 0x00E6, false, "Outside Castle Gate in Lesalia Talk with Alma (Setup)" },
    { 0x00E7, true, "Outside Castle Gate in Lesalia Talk with Alma" },
    { 0x00E8, false, "Orbonne Monastery (Setup)" },
    { 0x00E9, true, "Orbonne Monastery" },
    { 0x00EA, false, "Underground Book Storage Second Floor (Setup)" },
    { 0x00EB, true, "Underground Book Storage Second Floor" },
    { 0x00EC, true, "Underground Book Storage Second Floor (Victory)" },
    { 0x00ED, false, "Underground Book Storage Third Floor (Setup)" },
    { 0x00EE, true, "Underground Book Storage Third Floor" },
    { 0x00EF, true, "Underground Book Storage Third Floor (Izlude, Ramza talk first)" },
    { 0x00F0, true, "Underground Book Storage Third Floor (Izlude, Ramza talk second)" },
    { 0x00F1, true, "Underground Book Storage Third Floor (Victory)" },
    { 0x00F2, false, "Underground Book Storage First Floor (Setup)" },
    { 0x00F3, true, "Underground Book Storage First Floor" },
    { 0x00F4, true, "Underground Book Storage First Floor (Wiegraf talk)" },
    { 0x00F5, true, "Underground Book Storage First Floor (Wiegraf, Ramza talk first)" },
    { 0x00F6, true, "Underground Book Storage First Floor (Wiegraf, Ramza talk second)" },
    { 0x00F7, true, "Underground Book Storage First Floor (Victory)" },
    { 0x00F8, false, "Meet Velius (Setup)" },
    { 0x00F9, true, "Meet Velius" },
    { 0x00FA, false, "Malak and the Scriptures (Setup)" },
    { 0x00FB, true, "Malak and the Scriptures (Options Given)" },
    { 0x00FC, true, "Malak and the Scriptures (Yes Chosen)" },
    { 0x00FD, true, "Malak and the Scriptures (No Chosen)" },
    { 0x00FE, false, "Delita swears allegiance to Ovelia (Setup)" },
    { 0x00FF, true, "Delita swears allegiance to Ovelia" },
    { 0x0100, false, "Grog Hill (Setup)" },
    { 0x0101, true, "Grog Hill" },
    { 0x0102, true, "Grog Hill (Victory)" },
    { 0x0103, false, "Meet Again with Olan (Setup)" },
    { 0x0104, true, "Meet again with Olan" },
    { 0x0105, false, "Rescue Rafa (Setup)" },
    { 0x0106, true, "Rescue Rafa" },
    { 0x0107, true, "Rescue Rafa (Malak and Ramza talk)" },
    { 0x0108, true, "Rescue Rafa (Malak, Ninja and Ramza talk)" },
    { 0x0109, true, "Rescue Rafa (Malak Retreat)" },
    { 0x010A, true, "Rescue Rafa (Rafa Death, Malak Present)" },
    { 0x010B, true, "Rescue Rafa (Rafa Death, Malak Retreated)" },
    { 0x010C, true, "Rescue Rafa (Victory)" },
    { 0x010D, false, "Exploding Frog (Setup)" },
    { 0x010E, true, "Exploding Frog" },
    { 0x010F, false, "Yuguo Woods (Setup)" },
    { 0x0110, true, "Yuguo Woods" },
    { 0x0111, true, "Yuguo Woods (Victory)" },
    { 0x0112, false, "Barinten threatens Vormav (Setup)" },
    { 0x0113, true, "Barinten threatens Vormav" },
    { 0x0114, false, "Riovanes Castle Entrance (Setup)" },
    { 0x0115, true, "Riovanes Castle Entrance" },
    { 0x0116, true, "Riovanes Castle Entrance (Rafa, Malak and Ramza talk)" },
    { 0x0117, true, "Riovanes Castle Entrance (Malak Defeated)" },
    { 0x0118, true, "Riovanes Castle Entrance (Rafa Defeated)" },
    { 0x0119, true, "Riovanes Castle Entrance (Victory)" },
    { 0x011A, false, "Escaping Alma (Setup)" },
    { 0x011B, true, "Escaping Alma" },
    { 0x011C, false, "Inside of Riovanes Castle (Setup)" },
    { 0x011D, true, "Inside of Riovanes Castle" },
    { 0x011E, true, "Inside of Riovanes Castle (Wiegraf and Ramza talk)" },
    { 0x011F, true, "Inside of Riovanes Castle (Here comes Velius)" },
    { 0x0120, true, "Inside of Riovanes Castle (Victory)" },
    { 0x0121, false, "Ajora's vessel (Setup)" },
    { 0x0122, true, "Ajora's vessel" },
    { 0x0123, false, "Rooftop of Riovanes Castle (Setup)" },
    { 0x0124, true, "Rooftop of Riovanes Castle" },
    { 0x0125, true, "Rooftop of Riovanes Castle (Rafa Death)" },
    { 0x0126, true, "Rooftop of Riovanes Castle (Victory)" },
    { 0x0127, false, "Reviving Malak (Setup)" },
    { 0x0128, true, "Reviving Malak" },
    { 0x0129, false, "Searching for Alma (Setup)" },
    { 0x012A, true, "Searching for Alma" },
    { 0x012B, false, "Things Obtained (Setup)" },
    { 0x012C, true, "Things Obtained" },
    { 0x012D, false, "Underground Book Storage Fourth Floor (Setup)" },
    { 0x012E, true, "Underground Book Storage Fourth Floor" },
    { 0x012F, true, "Underground Book Storage Fourth Floor (Victory)" },
    { 0x0130, false, "Underground Book Storage Fifth Floor (Setup)" },
    { 0x0131, true, "Underground Book Storage Fifth Floor" },
    { 0x0132, true, "Underground Book Storage Fifth Floor (Rofel and Ramza talk)" },
    { 0x0133, true, "Underground Book Storage Fifth Floor (Victory)" },
    { 0x0134, false, "Entrance to the other world (Setup)" },
    { 0x0135, true, "Entrance to the other world" },
    { 0x0136, false, "Murond Death City (Setup)" },
    { 0x0137, true, "Murond Death City" },
    { 0x0138, true, "Murond Death City (Kletian and Ramza talk)" },
    { 0x0139, true, "Murond Death City (Victory)" },
    { 0x013A, false, "Lost Sacred Precincts (Setup)" },
    { 0x013B, true, "Lost Sacred Precincts" },
    { 0x013C, true, "Lost Sacred Precincts (Balk and Ramza talk)" },
    { 0x013D, true, "Lost Sacred Precincts (Victory)" },
    { 0x013E, false, "Graveyard of Airships (Setup)" },
    { 0x013F, true, "Graveyard of Airships" },
    { 0x0140, true, "Graveyard of Airships (Hashmalum and Ramza talk)" },
    { 0x0141, true, "Graveyard of Airships (Victory)" },
    { 0x0142, false, "Graveyard of Airships (Setup)" },
    { 0x0143, true, "Graveyard of Airships" },
    { 0x0144, true, "Graveyard of Airships (Here comes Altima 2)" },
    { 0x0145, true, "Graveyard of Airships (Victory)" },
    { 0x0146, false, "Reunion and Beyond" },
    { 0x0147, true, "Reunion and beyond" },
    { 0x0148, false, "Those Who Squirm In Darkness (Setup)" },
    { 0x0149, true, "Those Who Squirm in Darkness" },
    { 0x014A, false, "A Man with the Holy Stone (Setup)" },
    { 0x014B, true, "A Man with the Holy Stone" },
    { 0x014C, false, "Doguola Pass (Setup)" },
    { 0x014D, true, "Doguola Pass" },
    { 0x014E, false, "Doguola Pass (Victory)" },
    { 0x014F, false, "Bervenia Free City (Setup)" },
    { 0x0150, true, "Bervenia Free City" },
    { 0x0151, true, "Bervenia Free City (Meliadoul and Ramza talk first part)" },
    { 0x0152, true, "Bervenia Free City (Meliadoul and Ramza talk second part)" },
    { 0x0153, true, "Bervenia Free City (Meliadoul and Ramza talk third part)" },
    { 0x0154, true, "Bervenia Free City (Victory)" },
    { 0x0155, false, "Finath River (Setup)" },
    { 0x0156, true, "Finath River" },
    { 0x0157, false, "Finath River (Victory)" },
    { 0x0158, false, "Delita's Thoughts (Setup)" },
    { 0x0159, true, "Delita's Thoughts" },
    { 0x015A, false, "Zalmo II (Setup)" },
    { 0x015B, true, "Zalmo II" },
    { 0x015C, true, "Zalmo II (Zalmo and Delita talk)" },
    { 0x015D, true, "Zalmo II (Zalmo and Ramza talk)" },
    { 0x015E, true, "Zalmo II (Victory)" },
    { 0x015F, false, "Unstoppable Cog (Setup)" },
    { 0x0160, true, "Unstoppable Cog" },
    { 0x0161, false, "Balk I (Setup)" },
    { 0x0162, true, "Balk I" },
    { 0x0163, true, "Balk I (Balk and Ramza talk)" },
    { 0x0164, true, "Balk I (Victory)" },
    { 0x0165, false, "Seized T" },
    { 0x0166, true, "Seized T" },
    { 0x0167, false, "South Wall of Bethla Garrison (Setup)" },
    { 0x0168, true, "South Wall of Bethla Garrison" },
    { 0x0169, true, "South Wall of Bethla Garrison (Victory)" },
    { 0x016A, false, "North Wall of Bethla Garrison (Setup)" },
    { 0x016B, true, "North Wall of Bethla Garrison" },
    { 0x016C, true, "North Wall of Bethla Garrison (Victory)" },
    { 0x016D, false, "Assassination of Prince Larg (Setup)" },
    { 0x016E, true, "Assassination of Prince Larg" },
    { 0x016F, false, "Bethla Sluice (Setup)" },
    { 0x0170, true, "Bethla Sluice" },
    { 0x0171, true, "Bethla Sluice (First lever left)" },
    { 0x0172, true, "Bethla Sluice (Second lever left)" },
    { 0x0173, true, "Bethla Sluice (First lever right)" },
    { 0x0174, true, "Bethla Sluice (Second lever right)" },
    { 0x0175, false, "Rescue of Cid (Setup)" },
    { 0x0176, true, "Rescue of Cid" },
    { 0x0177, false, "Prince Goltana's Final Moments (Setup)" },
    { 0x0178, true, "Prince Goltana's Final Moments" },
    { 0x0179, false, "Germinas Peak (Setup)" },
    { 0x017A, true, "Germinas Peak" },
    { 0x017B, true, "Germinas Peak (Victory)" },
    { 0x017C, false, "Poeskas Lake (Setup)" },
    { 0x017D, true, "Poeskas Lake" },
    { 0x017E, false, "Poeskas Lake (Victory)" },
    { 0x017F, false, "Ambition of Dycedarg (Setup)" },
    { 0x0180, true, "Ambition of Dycedarg" },
    { 0x0181, false, "Outside of Limberry Castle (Setup)" },
    { 0x0182, true, "Outside of Limberry Castle" },
    { 0x0183, true, "Outside of Limberry Castle (Victory)" },
    { 0x0184, false, "Men of Odd Appearance (Setup)" },
    { 0x0185, true, "Men of Odd Appearance" },
    { 0x0186, false, "Elmdor II (Setup)" },
    { 0x0187, true, "Elmdor II" },
    { 0x0188, true, "Elmdor II (Ultima Demon Celia)" },
    { 0x0189, true, "Elmdor II (Ultima Demon Lede)" },
    { 0x018A, true, "Elmdor II (Victory)" },
    { 0x018B, false, "Zalera (Setup)" },
    { 0x018C, true, "Zalera" },
    { 0x018D, true, "Zalera (Zalera, Meliadoul and Ramza talk)" },
    { 0x018E, true, "Zalera (Meliadoul and Ramza talk)" },
    { 0x018F, true, "Zalera (Victory)" },
    // These 10 events are not referenced by a scenario
    { 0x0190, false, "Random Battle Template (Setup)" },
    { 0x0191, false, "Random Battle Template (Initiate)" },
    { 0x0192, false, "Random Battle Template (Victory)" },
    { 0x0193, false, "Empty" },
    { 0x0194, false, "Game Over Event (Plays automatically upon Game Over)" },
    { 0x0195, false, "Empty" },
    { 0x0196, false, "Empty" },
    { 0x0197, false, "Empty" },
    { 0x0198, false, "Empty" },
    { 0x0199, false, "Empty" },
    // End of non-scenario events
    { 0x019A, false, "Tutorial - (Battlefield Control) (Setup)" },
    { 0x019B, true, "Tutorial - (Battlefield Control)" },
    { 0x019C, false, "Tutorial - (Battle) (Setup)" },
    { 0x019D, true, "Tutorial - (Battle)" },
    { 0x019E, false, "Tutorial - (Move and Act) (Setup)" },
    { 0x019F, true, "Tutorial - (Move and Act)" },
    { 0x01A0, false, "Tutorial - (Charge Time Battle) (Setup)" },
    { 0x01A1, true, "Tutorial - (Charge Time Battle)" },
    { 0x01A2, false, "Tutorial - (How to Cast Spells) (Setup)" },
    { 0x01A3, true, "Tutorial - (How to Cast Spells)" },
    { 0x01A4, false, "Tutorial - (Abnormal Status) (Setup)" },
    { 0x01A5, true, "Tutorial - (Abnormal Status)" },
    { 0x01A6, false, "Tutorial - (On-Line Help) (Setup)" },
    { 0x01A7, true, "Tutorial - (On-Line Help)" },
    { 0x01A8, false, "Tutorial - (Options) (Setup)" },
    { 0x01A9, true, "Tutorial - (Options)" },
    { 0x01AA, false, "The Mystery of Lucavi (Setup)" },
    { 0x01AB, true, "The Mystery of Lucavi" },
    { 0x01AC, false, "Delita's Betrayal (Setup)" },
    { 0x01AD, true, "Delita's Betrayal" },
    { 0x01AE, true, "Delita's Betrayal" },
    { 0x01AF, false, "Mosfungus (Setup)" },
    { 0x01B0, true, "Mosfungus" },
    { 0x01B1, false, "At the Gate of the Beoulve Castle (Setup)" },
    { 0x01B2, true, "At the Gate of the Beoulve Castle" },
    { 0x01B3, false, "Adramelk (Setup)" },
    { 0x01B4, true, "Adramelk" },
    { 0x01B5, true, "Adramelk (Zalbag and Ramza talk)" },
    { 0x01B6, true, "Adramelk (Dycedarg and Zalbag talk)" },
    { 0x01B7, true, "Adramelk (Here comes Adramelk)" },
    { 0x01B8, true, "Adramelk (Victory)" },
    { 0x01B9, false, "Funeral's Final Moments (Setup)" },
    { 0x01BA, true, "Funeral's Final Moments" },
    { 0x01BB, false, "St. Murond Temple (Setup)" },
    { 0x01BC, true, "St. Murond Temple" },
    { 0x01BD, true, "St. Murond Temple (Victory)" },
    { 0x01BE, false, "Hall of St. Murond Temple (Setup)" },
    { 0x01BF, true, "Hall of St. Murond Temple" },
    { 0x01C0, true, "Hall of St. Murond Temple (Vormav and Meliadoul talk)" },
    { 0x01C1, true, "Hall of St. Murond Temple (Vormav and Ramza talk)" },
    { 0x01C2, true, "Hall of St. Murond Temple (Victory)" },
    { 0x01C3, false, "Chapel of St. Murond Temple (Setup)" },
    { 0x01C4, true, "Chapel of St. Murond Temple" },
    { 0x01C5, true, "Chapel of St. Murond Temple (Zalbag, Ramza first turn)" },
    { 0x01C6, true, "Chapel of St. Murond Temple (Ramza, Zalbag 50% HP talk)" },
    { 0x01C7, true, "Chapel of St. Murond Temple (Victory)" },
    { 0x01C8, false, "Requiem (Setup)" },
    { 0x01C9, true, "Requiem" },
    { 0x01CA, false, "Zarghidas (Setup)" },
    { 0x01CB, true, "Zarghidas (Options Given)" },
    { 0x01CC, true, "Zarghidas (Don't Buy Flower Chosen)" },
    { 0x01CD, true, "Zarghidas (Buy Flower Chosen)" },
    { 0x01CE, false, "Bar - Deep Dungeon (Setup)" },
    { 0x01CF, true, "Bar - Deep Dungeon" },
    { 0x01D0, false, "Bar - Goland Coal City (Setup)" },
    { 0x01D1, true, "Bar - Goland Coal City (Options Given)" },
    { 0x01D2, true, "Bar - Goland Coal City (Refuse Beowulf's Invitation Chosen)" },
    { 0x01D3, true, "Bar - Goland Coal City (Accept Beowulf's invitation Chosen)" },
    { 0x01D4, false, "Colliery Underground - Third Floor (Setup)" },
    { 0x01D5, false, "Colliery Underground - Third Floor (Battle)" },
    { 0x01D6, false, "Colliery Underground - Third Floor (Victory)" },
    { 0x01D7, false, "Colliery Underground - Second Floor (Setup)" },
    { 0x01D8, false, "Colliery Underground - Second Floor (Battle)" },
    { 0x01D9, false, "Colliery Underground - Second Floor (Victory)" },
    { 0x01DA, false, "Colliery Underground - First Floor (Setup)" },
    { 0x01DB, false, "Colliery Underground - First Floor (Battle)" },
    { 0x01DC, false, "Colliery Underground - First Floor (Victory)" },
    { 0x01DD, false, "Underground Passage in Goland (Setup)" },
    { 0x01DE, true, "Underground Passage in Goland (Battle)" },
    { 0x01DF, true, "Underground Passage in Goland (Reis's Death, Beowulf Alive)" },
    { 0x01E0, false, "Underground Passage in Goland (Reis's Death, Beowulf KO'd)" },
    { 0x01E1, false, "Underground Passage in Goland (Victory)" },
    { 0x01E2, false, "Underground Passage in Goland (Setup)" },
    { 0x01E3, true, "Underground Passage in Goland (Post-Battle)" },
    { 0x01E4, false, "Nelveska Temple (Setup)" },
    { 0x01E5, true, "Nelveska Temple" },
    { 0x01E6, true, "Nelveska Temple (Worker 7 recharging)" },
    { 0x01E7, true, "Nelveska Temple (Victory)" },
    { 0x01E8, false, "Reis Curse (Setup)" },
    { 0x01E9, true, "Reis Curse" },
    { 0x01EA, true, "Bethla Sluice (Late add-in Ramza hint)" },
};
