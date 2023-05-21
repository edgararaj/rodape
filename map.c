#include <assert.h>
#include <math.h>

#include "camera.h"
#include "collide.h"
#include "colors.h"
#include "draw.h"
#include "light.h"
#include "map.h"
#include "objects.h"
#include "state.h"
#include "term.h"
#include "utils.h"

uint32_t dist_map_encode(int value)
{
    return (value & PARTITION_MASK) << DIST_MAP_SHIFT;
}

uint32_t light_map_encode(int value)
{
    return (value & PARTITION_MASK) << LIGHT_MAP_SHIFT;
}

int light_map_decode(uint32_t value)
{
    return (value & LIGHT_MAP_MASK) >> LIGHT_MAP_SHIFT;
}

int dist_map_decode(uint32_t value)
{
    return (value & DIST_MAP_MASK) >> DIST_MAP_SHIFT;
}

int normal_map_decode(uint32_t value)
{
    return (value & NORMAL_MAP_MASK) >> NORMAL_MAP_SHIFT;
}

uint32_t normal_map_encode(int value)
{
    return value << NORMAL_MAP_SHIFT;
}

void set_normal_map_value(Bitmap bitmap, Vec2i pos, int value)
{
    bitmap.data[pos.y * bitmap.width + pos.x] &= (~NORMAL_MAP_MASK);
    bitmap.data[pos.y * bitmap.width + pos.x] |= normal_map_encode(value);
}

void set_light_map_value(Bitmap bitmap, Vec2i pos, int value)
{
    // assert(value >= 0 && value <= LIGHT_RADIUS);
    bitmap.data[pos.y * bitmap.width + pos.x] &= (~LIGHT_MAP_MASK);
    bitmap.data[pos.y * bitmap.width + pos.x] |= light_map_encode(value);
}

int get_light_map_value(Bitmap bitmap, Vec2i pos)
{
    return light_map_decode(bitmap.data[pos.y * bitmap.width + pos.x]);
}

int get_normal_map_value(Bitmap bitmap, Vec2i pos)
{
    return normal_map_decode(bitmap.data[pos.y * bitmap.width + pos.x]);
}

int get_dist_map_value(Bitmap bitmap, Vec2i pos)
{
    return dist_map_decode(bitmap.data[pos.y * bitmap.width + pos.x]);
}

void add_light_map_value(Bitmap bitmap, Vec2i pos, int value)
{
    int result = value + get_light_map_value(bitmap, pos);
    // if (result > LIGHT_RADIUS) {
    // add_term_line("%d\n", result);
    // }
    // if (result > LIGHT_RADIUS)
    //     result = LIGHT_RADIUS;
    set_light_map_value(bitmap, pos, result);
}

void set_dist_map_value(Bitmap bitmap, Vec2i pos, int value)
{
    // assert(value >= 0 && value <= MAX_DIST);
    bitmap.data[pos.y * bitmap.width + pos.x] &= (~DIST_MAP_MASK);
    bitmap.data[pos.y * bitmap.width + pos.x] |= dist_map_encode(value);
}

void apply_horizontal_tunnel(int x1, int x2, int y, Bitmap bitmap)
{
    int start = x1 < x2 ? x1 : x2;
    int end = x1 > x2 ? x1 : x2;

    for (int x = start; x <= end; x++)
    {
        set_normal_map_value(bitmap, (Vec2i){x, y}, WALKABLE);
        if (y + 1 < bitmap.height) // Verifica se não estamos no limite do bitmap
        {
            set_normal_map_value(bitmap, (Vec2i){x, y + 1}, WALKABLE);
        }
    }
}

void apply_vertical_tunnel(int y1, int y2, int x, Bitmap bitmap)
{
    int start = y1 < y2 ? y1 : y2;
    int end = y1 > y2 ? y1 : y2;

    for (int y = start; y <= end; y++)
    {
        set_normal_map_value(bitmap, (Vec2i){x, y}, WALKABLE);
        if (x + 1 < bitmap.width) // Verifica se não estamos no limite do bitmap
        {
            set_normal_map_value(bitmap, (Vec2i){x + 1, y}, WALKABLE);
        }
    }
}

