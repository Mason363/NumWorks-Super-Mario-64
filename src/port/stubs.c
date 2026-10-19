/* What the decompilation's code calls that the calculator has no use for: the N64's video and its display
 * lists, its controllers' and EEPROM's hardware, sound and music (the calculator has no speaker), the
 * debugging displays, the Mario head of the title screen (Goddard). Each does nothing, or as little as the
 * game needs. */
#include <ultra64.h>
#include "sm64.h"
#include "types.h"
#include "game/game_init.h"
#include "game/memory.h"
#include "game/paintings.h"
#include "engine/graph_node.h"
#include "audio/external.h"
#include "game/profiler.h"

/* ---------------------------------------------------------------------------------------------- sound */
f32 gGlobalSoundSource[3];
u32 gAudioRandom;
void play_sound(s32 soundBits, f32 *pos) {}
void stop_sound(u32 soundBits, f32 *pos) {}
void stop_sounds_from_source(f32 *pos) {}
void stop_sounds_in_continuous_banks(void) {}
void seq_player_fade_out(u8 player, u16 fadeDuration) {}
void fade_volume_scale(u8 player, u8 targetScale, u16 fadeDuration) {}
void seq_player_lower_volume(u8 player, u16 fadeDuration, u8 percentage) {}
void seq_player_unlower_volume(u8 player, u16 fadeDuration) {}
void sound_banks_disable(u8 player, u16 bankMask) {}
void sound_banks_enable(u8 player, u16 bankMask) {}
void set_sound_moving_speed(u8 bank, u8 speed) {}
void play_music(u8 player, u16 seqArgs, u16 fadeTimer) {}
void stop_background_music(u16 seqId) {}
void drop_queued_background_music(void) {}
static u16 sCurrentMusic;
u16 get_current_background_music(void) { return sCurrentMusic; }
void play_secondary_music(u8 seqId, u8 bgMusicVolume, u8 volume, u16 fadeTimer) {}
void func_80321080(u16 fadeTimer) {}
void play_course_clear(void) {}
void play_peachs_jingle(void) {}
void play_puzzle_jingle(void) {}
void play_power_star_jingle(u8 arg0) {}
void play_race_fanfare(void) {}
void play_toads_jingle(void) {}
void play_menu_sounds(s16 soundMenuFlags) {}
void play_painting_eject_sound(void) {}
void play_infinite_stairs_music(void) {}
void set_background_music(u16 a, u16 seqArgs, s16 fadeTimer) { sCurrentMusic = seqArgs; }
void fadeout_music(s16 fadeOutTime) {}
void fadeout_level_music(s16 fadeTimer) {}
void play_cutscene_music(u16 seqArgs) {}
void play_shell_music(void) {}
void stop_shell_music(void) {}
void play_cap_music(u16 seqArgs) {}
void fadeout_cap_music(void) {}
void stop_cap_music(void) {}
void lower_background_noise(s32 a) {}
void raise_background_noise(s32 a) {}
void enable_background_sound(void) {}
void disable_background_sound(void) {}
void reset_volume(void) {}
void set_sound_mode(u16 soundMode) {}

/* ---------------------------------------------------------------------------------------------- the N64 */
u64 osClockRate = 62500000;
OSMesgQueue gSIEventMesgQueue;
s8 gEepromProbe = 1;
void (*gGoddardVblankCallback)(void);
Gfx *gDisplayListHead;
struct DmaHandlerList gMarioAnimsBuf;
struct DemoInput *gCurrDemoInput;
s64 get_current_clock(void) { return 0; }
s64 get_clock_difference(UNUSED s64 cycles) { return 0; }
void make_viewport_clip_rect(Vp *viewport) {}
void clear_viewport(Vp *viewport, s32 color) {}
void clear_framebuffer(s32 color) {}

void guMtxF2L(float mf[4][4], Mtx *m) {}
void osViBlack(u8 active) {}
void osViSetSpecialFeatures(u32 func) {}
s32 osPiStartDma(OSIoMesg *mb, s32 priority, s32 direction, uintptr_t devAddr, void *vAddr, size_t nbytes,
                 OSMesgQueue *mq) { return 0; }
void osInvalDCache(void *vaddr, size_t nbytes) {}
s32 osRecvMesg(OSMesgQueue *mq, OSMesg *msg, s32 flag) { return 0; }
void osCreateMesgQueue(OSMesgQueue *mq, OSMesg *msg, s32 count) {}


/* ---------------------------------------------------------------------------------------------- paintings */
s8 gDDDPaintingStatus;
f32 gPaintingMarioYEntry;
struct Painting *gRipplingPainting;
s16 gEnvFxBubbleConfig[10];

/* ---------------------------------------------------------------------------------------------- debugging */
s8 gDebugLevelSelect;
s8 gShowDebugText;
s8 gShowProfiler;
void debug_unknown_level_select_check(void) {}
void print_debug_bottom_up(const char *str, s32 number) {}
void print_debug_top_down_mapinfo(const char *str, s32 number) {}
void print_debug_top_down_objectinfo(const char *str, s32 number) {}
void reset_debug_objectinfo(void) {}
void stub_debug_5(void) {}
void try_do_mario_debug_object_spawn(void) {}
void try_print_debug_mario_level_info(void) {}
void try_print_debug_mario_object_info(void) {}
void profiler_log_thread5_time(enum ProfilerGameEvent eventID) {}

/* ---------------------------------------------------------------------------------------------- Goddard */
void gd_copy_p1_contpad(OSContPad *p1cont) {}
void *gdm_gettestdl(s32 id) { return NULL; }
void gd_vblank(void) {}
s32 gd_sfx_to_play(void) { return 0; }
void gdm_init(void *blockpool, u32 size) {}
void gdm_setup(void) {}
void gdm_maketestdl(s32 id) {}
void gd_add_to_heap(void *addr, u32 size) {}

/* ---------------------------------------------------------------------------------------------- the menus
 * (the file select and act select were objects drawn by the N64: the calculator has its own, src/port/menu.c) */
OSIoMesg gDmaIoMesg;
OSMesg gMainReceivedMesg;
OSMesgQueue gDmaMesgQueue;
u8 *gGfxPoolEnd;
void beh_yellow_background_menu_init(void) {}
void beh_yellow_background_menu_loop(void) {}
void bhv_act_selector_init(void) {}
void bhv_act_selector_loop(void) {}
void bhv_act_selector_star_type_loop(void) {}
void bhv_menu_button_init(void) {}
void bhv_menu_button_loop(void) {}
void bhv_menu_button_manager_init(void) {}
void bhv_menu_button_manager_loop(void) {}
