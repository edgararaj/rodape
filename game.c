#include "camera.h"
#include "colors.h"
#include "map.h"
#include "mobs.h"
#include "objects.h"
#include "state.h"
#include "term.h"
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int char_width_int(int value) { return snprintf(NULL, 0, "%d", value); }

void render_life(WINDOW *win_game, Camera camera, Rect rect, int life) {
    Vec2i size = rect_size(rect);
    Rect translated_rect =
        rect_translate(rect, vec2i_mul_const(camera.offset, -1));
    int char_width = char_width_int(life);
    int new_x = (int)(translated_rect.tl.x * X_SCALE +
                      (size.x * X_SCALE) / 2.f - (char_width / 2.f)) +
                1;
    wattrset(win_game, COLOR_PAIR(Culur_Default));
    mvwprintw(win_game, translated_rect.tl.y - 1, new_x, "%d", life);
}

void update_player(Rect *st, int key) {
    switch (key) {
        case KEY_A1:
        case '7':
            *st = rect_translate(*st, (Vec2i){-1, -1});
            break;
        case KEY_UP:
        case '8':
            *st = rect_translate(*st, (Vec2i){0, -1});
            break;
        case KEY_A3:
        case '9':
            *st = rect_translate(*st, (Vec2i){1, -1});
            break;
        case KEY_LEFT:
        case '4':
            *st = rect_translate(*st, (Vec2i){-1, 0});
            break;
        case KEY_B2:
        case '5':
            break;
        case KEY_RIGHT:
        case '6':
            *st = rect_translate(*st, (Vec2i){1, 0});
            break;
        case KEY_C1:
        case '1':
            *st = rect_translate(*st, (Vec2i){-1, 1});
            break;
        case KEY_DOWN:
        case '2':
            *st = rect_translate(*st, (Vec2i){0, 1});
            break;
        case KEY_C3:
        case '3':
            *st = rect_translate(*st, (Vec2i){1, 1});
            break;
        default:
            break;
    }
}

void render_rect(WINDOW *win, Camera camera, Rect player) {
    Rect rect = rect_translate(player, vec2i_mul_const(camera.offset, -1));
    print_rectangle(win, rect);
}

Rect rect_float_to_rect(RectFloat rect) {
    Rect result;
    result.color = rect.color;
    result.tl = vec2f_to_i(rect.tl);
    result.br = vec2f_to_i(rect.br);
    return result;
}

void shine_reset(Bitmap distmap) {
    for (int x = 0; x < distmap.width; x++) {
        for (int y = 0; y < distmap.height; y++) {
            if (normal_map_decode(distmap.data[y * distmap.width + x]) == SHINE)
                set_normal_map_value(distmap, (Vec2i){x, y}, WALL);
        }
    }
}

void draw_game(GameState *gs, Vec2i window_size, int key) {
    gs->camera.width = window_size.x;
    gs->camera.height = window_size.y;
    if (key == 't') {
        ++gs->cam_mode;
        gs->cam_mode %= CameraMode__Size;
    }

    if (key == ' ') {
        center_camera(&gs->camera, gs->player.tl.x, gs->player.tl.y);
        // add_term_line("%d, %d\n", gs->camera.x, gs->camera.y);
    }

    if (key == 'i') {
        int ch;
        while (1) {
            draw_inventory(gs->win_inventory, &gs->inventory);
            ch = getch();
            if (ch == 'i' ||
                ch == 'q') { // Pressione 'i' ou 'q' para sair do inventário
                break;
            }
        }
    }

    if (key == 'm') {
        minimap_maximized = !minimap_maximized;
    }

    Rect prev_player = gs->player;
    update_player(&gs->player, key);
    if (collide_rect_bitmap(gs->player, gs->pixmap)) {
        gs->player = prev_player;
    }

    update_mobs(gs->mobs, MAX_MOBS, gs->player.tl, gs->pixmap);

    if (gs->cam_mode == CameraMode_Margin)
        update_camera(&gs->camera, gs->player.tl.x, gs->player.tl.y);
    else
        center_camera(&gs->camera, gs->player.tl.x, gs->player.tl.y);

    dist_reset(gs->pixmap);
    shine_reset(gs->pixmap);
    dist_pass(gs->pixmap, gs->player.tl, gs->illuminated);

    light_reset(gs->pixmap);
    for (int i = 0; i < MAX_TORCHES; i++) {
        light_pass(gs->win_game, gs->camera, gs->pixmap,
                   gs->torches[i].position, gs->torches[i].radius,
                   LightType_Torch);
        wattrset(gs->win_game, COLOR_PAIR(3));
        print_pixel(gs->win_game, gs->torches[i].position.tl.x - gs->camera.x,
                    gs->torches[i].position.tl.y - gs->camera.y);
    }

    light_pass(gs->win_game, gs->camera, gs->pixmap, gs->player, LIGHT_RADIUS,
               LightType_Vision);

    render_map(gs->win_game, gs->camera, gs->pixmap, gs->win_game,
               gs->illuminated);

    for (int i = 0; i < MAX_MOBS; i++) {
        render_rect(gs->win_game, gs->camera,
                    rect_float_to_rect(gs->mobs[i].rect));
    }

    render_life(gs->win_game, gs->camera, gs->player, 100);

    render_rect(gs->win_game, gs->camera, gs->player);
    render_minimap(gs->win_game, gs->illuminated, window_size, gs->player.tl);

    wrefresh(gs->win_game);
}
