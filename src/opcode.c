#include "opcode.h"

// clang-format off
const opcode_desc_t opcode_desc_list[] = {
    [OPCODE_ID_DISPLAYMESSAGE]     = { 0x10, "DisplayMessage", { 1, 1, 2, 1, 1, 1, 2, 2, 2, 1 }, 10 },
    [OPCODE_ID_UNITANIM]           = { 0x11, "UnitAnim", { 1, 1, 1, 1, 1 }, 5 },
    [OPCODE_ID_UNKNOWN_0x12]       = { 0x12, "Unknown(0x12)", { 2 }, 1 }, // Chatper 3 Initialization
    [OPCODE_ID_CHANGEMAPBETA]      = { 0x13, "ChangeMapBeta", { 1, 1 }, 2 },
    [OPCODE_ID_PAUSE]              = { 0x16, "Pause", { 0 }, 0 },
    [OPCODE_ID_EFFECT]             = { 0x18, "Effect", { 2, 1, 1, 1, 1 }, 5 },
    [OPCODE_ID_CAMERA]             = { 0x19, "Camera", { 2, 2, 2, 2, 2, 2, 2, 2 }, 8 },
    [OPCODE_ID_MAPDARKNESS]        = { 0x1A, "MapDarkness", { 1, 1, 1, 1, 1 }, 5 },
    [OPCODE_ID_MAPLIGHT]           = { 0x1B, "MapLight", { 2, 2, 2, 2, 2, 2, 2 }, 7 },
    [OPCODE_ID_EVENTSPEED]         = { 0x1C, "EventSpeed", { 1 }, 1 },
    [OPCODE_ID_CAMERAFUSIONSTART]  = { 0x1D, "CameraFusionStart", { 0 }, 0 },
    [OPCODE_ID_CAMERAFUSIONEND]    = { 0x1E, "CameraFusionEnd", { 0 }, 0 },
    [OPCODE_ID_FOCUS]              = { 0x1F, "Focus", { 1, 1, 1, 1, 1 }, 5 },
    [OPCODE_ID_SOUNDEFFECT]        = { 0x21, "SoundEffect", { 2 }, 1 },
    [OPCODE_ID_SWITCHTRACK]        = { 0x22, "SwitchTrack", { 1, 1, 1 }, 3 },
    [OPCODE_ID_RELOADMAPSTATE]     = { 0x27, "ReloadMapState", { 0 }, 0 },
    [OPCODE_ID_WALKTO]             = { 0x28, "WalkTo", { 1, 1, 1, 1, 1, 1, 1, 1 }, 8 },
    [OPCODE_ID_WAITWALK]           = { 0x29, "WaitWalk", { 1, 1 }, 2 },
    [OPCODE_ID_BLOCKSTART]         = { 0x2A, "BlockStart", { 0 }, 0 },
    [OPCODE_ID_BLOCKEND]           = { 0x2B, "BlockEnd", { 0 }, 0 },
    [OPCODE_ID_FACEUNIT2]          = { 0x2C, "FaceUnit2", { 1, 1, 1, 1, 1, 1, 1 }, 7 },
    [OPCODE_ID_ROTATEUNIT]         = { 0x2D, "RotateUnit", { 1, 1, 1, 1, 1, 1 }, 6 },
    [OPCODE_ID_BACKGROUND]         = { 0x2E, "Background", { 1, 1, 1, 1, 1, 1, 1, 1 }, 8 },
    [OPCODE_ID_COLORBGBETA]        = { 0x31, "ColorBGBeta", { 1, 1, 1, 1, 1 }, 5 },
    [OPCODE_ID_COLORUNIT]          = { 0x32, "ColorUnit", { 1, 1, 1, 1, 1, 1, 1 }, 7 },
    [OPCODE_ID_COLORFIELD]         = { 0x33, "ColorField", { 1, 1, 1, 1, 1 }, 5 },
    [OPCODE_ID_FOCUSSPEED]         = { 0x38, "FocusSpeed", { 2 }, 1 },
    [OPCODE_ID_UNKNOWN_0x39]       = { 0x39, "Unknown(0x39)", { 0 }, 0 },
    [OPCODE_ID_UNKNOWN_0x3A]       = { 0x3A, "Unknown(0x3A)", { 0 }, 0 },
    [OPCODE_ID_SPRITEMOVE]         = { 0x3B, "SpriteMove", { 1, 1, 2, 2, 2, 1, 1, 2 }, 8 },
    [OPCODE_ID_WEATHER]            = { 0x3C, "Weather", { 1, 1 }, 2 },
    [OPCODE_ID_REMOVEUNIT]         = { 0x3D, "RemoveUnit", { 1, 1 }, 2 },
    [OPCODE_ID_COLORSCREEN]        = { 0x3E, "ColorScreen", { 1, 1, 1, 1, 1, 1, 1, 2 }, 8 },
    [OPCODE_ID_UNKNOWN_0x40]       = { 0x40, "Unknown(0x40)", { 0 }, 0 },
    [OPCODE_ID_EARTHQUAKESTART]    = { 0x41, "EarthquakeStart", { 1, 1, 1, 1 }, 4 },
    [OPCODE_ID_EARTHQUAKEEND]      = { 0x42, "EarthquakeEnd", { 0 }, 0 },
    [OPCODE_ID_CALLFUNCTION]       = { 0x43, "CallFunction", { 1 }, 1 },
    [OPCODE_ID_DRAW]               = { 0x44, "Draw", { 1, 1 }, 2 },
    [OPCODE_ID_ADDUNIT]            = { 0x45, "AddUnit", { 1, 1, 1 }, 3 },
    [OPCODE_ID_ERASE]              = { 0x46, "Erase", { 1, 1 }, 2 },
    [OPCODE_ID_ADDGHOSTUNIT]       = { 0x47, "AddGhostUnit", { 1, 1, 1, 1, 1, 1, 1, 1 }, 8 },
    [OPCODE_ID_WAITADDUNIT]        = { 0x48, "WaitAddUnit", { 0 }, 0 },
    [OPCODE_ID_ADDUNITSTART]       = { 0x49, "AddUnitStart", { 0 }, 0 },
    [OPCODE_ID_ADDUNITEND]         = { 0x4A, "AddUnitEnd", { 0 }, 0 },
    [OPCODE_ID_WAITADDUNITEND]     = { 0x4B, "WaitAddUnitEnd", { 0 }, 0 },
    [OPCODE_ID_CHANGEMAP]          = { 0x4C, "ChangeMap", { 1, 1 }, 2 },
    [OPCODE_ID_REVEAL]             = { 0x4D, "Reveal", { 1 }, 1 },
    [OPCODE_ID_UNITSHADOW]         = { 0x4E, "UnitShadow", { 1, 1, 1 }, 3 },
    [OPCODE_ID_PORTRAITCOL]        = { 0x50, "PortraitCol", { 1 }, 1 },
    [OPCODE_ID_CHANGEDIALOG]       = { 0x51, "ChangeDialog", { 1, 2, 1, 1 }, 4 },
    [OPCODE_ID_FACEUNIT]           = { 0x53, "FaceUnit", { 1, 1, 1, 1, 1, 1, 1 }, 7 },
    [OPCODE_ID_USE3DOBJECT]        = { 0x54, "Use3DObject", { 1, 1 }, 2 },
    [OPCODE_ID_USEFIELDOBJECT]     = { 0x55, "UseFieldObject", { 1, 1 }, 2 },
    [OPCODE_ID_WAIT3DOBJECT]       = { 0x56, "Wait3DObject", { 0 }, 0 },
    [OPCODE_ID_WAITFIELDOBJECT]    = { 0x57, "WaitFieldObject", { 0 }, 0 },
    [OPCODE_ID_LOADEVTCHR]         = { 0x58, "LoadEVTCHR", { 1, 1, 1 }, 3 },
    [OPCODE_ID_SAVEEVTCHR]         = { 0x59, "SaveEVTCHR", { 1 }, 1 },
    [OPCODE_ID_SAVEEVTCHRCLEAR]    = { 0x5A, "SaveEVTCHRClear", { 1 }, 1 },
    [OPCODE_ID_LOADEVTCHRCLEAR]    = { 0x5B, "LoadEVTCHRClear", { 1 }, 1 },
    [OPCODE_ID_WARPUNIT]           = { 0x5F, "WarpUnit", { 1, 1, 1, 1, 1, 1 }, 6 },
    [OPCODE_ID_FADESOUND]          = { 0x60, "FadeSound", { 1, 1 }, 2 },
    [OPCODE_ID_CAMERASPEEDCURVE]   = { 0x63, "CameraSpeedCurve", { 1 }, 1 },
    [OPCODE_ID_WAITROTATEUNIT]     = { 0x64, "WaitRotateUnit", { 1, 1 }, 2 },
    [OPCODE_ID_WAITROTATEALL]      = { 0x65, "WaitRotateAll", { 0 }, 0 },
    [OPCODE_ID_UNKNOWN_0x66]       = { 0x66, "Unknown(0x66)", { 0 }, 0 },
    [OPCODE_ID_MIRRORSPRITE]       = { 0x68, "MirrorSprite", { 1, 1, 1 }, 3 },
    [OPCODE_ID_FACETILE]           = { 0x69, "FaceTile", { 1, 1, 1, 1, 1, 1, 1, 1 }, 8 },
    [OPCODE_ID_EDITBGSOUND]        = { 0x6A, "EditBGSound", { 1, 1, 1, 1, 1 }, 5 },
    [OPCODE_ID_BGSOUND]            = { 0x6B, "BGSound", { 1, 1, 1, 1, 1 }, 5 },
    [OPCODE_ID_UNKNOWN_0x6D]       = { 0x6D, "Unknown(0x6D)", { 1, 1 }, 2 },
    [OPCODE_ID_SPRITEMOVEBETA]     = { 0x6E, "SpriteMoveBeta", { 1, 1, 2, 2, 2, 1, 1, 2 }, 8 },
    [OPCODE_ID_WAITSPRITEMOVE]     = { 0x6F, "WaitSpriteMove", { 1, 1 }, 2 },
    [OPCODE_ID_JUMP]               = { 0x70, "Jump", { 1, 1, 1, 1 }, 4 },
    [OPCODE_ID_UNKNOWN_0x71]       = { 0x71, "Unknown(0x71)", { 1, 1 }, 2 },
    [OPCODE_ID_UNKNOWN_0x73]       = { 0x73, "Unknown(0x73)", { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 14 },
    [OPCODE_ID_UNKNOWN_0x75]       = { 0x75, "Unknown(0x75)", { 0 }, 0 },
    [OPCODE_ID_DARKSCREEN]         = { 0x76, "DarkScreen", { 1, 1, 1, 1, 1, 1 }, 6 },
    [OPCODE_ID_REMOVEDARKSCREEN]   = { 0x77, "RemoveDarkScreen", { 0 }, 0 },
    [OPCODE_ID_DISPLAYCONDITIONS]  = { 0x78, "DisplayConditions", { 1, 1 }, 2 },
    [OPCODE_ID_WALKTOANIM]         = { 0x79, "WalkToAnim", { 1, 1, 2 }, 3 },
    [OPCODE_ID_DISMISSUNIT]        = { 0x7A, "DismissUnit", { 1, 1 }, 2 },
    [OPCODE_ID_UNKNOWN_0x7B]       = { 0x7B, "Unknown(0x7B)", { 1, 1 }, 2 },
    [OPCODE_ID_UNKNOWN_0x7C]       = { 0x7C, "Unknown(0x7C)", { 0 }, 0 },
    [OPCODE_ID_SHOWGRAPHIC]        = { 0x7D, "ShowGraphic", { 1 }, 1 },
    [OPCODE_ID_WAITVALUE]          = { 0x7E, "WaitValue", { 2, 2 }, 2 },
    [OPCODE_ID_EVTCHRPALETTE]      = { 0x7F, "EVTCHRPalette", { 1, 1, 1, 1 }, 4 },
    [OPCODE_ID_MARCH]              = { 0x80, "March", { 1, 1, 1 }, 3 },
    [OPCODE_ID_UNKNOWN_0x82]       = { 0x82, "Unknown(0x82)", { 0 }, 0 },
    [OPCODE_ID_CHANGESTATS]        = { 0x83, "ChangeStats", { 1, 1, 1, 2 }, 4 },
    [OPCODE_ID_PLAYTUNE]           = { 0x84, "PlayTune", { 1 }, 1 },
    [OPCODE_ID_UNLOCKDATE]         = { 0x85, "UnlockDate", { 1 }, 1 },
    [OPCODE_ID_TEMPWEAPON]         = { 0x86, "TempWeapon", { 1, 1, 1 }, 3 },
    [OPCODE_ID_ARROW]              = { 0x87, "Arrow", { 1, 1, 1, 1 }, 4 },
    [OPCODE_ID_MAPUNFREEZE]        = { 0x88, "MapUnfreeze", { 0 }, 0 },
    [OPCODE_ID_MAPFREEZE]          = { 0x89, "MapFreeze", { 0 }, 0 },
    [OPCODE_ID_EFFECTSTART]        = { 0x8A, "EffectStart", { 0 }, 0 },
    [OPCODE_ID_EFFECTEND]          = { 0x8B, "EffectEnd", { 0 }, 0 },
    [OPCODE_ID_UNITANIMROTATE]     = { 0x8C, "UnitAnimRotate", { 1, 1, 1, 1, 1, 1 }, 6 },
    [OPCODE_ID_WAITGRAPHICPRINT]   = { 0x8E, "WaitGraphicPrint", { 0 }, 0 },
    [OPCODE_ID_UNKNOWN_0x8F]       = { 0x8F, "Unknown(0x8F)", { 0 }, 0 },
    [OPCODE_ID_UNKNOWN_0x90]       = { 0x90, "Unknown(0x90)", { 0 }, 0 },
    [OPCODE_ID_SHOWMAPTITLE]       = { 0x91, "ShowMapTitle", { 1, 1, 1 }, 3 },
    [OPCODE_ID_INFLICTSTATUS]      = { 0x92, "InflictStatus", { 1, 1, 1, 1, 1 }, 5 },
    [OPCODE_ID_UNKNOWN_0x93]       = { 0x93, "Unknown(0x93)", { 0 }, 0 },
    [OPCODE_ID_TELEPORTOUT]        = { 0x94, "TeleportOut", { 1, 1 }, 2 },
    [OPCODE_ID_APPENDMAPSTATE]     = { 0x96, "AppendMapState", { 0 }, 0 },
    [OPCODE_ID_RESETPALETTE]       = { 0x97, "ResetPalette", { 1, 1 }, 2 },
    [OPCODE_ID_TELEPORTIN]         = { 0x98, "TeleportIn", { 1, 1 }, 2 },
    [OPCODE_ID_BLUEREMOVEUNIT]     = { 0x99, "BlueRemoveUnit", { 1, 1 }, 2 },
    [OPCODE_ID_LTE]                = { 0xA0, "LTE", { 0 }, 0 },
    [OPCODE_ID_GTE]                = { 0xA1, "GTE", { 0 }, 0 },
    [OPCODE_ID_EQ]                 = { 0xA2, "EQ", { 0 }, 0 },
    [OPCODE_ID_NEQ]                = { 0xA3, "NEQ", { 0 }, 0 },
    [OPCODE_ID_LT]                 = { 0xA4, "LT", { 0 }, 0 },
    [OPCODE_ID_GT]                 = { 0xA5, "GT", { 0 }, 0 },
    [OPCODE_ID_ADD]                = { 0xB0, "ADD", { 2, 2 }, 2 },
    [OPCODE_ID_ADDVAR]             = { 0xB1, "ADDVar", { 2, 2 }, 2 },
    [OPCODE_ID_SUB]                = { 0xB2, "SUB", { 2, 2 }, 2 },
    [OPCODE_ID_SUBVAR]             = { 0xB3, "SUBVar", { 2, 2 }, 2 },
    [OPCODE_ID_MULT]               = { 0xB4, "MULT", { 2, 2 }, 2 },
    [OPCODE_ID_MULTVAR]            = { 0xB5, "MULTVar", { 2, 2 }, 2 },
    [OPCODE_ID_DIV]                = { 0xB6, "DIV", { 2, 2 }, 2 },
    [OPCODE_ID_DIVVAR]             = { 0xB7, "DIVVar", { 2, 2 }, 2 },
    [OPCODE_ID_MOD]                = { 0xB8, "MOD", { 2, 2 }, 2 },
    [OPCODE_ID_MODVAR]             = { 0xB9, "MODVar", { 2, 2 }, 2 },
    [OPCODE_ID_AND]                = { 0xBA, "AND", { 2, 2 }, 2 },
    [OPCODE_ID_ANDVAR]             = { 0xBB, "ANDVar", { 2, 2 }, 2 },
    [OPCODE_ID_OR]                 = { 0xBC, "OR", { 2, 2 }, 2 },
    [OPCODE_ID_ORVAR]              = { 0xBD, "ORVar", { 2, 2 }, 2 },
    [OPCODE_ID_ZERO]               = { 0xBE, "ZERO", { 2 }, 1 },
    [OPCODE_ID_JUMPFORWARDIFZERO]  = { 0xD0, "JumpForwardIfZero", { 1 }, 1 },
    [OPCODE_ID_JUMPFORWARD]        = { 0xD1, "JumpForward", { 1 }, 1 },
    [OPCODE_ID_FORWARDTARGET]      = { 0xD2, "ForwardTarget", { 1 }, 1 },
    [OPCODE_ID_JUMPBACK]           = { 0xD3, "JumpBack", { 1 }, 1 },
    [OPCODE_ID_UNKNOWN_0xD4]       = { 0xD4, "Unknown(0xD4)", { 0 }, 0 },
    [OPCODE_ID_BACKTARGET]         = { 0xD5, "BackTarget", { 1 }, 1 },
    [OPCODE_ID_EVENTEND]           = { 0xDB, "EventEnd", { 0 }, 0 },
    [OPCODE_ID_EVENTEND2]          = { 0xE3, "EventEnd2", { 0 }, 0 },
    [OPCODE_ID_WAITFORINSTRUCTION] = { 0xE5, "WaitForInstruction", { 1, 1 }, 2 },
    [OPCODE_ID_UNKNOWN_0xF0]       = { 0xF0, "Unknown(0xF0)", { 0 }, 0 },
    [OPCODE_ID_WAIT]               = { 0xF1, "Wait", { 2 }, 1 },
    [OPCODE_ID_PAD]                = { 0xF2, "Pad", { 0 }, 0 },
};
// clang-format on