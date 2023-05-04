#ifndef INVENTORY_H
#define INVENTORY_H

#include <ncurses.h>

typedef struct Item {
    char *name;
    char symbol;
    int color;
} Item;

typedef struct Inventory {
    Item *items;
    int size;
    int capacity;
} Inventory;

void init_inventory(Inventory *inventory, int capacity);
void free_inventory(Inventory *inventory);
int add_item(Inventory *inventory, Item item);
int remove_item(Inventory *inventory, int index);
void draw_inventory(WINDOW *win, Inventory *inventory);

#endif