int radius_count(Bitmap bitmap, int x, int y, int r)
{
    int result = 0;

    int xmin = MAX(x - r, 0);
    int xmax = MIN(x + r, bitmap.width - 1);
    for (int i = xmin; i <= xmax; i++)
    {
        int ymin = MAX(y - r, 0);
        int ymax = MIN(y + r, bitmap.height - 1);
        for (int j = ymin; j <= ymax; j++)
        {
            if (normal_map_decode(bitmap.data[j * bitmap.width + i]) == WALL)
                result++;
        }
    }
    return result;
}

void generate_obstacles(Bitmap bitmap, Rect rect2)
{
    Rect rect = expand_rect(rect2, -5);
    for (int x = rect.tl.x; x < rect.br.x; x++)
    {
        for (int y = rect.tl.y; y < rect.br.y; y++)
        {
            if (rand() % 100 < 10 && radius_count(bitmap, x, y, 4) <= 10)
            {
                set_normal_map_value(bitmap, (Vec2i){x, y}, WALL);
            }
        }
    }
    for (int k = 0; k < 100; k++)
    {
        for (int x = rect.tl.x; x < rect.br.x; x++)
        {
            for (int y = rect.tl.y; y < rect.br.y; y++)
            {
                if (radius_count(bitmap, x, y, 1) >= 4)
                {
                    set_normal_map_value(bitmap, (Vec2i){x, y}, WALL);
                }
            }
        }
    }
    for (int x = rect.tl.x; x < rect.br.x; x++)
    {
        for (int y = rect.tl.y; y < rect.br.y; y++)
        {
            if (radius_count(bitmap, x, y, 2) <= 4 && normal_map_decode(bitmap.data[y * bitmap.width + x]) == WALL)
            {
                set_normal_map_value(bitmap, (Vec2i){x, y}, WALKABLE);
            }
        }
    }
}

void generate_tunnels_and_rasterize(Bitmap bitmap, Rect *rects, int rect_count)
{
    bitmap_draw_rectangle(bitmap, rects[0]);
    for (int i = 1; i < rect_count; i++)
    {
        bitmap_draw_rectangle(bitmap, rects[i]);
        Vec2i prev_center = get_center(rects[i - 1]);
        Vec2i new_center = get_center(rects[i]);

        // add_term_line("P:%d,%d N:%d,%d", prev_center.x, prev_center.y,
        // new_center.x, new_center.y);

        if (rand() % 2 == 1)
        {
            apply_horizontal_tunnel(prev_center.x, new_center.x, prev_center.y, bitmap);
            apply_vertical_tunnel(prev_center.y, new_center.y, new_center.x, bitmap);
        }
        else
        {
            apply_vertical_tunnel(prev_center.y, new_center.y, prev_center.x, bitmap);
            apply_horizontal_tunnel(prev_center.x, new_center.x, new_center.y, bitmap);
        }
    }
}

int generate_rects(Rect window, Rect *rects, int rects_max)
{
    int rects_count = 0;

    int div = 2;
    for (int f = 0; f < div * div; f++)
    {
        Rect sub = subdivide_rect(window, div, f);
        for (int i = 0; i < (rects_max / (div * div)); i++)
        {
            Rect new_rect;
            int valid_new_rect;
            for (int j = 0; j < 255; j++)
            {
                valid_new_rect = 1;
                new_rect = gen_random_subrect(sub);
                for (int n = 0; n < rects_count; n++)
                {
                    if (collide_rect_rect(rects[n], expand_rect(new_rect, 2)))
                    {
                        valid_new_rect = 0;
                        break;
                    }
                }
                if (valid_new_rect)
                    break;
            }
            if (valid_new_rect)
            {
                rects[rects_count++] = new_rect;
            }
        }
    }

    return rects_count;
}

void order_rects(Rect *rects, int rects_count)
{
    for (int i = 0; i < rects_count; i++)
    {
        for (int j = 0; j < rects_count; j++)
        {
            if (rects[i].tl.x < rects[j].tl.x)
            {
                Rect temp = rects[i];
                rects[i] = rects[j];
                rects[j] = temp;
            }
        }
    }
}

