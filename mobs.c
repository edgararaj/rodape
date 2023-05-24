#include "mobs.h"
#include "combat.h"
#include "dist.h"
#include "map.h"
#include "objects.h"
#include "state.h"
#include "utils.h"
#include <assert.h>
#include <stdlib.h>

void create_mobs(Bitmap pixmap, Mob *mobs, int num_mobs)
{
    int width = 1;
    int height = 1;
    for (int i = 0; i < num_mobs; i++)
    {
        int x, y;
        do
        {
            x = random_between(0, pixmap.width - width);
            y = random_between(0, pixmap.height - height);
        } while (normal_map_decode(pixmap.data[y * pixmap.width + x]) != WALKABLE);

        mobs[i].type = random_between(0, MobType__Size);
        mobs[i].warrior.rect = (RectFloat){{x, y}, {x + width - 1, y + height - 1}, 6};
        mobs[i].warrior.dmg = random_between(2, 20);
        mobs[i].speed = random_between(1, 5);
        mobs[i].warrior.hp = random_between(10, 100);
        mobs[i].warrior.maxHP = 100;
        mobs[i].warrior.weight = 3;
        mobs[i].warrior.dmg_cooldown = 1000;
    }
}

Vec2i step_to_player(Bitmap map, Mob *mob)
{
    int smallest = MAX_DIST_CALC;
    Vec2i smallest_add = {0, 0};
    for (int i = -1; i < 2; i++)
    {
        for (int j = -1; j < 2; j++)
        {
            if (i + j == -1 || i + j == 1)
            {
                Vec2i add = {i, j};
                Vec2f rect = vec2f_add(rect_float_center(mob->warrior.rect), vec2i_to_f(add));
                int data = get_dist_map_value(map, vec2f_to_i(rect));
                if (data < smallest && data && data <= MAX_DIST_CALC)
                {
                    smallest = data;
                    smallest_add = add;
                }
            }
        }
    }
    return smallest_add;
}

void move_mob(Mob *mob, Vec2i step, int delta_us)
{
    mob->warrior.rect = rect_float_translate(mob->warrior.rect, vec2f_div_const(vec2i_to_f(step), mob->speed * 10));
}

void attack_player(Mob *mob, Warrior *player, Bitmap map, int delta_us)
{
    Vec2i step = step_to_player(map, mob);
    move_mob(mob, step, delta_us);

    warrior_attack(&mob->warrior, player, delta_us);
}

void wander(Mob *mob, Bitmap map, int delta_us)
{
    for (int i = -1; i < 2; i++)
    {
        for (int j = -1; j < 2; j++)
        {
            Vec2i add = {i, j};
            if (!map_is_wall(map, vec2f_add(rect_float_center(mob->warrior.rect), vec2i_to_f(add))) &&
                (rand() % 100 < 20))
            {
                move_mob(mob, add, delta_us);
            }
        }
    }
}

void update_mob(Mob *mobs, int num_mobs, int ii, Bitmap map, Warrior *player, int delta_us)
{
    Mob *mob = &mobs[ii];
    if (mob->type == MobType_Stupid)
    {
        if (vec2f_sqrdistance(rect_float_center(mob->warrior.rect), rect_float_center(player->rect)) <
            THREAT_RADIUS_SQR)
        {
            mob->warrior.rect.color = COLOR_RED;
            attack_player(mob, player, map, delta_us);
        }
        else
        {
            if (!mob->called)
            {
                mob->warrior.rect.color = COLOR_BLUE;
                wander(mob, map, delta_us);
            }
        }
    }
    else if (mob->type == MobType_Coward || mob->type == MobType_Intelligent)
    {
        int mob_dist_to_player =
            vec2f_sqrdistance(rect_float_center(mob->warrior.rect), rect_float_center(player->rect));
        if (mob_dist_to_player < VISION_RADIUS_SQR)
        {
            int mobs_near = 0;
            for (int i = 0; i < num_mobs; i++)
            {
                int mob_dist_to_caller =
                    vec2f_sqrdistance(rect_float_center(mobs[i].warrior.rect), rect_float_center(mob->warrior.rect));
                if ((ii != i) && (mob_dist_to_caller <= THREAT_RADIUS_SQR))
                {
                    mobs_near++;
                }
            }
            if (mobs_near || (vec2f_sqrdistance(rect_float_center(mob->warrior.rect), rect_float_center(player->rect)) <
                              THREAT_RADIUS_SQR))
            {
                mob->warrior.rect.color = COLOR_RED;
                attack_player(mob, player, map, delta_us);
            }
            else
            {
                mob->warrior.rect.color = COLOR_YELLOW;
                if (!mob->called)
                {
                    wander(mob, map, delta_us);
                }
            }
            for (int i = 0; i < num_mobs; i++)
            {
                mob_dist_to_player =
                    vec2f_sqrdistance(rect_float_center(mobs[i].warrior.rect), rect_float_center(player->rect));
                if (((i != ii) && mob_dist_to_player >= THREAT_RADIUS_SQR))
                {
                    mobs[i].called = 1;
                    // Is Calling other
                    mobs[i].warrior.rect.color = COLOR_YELLOW;
                    attack_player(&mobs[i], player, map, delta_us);
                }
                if (mob_dist_to_player >= VISION_RADIUS_SQR)
                {
                    mobs[i].called = 0;
                }
            }
        }
        else
        {
            if (!mob->called)
            {
                mob->warrior.rect.color = COLOR_BLUE;
                wander(mob, map, delta_us);
            }
        }
    }
}

void update_mobs(Mob *mobs, int num_mobs, Bitmap map, Warrior *player, int delta_us)
{
    for (int i = 0; i < num_mobs; i++)
    {
        if (mobs[i].warrior.hp <= 0)
            continue;
        update_mob(mobs, num_mobs, i, map, player, delta_us);
    }
}