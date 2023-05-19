#pragma once
#include "camera.h"
#include "objects.h"
#include "screen.h"
#include "state.h"

#define MAP_WIDTH (2 * GAME_WIDTH)
#define MAP_HEIGHT (2 * GAME_HEIGHT)

#define WALL 0
#define WALKABLE 1
#define SHINE 2
#define SPIKE 3
#define CHEST 4
#define CHESTOUT 5
#define CHESTIN 6
#define PORTAL 7

#define MAX_CHESTS 100

#define SPIKE_DAMAGE 7
#define SPIKE_DAMAGE_COOLDOWN 1200

#define PARTITION_MASK 0xFF

#define NORMAL_MAP_SHIFT 0
#define NORMAL_MAP_MASK (0xFF << NORMAL_MAP_SHIFT)

#define DIST_MAP_SHIFT 8
#define DIST_MAP_MASK (0xFF << DIST_MAP_SHIFT)
#define MAX_DIST_CALC 20
#define MAX_DIST 20

#define LIGHT_MAP_SHIFT 16
#define LIGHT_MAP_MASK (0xFF << LIGHT_MAP_SHIFT)
#define LIGHT_MAP_MAX LIGHT_RADIUS

#define HIGH_RESOLUTION 2.6
#define DEFAULT_RESOLUTION 9

#define min(a, b) ((a) < (b) ? (a) : (b))

int map_is_wall(Bitmap pixmap, Vec2f pos);
void render_map(WINDOW *win_game, GameState* gs, Camera camera, Bitmap map, WINDOW *window, Bitmap illuminated);
void render_minimap(WINDOW *win, Bitmap illuminated, Vec2i window_size, Vec2i player_pos, int);
int map_is_walkable(GameState *gs, Bitmap pixmap, Camera camera, Vec2f pos, Vec2f inc, Player_Stats player, Inventory *inventory);
void add_light_map_value(Bitmap bitmap, Vec2i pos, int value);

uint32_t dist_map_encode(int value);
uint32_t light_map_encode(int value);
int light_map_decode(uint32_t value);
int dist_map_decode(uint32_t value);
int normal_map_decode(uint32_t value);
uint32_t normal_map_encode(int value);
void set_normal_map_value(Bitmap bitmap, Vec2i pos, int value);
void set_light_map_value(Bitmap bitmap, Vec2i pos, int value);
void set_dist_map_value(Bitmap bitmap, Vec2i pos, int value);
int get_light_map_value(Bitmap bitmap, Vec2i pos);
int get_normal_map_value(Bitmap bitmap, Vec2i pos);
int generate_rects(Rect window, Rect *rects, int rects_max);
void order_rects(Rect *rects, int rects_count);
void generate_tunnels_and_rasterize(Bitmap bitmap, Rect *rects, int rect_count);
void erode(Bitmap bitmap, int iterations);
void generate_spikes(Bitmap pixmap, Rect rect2);
void generate_obstacles(Bitmap bitmap, Rect rect2);
void generate_chests(GameState* gs, Bitmap pixmap, Rect rect2);
void generate_portal(GameState* gs, Bitmap pixmap, Rect rect2);