void erode(Bitmap bitmap, int iterations)
{
    // Erode com DLA
    for (int i = 0; i < iterations; i++)
    {
        // Encontre todas as células abertas
        int open_tiles[bitmap.width * bitmap.height];
        int open_tile_count = 0;

        for (int j = 0; j < bitmap.width * bitmap.height; j++)
        {
            if (normal_map_decode(bitmap.data[j]) == WALKABLE)
            {
                open_tiles[open_tile_count++] = j;
            }
        }

        // Escolha uma célula aberta aleatoriamente
        int random_index = rand() % open_tile_count;
        int digger = open_tiles[random_index];
        int digger_x = digger % bitmap.width;
        int digger_y = digger / bitmap.width;

        // Enquanto a célula escolhida ainda for um '#', escolha uma direção
        // aleatória e mova o "digger"
        int cap = 1000;
        while (normal_map_decode(bitmap.data[digger]) == WALKABLE && cap-- > 0)
        {
            int direction = rand() % 4;

            switch (direction)
            {
            case 0: // Mova para a esquerda
                if (digger_x > 2)
                {
                    digger_x -= 1;
                }
                break;
            case 1: // Mova para a direita
                if (digger_x < bitmap.width - 2)
                {
                    digger_x += 1;
                }
                break;
            case 2: // Mova para cima
                if (digger_y > 2)
                {
                    digger_y -= 1;
                }
                break;
            case 3: // Mova para baixo
                if (digger_y < bitmap.height - 2)
                {
                    digger_y += 1;
                }
                break;
            }

            digger = digger_y * bitmap.width + digger_x;
        }

        // Marque a nova célula como um '#'
        set_normal_map_value(bitmap, (Vec2i){digger_x, digger_y}, WALKABLE);
    }
}

int map_is_wall(Bitmap pixmap, Vec2f pos)
{
    int data = normal_map_decode(pixmap.data[(int)pos.y * pixmap.width + (int)pos.x]);
    return data == WALL || data == SHINE;
}

void generate_spikes(Bitmap pixmap, Rect rect2)
{
    Rect rect = expand_rect(rect2, -5);
    for (int x = rect.tl.x; x < rect.br.x; x++)
    {
        for (int y = rect.tl.y; y < rect.br.y; y++)
        {
            Vec2i pos = (Vec2i){x, y};
            if (get_normal_map_value(pixmap, pos) == WALKABLE && rand() % 100 < 3.5)
            { // 3.5% chance to place a spike
                set_normal_map_value(pixmap, pos, SPIKE);
            }
        }
    }
}

void generate_chests(GameState *gs, Bitmap pixmap, Rect rect2)
{
    Rect rect = expand_rect(rect2, -5);
    for (int x = rect.tl.x; x < rect.br.x - 3; x++)     // -3 to avoid going out of bounds
    {
        for (int y = rect.tl.y; y < rect.br.y - 2; y++) // -2 to avoid going out of bounds
        {
            if (rand() % 1000 < 5)                      // 0.5% chance to place a chest
            {
                // Place the chest pattern
                for (int dx = 0; dx < 4; dx++)
                {
                    for (int dy = 0; dy < 3; dy++)
                    {
                        if (dy == 1 && (dx == 1 || dx == 2)) // Place the yellow 'O's
                        {
                            set_normal_map_value(pixmap, (Vec2i){x + dx, y + dy}, CHESTIN);
                        }
                        else // Place the brown 'C's
                        {
                            set_normal_map_value(pixmap, (Vec2i){x + dx, y + dy}, CHESTOUT);
                        }
                    }
                }
            }
        }
    }
}

void generate_portal(GameState *gs, Bitmap pixmap, Rect rect2)
{
    Rect rect = expand_rect(rect2, -5);
    for (int x = rect.tl.x; x < rect.br.x - 3; x++)     // -3 to avoid going out of bounds
    {
        for (int y = rect.tl.y; y < rect.br.y - 2; y++) // -2 to avoid going out of bounds
        {
            if (rand() % 1000 < 1000)                   // 0.5% chance to place a chest
            {
                // Place the chest pattern
                for (int dx = 0; dx < 4; dx++)
                {
                    for (int dy = 0; dy < 3; dy++)
                    {
                        if (dy == 1 && (dx == 1 || dx == 2)) // Place the yellow 'O's
                        {
                            set_normal_map_value(pixmap, (Vec2i){x + dx, y + dy}, PORTAL);
                        }
                        else // Place the brown 'C's
                        {
                            set_normal_map_value(pixmap, (Vec2i){x + dx, y + dy}, PORTAL);
                        }
                    }
                }
            }
        }
    }
}

int has_item(Inventory *inventory, ItemType type)
{
    for (int i = 0; i < inventory->size; i++)
    {
        if (inventory->items[i].type == type)
        {
            return 1;
        }
    }
    return 0;
}

int map_is_walkable(Bitmap pixmap, Vec2f pos, Vec2f inc)
{
    Vec2f inc_x = {inc.x, 0};
    Vec2f inc_y = {0, inc.y};
    int data = normal_map_decode(pixmap.data[(int)pos.y * pixmap.width + (int)pos.x]);
    return ((!map_is_wall(pixmap, vec2f_add(pos, inc_x)) ||
            !map_is_wall(pixmap, vec2f_add(pos, inc_y))) &&
           !map_is_wall(pixmap, vec2f_add(pos, inc)));
}

void render_map(WINDOW *win_game, GameState *gs, Camera camera, Bitmap map, WINDOW *window, Bitmap illuminated)
{
    for (int x = 0; x < camera.width; ++x)
    {
        for (int y = 0; y < camera.height; ++y)
        {
            int map_x = x + camera.x;
            int map_y = y + camera.y;
            uint32_t data = map.data[map_y * map.width + map_x];

            if (normal_map_decode(data) == WALKABLE)
            {
                wattrset(win_game, COLOR_PAIR(Culur_Light_Gradient + MIN(light_map_decode(data), LIGHT_RADIUS - 1)));
                print_pixel(window, x, y);
            }
            // if (light_map_decode(data)) {
            //     wattrset(win_game, COLOR_PAIR(Culur_Default));
            //     char s[] = "0";
            //     s[0] = '0' + light_map_decode(data);
            //     print_pixel_custom(window, x, y, s);
            // }

            if (normal_map_decode(data) == SHINE)
            {
                wattrset(win_game, COLOR_PAIR(Culur_Shine));
                print_pixel(window, x, y);
            }
            else if (normal_map_decode(data) == SPIKE)
            {
                wattrset(win_game, COLOR_PAIR(Culur_Spike));
                print_pixel_custom(window, x, y, "^");
            }
            else if (normal_map_decode(data) == CHESTOUT)
            {
                wattrset(win_game, COLOR_PAIR(Culur_Chest_Back));
                print_pixel_custom(window, x, y, "C");
            }
            else if (normal_map_decode(data) == CHESTIN)
            {
                wattrset(win_game, COLOR_PAIR(Culur_Chest_Front));
                print_pixel_custom(window, x, y, "O");
            }
            else if (normal_map_decode(data) == PORTAL)
            {
                wattrset(win_game, COLOR_PAIR(Culur_Portal));
                print_pixel_custom(window, x, y, "P");
            }
            else
            {
                data = normal_map_decode(illuminated.data[map_y * map.width + map_x]);
                if (data == SHINE)
                {
                    wattrset(win_game, COLOR_PAIR(Culur_Shine_Dimmed));
                    print_pixel(window, x, y);
                }
            }
            if (dist_map_decode(data) == 1)
            {
                wattrset(win_game, COLOR_PAIR(2));
                print_pixel(window, x, y);
            }
        }
    }
}

void render_minimap(WINDOW *win, Bitmap illuminated, Vec2i window_size, Vec2i player_pos)
{
    float scale_x = (float)illuminated.width / window_size.x;
    float scale_y = (float)illuminated.height / window_size.y;

    wattrset(win, COLOR_PAIR(COLOR_RED));
    for (int y = 0; y < window_size.y; y++)
    {
        for (int x = 0; x < window_size.x; x++)
        {
            int map_x = x * scale_x;
            int map_y = y * scale_y;

            if (illuminated.data[map_y * illuminated.width + map_x])
            {
                print_pixel(win, x, y);
            }
        }
    }
    wattrset(win, COLOR_PAIR(COLOR_BLUE));
    int player_x = player_pos.x / scale_x;
    int player_y = player_pos.y / scale_y;
    add_term_line("%d, %d\n", player_x, player_y);
    print_pixel(win, player_x, player_y);
}
