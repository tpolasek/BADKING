/* badking.c - The Bad King RPG
 * Original game by Griffin Knodle (1991), Flatrat Production
 * Ported from 16-bit Windows to Linux terminal
 * Based on decompiled BADKING.EXE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

/* Game constants */
#define MAX_NAME 20
#define NUM_LOCATIONS 49
#define NUM_ENEMIES 16
#define NUM_WEAPONS 6
#define NUM_ARMORS 6
#define NUM_RINGS 6
#define NUM_SPELLS 8
#define SAVE_FILE "badking.sav"

/* Location IDs */
enum {
    LOC_TREE_SQUARE = 0,
    LOC_SQUIRRELS_INN,
    LOC_GRUBBYS_ARMORY,
    LOC_MURAS_MAGIC,
    LOC_FOREST_GATE,
    LOC_FOREST_PATH,
    LOC_TALL_TREE,
    LOC_BEAR_ROCK,
    LOC_SHADOWY_PATH,
    LOC_BIG_OAKS,
    LOC_MEADOW,
    LOC_BABBLING_BROOK,
    LOC_RUSHING_FALLS,
    LOC_SLIPPERY_PATH,
    LOC_RUSHING_POOL,
    LOC_DARK_TUNNEL,
    LOC_DARKER_TUNNEL,
    LOC_DARKEST_TUNNEL,
    LOC_CAVES_END,
    LOC_LAKE_SHORE,
    LOC_LAKE_HOBS_WEST,
    LOC_LAKE_HOBS,
    LOC_LAKE_HOBS_EAST,
    LOC_EAST_SHORE,
    LOC_DUSTY_ROAD,
    LOC_STONY_GATE,
    LOC_EMPTY_SQUARE,
    LOC_TYRS_BATTLE_GEAR,
    LOC_HAPPY_FACE_INN,
    LOC_THURAS_MAGIC,
    LOC_DUSTY_ROAD_EAST,
    LOC_CROSSROADS,
    LOC_NORTH_ROAD,
    LOC_BRAMBLES1,
    LOC_BRAMBLES2,
    LOC_BRAMBLES3,
    LOC_EAST_ROAD,
    LOC_CASTLE_WALL,
    LOC_GARDEN_PATH,
    LOC_CASTLE_KEEP,
    LOC_ENTRANCE_HALL,
    LOC_PASSAGE,
    LOC_THRONE_ROOM,
    LOC_TOWER_1F,
    LOC_TOWER_2F,
    LOC_TOWER_3F,
    LOC_TOWER_4F,
    LOC_TOWER_ROOF
};

/* Enemy IDs */
enum {
    ENEMY_IMP = 0,
    ENEMY_SLIME,
    ENEMY_GOBLIN,
    ENEMY_VIPER,
    ENEMY_MAGE,
    ENEMY_VAMPIRE,
    ENEMY_KNIGHT,
    ENEMY_WYVERN,
    ENEMY_GREEN_DRAGON,
    ENEMY_DEMON_KNIGHT,
    ENEMY_WARLOCK,
    ENEMY_SERPENT,
    ENEMY_BLUE_DRAGON,
    ENEMY_RED_DRAGON,
    ENEMY_BAD_KING,
    ENEMY_BLACK_DRAGON,
    ENEMY_WIND_WIZARDS
};

/* Weapon IDs */
enum {
    WEAPON_NONE = 0,
    WEAPON_SHORT_SWORD,
    WEAPON_LONG_SWORD,
    WEAPON_BROADSWORD,
    WEAPON_SILVER_SWORD,
    WEAPON_DRAGON_SLAYER
};

/* Armor IDs */
enum {
    ARMOR_NONE = 0,
    ARMOR_LEATHER,
    ARMOR_CHAIN_MAIL,
    ARMOR_COPPER_PLATE,
    ARMOR_IRON_PLATE,
    ARMOR_BATTLE_SUIT
};

/* Ring IDs */
enum {
    RING_NONE = 0,
    RING_POWER,
    RING_SHIELD,
    RING_HEAL,
    RING_SILVER,
    RING_RUNE
};

/* Spell IDs */
enum {
    SPELL_HEAL = 0,
    SPELL_FIRE,
    SPELL_ICE,
    SPELL_LIT,
    SPELL_SLEEP,
    SPELL_HEAL_MAJOR,
    SPELL_RETURN,
    SPELL_NUKE
};

/* Game state */
struct GameState {
    char name[MAX_NAME];
    int hp, maxhp;
    int mp, maxmp;
    int strength;
    int attack, defense;
    int exp, gold, level;
    int weapon, armor, ring;
    int water_shoes, the_key;
    int spells[NUM_SPELLS];
    int location;
    int tree_gold;
};

struct GameState player;

/* Enemy data: name, hp, attack, defense, gold, exp */
struct EnemyData {
    const char *name;
    int hp, attack, defense, gold, exp;
};

struct EnemyData enemies[] = {
    {"Imp", 8, 3, 1, 5, 3},
    {"Slime", 12, 5, 2, 8, 5},
    {"Goblin", 18, 8, 3, 12, 8},
    {"Viper", 25, 10, 4, 18, 12},
    {"Mage", 30, 12, 5, 25, 15},
    {"Vampire", 40, 15, 8, 35, 20},
    {"Knight", 50, 18, 10, 45, 25},
    {"Wyvern", 60, 22, 12, 55, 30},
    {"Green Dragon", 80, 28, 15, 75, 40},
    {"Demon Knight", 100, 32, 18, 100, 50},
    {"Warlock", 120, 35, 20, 120, 60},
    {"Serpent", 140, 38, 22, 140, 70},
    {"Blue Dragon", 160, 42, 25, 160, 80},
    {"Red Dragon", 200, 48, 30, 200, 100},
    {"Bad King", 250, 55, 35, 0, 0},
    {"Black Dragon", 180, 45, 28, 180, 90},
    {"Wind Wizards", 90, 30, 16, 90, 45}
};

/* Weapon data: name, attack bonus, cost */
struct WeaponData {
    const char *name;
    int attack, cost;
};

struct WeaponData weapons[] = {
    {"None", 0, 0},
    {"Short Sword", 3, 10},
    {"Long Sword", 8, 70},
    {"2-Hand Broadsword", 16, 300},
    {"Silver Sword", 40, 1000},
    {"Dragon Slayer", 100, 0}
};

/* Armor data: name, defense bonus, cost */
struct ArmorData {
    const char *name;
    int defense, cost;
};

struct ArmorData armors[] = {
    {"None", 0, 0},
    {"Leather", 2, 10},
    {"Chain Mail", 5, 60},
    {"Copper Plate", 12, 315},
    {"Iron Plate", 24, 850},
    {"Battle Suit", 55, 6970}
};

/* Ring data: name, special effect, cost */
struct RingData {
    const char *name;
    int effect, cost;
};

struct RingData rings[] = {
    {"None", 0, 0},
    {"Power Ring", 1, 65},
    {"Shield Ring", 2, 85},
    {"Heal Ring", 3, 0},
    {"Silver Ring", 4, 1420},
    {"Rune Ring", 5, 0}
};

/* Spell data: name, mp cost */
struct SpellData {
    const char *name;
    int mp_cost;
};

struct SpellData spell_data[] = {
    {"Heal", 1},
    {"Fire", 2},
    {"Ice", 2},
    {"Lit", 2},
    {"Sleep", 3},
    {"Heal Major", 5},
    {"Return", 1},
    {"Nuke", 10}
};

/* Location descriptions */
struct LocationData {
    const char *name;
    const char *description;
    int enemy_type; /* -1 = no random encounters */
    int has_shop;
    int has_inn;
};

struct LocationData locations[] = {
    {"Tree Square", "You are standing in the town square of Forest-For-the-Trees.", -1, 0, 0},
    {"Squirrel's Nook Inn", "You are in the common room of the Squirrel's Nook Inn.", -1, 0, 1},
    {"Grubby's Armory", "Inside the shop, the air is filled with the smells of forged metal.", -1, 1, 0},
    {"Mura's Magic Shop", "Strange smells assault you as you enter the shop.", -1, 1, 0},
    {"Forest Gate", "You are standing outside the gate of Forest-For-the-Trees.", -1, 0, 0},
    {"Forest Path", "You are on a path that runs east and west through the forest.", 1, 0, 0},
    {"Tall Tree", "You have climbed the tree. It's very peaceful up here.", -1, 0, 0},
    {"Bear Rock", "You are at the landmark called Bear Rock.", 2, 0, 0},
    {"Shadowy Path", "The forest around here is so dense that it's hard to see the path.", 2, 0, 0},
    {"Big Oaks", "You are walking through a grove of immense oak trees.", 2, 0, 0},
    {"Meadow", "You are in a meadow of tall grasses and sticker bushes.", 3, 0, 0},
    {"Babbling Brook", "You are on the banks of a small stream.", 3, 0, 0},
    {"Rushing Falls", "You are standing at the top of a waterfall.", -1, 0, 0},
    {"Slippery Path", "You are on a narrow path leading down the cliff.", 3, 0, 0},
    {"Rushing Pool", "You are standing on the edge of a foaming pool.", 3, 0, 0},
    {"Dark Tunnel", "You are standing in a dimly lit tunnel.", 4, 0, 0},
    {"Darker Tunnel", "Further down the tunnel, it becomes very hard to see.", 4, 0, 0},
    {"Darkest Tunnel", "You can't see anything in this part of the tunnel.", 5, 0, 0},
    {"Cave's End", "You have reached the end of the cave.", 5, 0, 0},
    {"Lake Shore", "You are standing on the shore of Lake Hobs.", -1, 0, 0},
    {"Lake Hobs - West", "You hadn't realized how big Lake Hobs was until you tried to walk across it.", -1, 0, 0},
    {"Lake Hobs", "You are in the middle of Lake Hobs.", -1, 0, 0},
    {"Lake Hobs - East", "You are drawing near to the eastern shore, at last.", -1, 0, 0},
    {"East Shore", "The east shore of the lake is barren. No plants grow here.", -1, 0, 0},
    {"Dusty Road", "This dusty road runs through fields of brown grass.", 6, 0, 0},
    {"Stony Gate", "You've reached a gate. The wall is high and strong-looking.", -1, 0, 0},
    {"Empty Square", "Your footsteps echo hollowly on the stones.", -1, 0, 0},
    {"Tyr's Battle Gear", "The shop carries an impressive array of weapons and armor.", -1, 1, 0},
    {"Happy Face Inn", "The common room of the inn is painted a garish yellow color.", -1, 0, 1},
    {"Thura's Magic Shop", "This shop looks strangely familiar.", -1, 1, 0},
    {"Dusty Road - East", "This stretch of road looks much the same as the last.", 7, 0, 0},
    {"Crossroads", "Two roads meet here. An evil-looking statue watches over the east road.", 7, 0, 0},
    {"North Road", "Up ahead, the road disappears into a patch of brambles.", 7, 0, 0},
    {"Brambles", "The bramble patch is a thing of twisted, rotting evil.", 8, 0, 0},
    {"Brambles", "The vines tower overhead like blackened fingers reaching for the sky.", 8, 0, 0},
    {"Brambles", "You hear things skittering out of sight.", 8, 0, 0},
    {"East Road", "A steady rain falls on your face as you trudge along the road.", 9, 0, 0},
    {"Castle Wall", "You have reached the wall of an imposing castle.", -1, 0, 0},
    {"Garden Path", "You are on a path paved with flat gray stone.", 10, 0, 0},
    {"Castle Keep", "You are standing at the door to the castle's immense tower.", -1, 0, 0},
    {"Entrance Hall", "The entrance hall is hung with gaudy tapestries.", 11, 0, 0},
    {"Passage", "Torches light the hallway with a flickering glow.", 11, 0, 0},
    {"Throne Room", "The throne room is lit with an eerie green light.", -1, 0, 0},
    {"Tower - 1F", "You are in a tower room, with stairs leading up and down.", 12, 0, 0},
    {"Tower - 2F", "You are in a tower room, with stairs leading up and down.", 12, 0, 0},
    {"Tower - 3F", "You are in a tower room, with stairs leading up and down.", 12, 0, 0},
    {"Tower - 4F", "You are in a tower room, with stairs leading up and down.", 12, 0, 0},
    {"Tower Roof", "Wind buffets you as you stand on top of the tower.", 13, 0, 0}
};

/* Movement connections: [location][direction] = destination (-1 = invalid) */
/* Directions: 0=North, 1=South, 2=East, 3=West, 4=Up, 5=Down */
int movement[NUM_LOCATIONS][6];

/* Initialize movement connections */
void init_movement(void) {
    memset(movement, -1, sizeof(movement));
    
    /* Town area */
    movement[LOC_TREE_SQUARE][2] = LOC_FOREST_GATE;
    movement[LOC_TREE_SQUARE][0] = LOC_SQUIRRELS_INN;
    movement[LOC_TREE_SQUARE][1] = LOC_GRUBBYS_ARMORY;
    movement[LOC_TREE_SQUARE][3] = LOC_MURAS_MAGIC;
    
    movement[LOC_FOREST_GATE][3] = LOC_TREE_SQUARE;
    movement[LOC_FOREST_GATE][2] = LOC_LAKE_SHORE;
    
    movement[LOC_FOREST_PATH][2] = LOC_BEAR_ROCK;
    movement[LOC_FOREST_PATH][3] = LOC_FOREST_GATE;
    
    movement[LOC_BEAR_ROCK][2] = LOC_SHADOWY_PATH;
    movement[LOC_BEAR_ROCK][3] = LOC_FOREST_PATH;
    
    movement[LOC_SHADOWY_PATH][2] = LOC_BIG_OAKS;
    movement[LOC_SHADOWY_PATH][3] = LOC_BEAR_ROCK;
    
    movement[LOC_BIG_OAKS][2] = LOC_MEADOW;
    movement[LOC_BIG_OAKS][3] = LOC_SHADOWY_PATH;
    
    movement[LOC_MEADOW][2] = LOC_BABBLING_BROOK;
    movement[LOC_MEADOW][3] = LOC_BIG_OAKS;
    
    movement[LOC_BABBLING_BROOK][2] = LOC_RUSHING_FALLS;
    movement[LOC_BABBLING_BROOK][3] = LOC_MEADOW;
    
    movement[LOC_RUSHING_FALLS][5] = LOC_SLIPPERY_PATH;
    movement[LOC_RUSHING_FALLS][3] = LOC_BABBLING_BROOK;
    
    movement[LOC_SLIPPERY_PATH][5] = LOC_RUSHING_POOL;
    movement[LOC_SLIPPERY_PATH][4] = LOC_RUSHING_FALLS;
    
    movement[LOC_RUSHING_POOL][2] = LOC_DARK_TUNNEL;
    movement[LOC_RUSHING_POOL][4] = LOC_SLIPPERY_PATH;
    
    movement[LOC_DARK_TUNNEL][2] = LOC_DARKER_TUNNEL;
    movement[LOC_DARK_TUNNEL][3] = LOC_RUSHING_POOL;
    
    movement[LOC_DARKER_TUNNEL][2] = LOC_DARKEST_TUNNEL;
    movement[LOC_DARKER_TUNNEL][3] = LOC_DARK_TUNNEL;
    
    movement[LOC_DARKEST_TUNNEL][2] = LOC_CAVES_END;
    movement[LOC_DARKEST_TUNNEL][3] = LOC_DARKER_TUNNEL;
    
    movement[LOC_CAVES_END][3] = LOC_DARKEST_TUNNEL;
    
    /* Lake area */
    movement[LOC_LAKE_SHORE][3] = LOC_FOREST_GATE;
    movement[LOC_LAKE_SHORE][2] = LOC_LAKE_HOBS_WEST;
    
    movement[LOC_LAKE_HOBS_WEST][2] = LOC_LAKE_HOBS;
    movement[LOC_LAKE_HOBS_WEST][3] = LOC_LAKE_SHORE;
    
    movement[LOC_LAKE_HOBS][2] = LOC_LAKE_HOBS_EAST;
    movement[LOC_LAKE_HOBS][3] = LOC_LAKE_HOBS_WEST;
    
    movement[LOC_LAKE_HOBS_EAST][2] = LOC_EAST_SHORE;
    movement[LOC_LAKE_HOBS_EAST][3] = LOC_LAKE_HOBS;
    
    movement[LOC_EAST_SHORE][3] = LOC_LAKE_HOBS_EAST;
    movement[LOC_EAST_SHORE][2] = LOC_DUSTY_ROAD;
    
    /* Bad King's realm */
    movement[LOC_DUSTY_ROAD][2] = LOC_STONY_GATE;
    movement[LOC_DUSTY_ROAD][3] = LOC_EAST_SHORE;
    
    movement[LOC_STONY_GATE][3] = LOC_DUSTY_ROAD;
    movement[LOC_STONY_GATE][2] = LOC_EMPTY_SQUARE;
    
    movement[LOC_EMPTY_SQUARE][3] = LOC_STONY_GATE;
    movement[LOC_EMPTY_SQUARE][0] = LOC_TYRS_BATTLE_GEAR;
    movement[LOC_EMPTY_SQUARE][1] = LOC_HAPPY_FACE_INN;
    movement[LOC_EMPTY_SQUARE][2] = LOC_DUSTY_ROAD_EAST;
    
    movement[LOC_DUSTY_ROAD_EAST][2] = LOC_CROSSROADS;
    movement[LOC_DUSTY_ROAD_EAST][3] = LOC_EMPTY_SQUARE;
    
    movement[LOC_CROSSROADS][2] = LOC_NORTH_ROAD;
    movement[LOC_CROSSROADS][3] = LOC_DUSTY_ROAD_EAST;
    
    movement[LOC_NORTH_ROAD][2] = LOC_BRAMBLES1;
    movement[LOC_NORTH_ROAD][3] = LOC_CROSSROADS;
    
    movement[LOC_BRAMBLES1][2] = LOC_BRAMBLES2;
    movement[LOC_BRAMBLES1][3] = LOC_NORTH_ROAD;
    
    movement[LOC_BRAMBLES2][2] = LOC_BRAMBLES3;
    movement[LOC_BRAMBLES2][3] = LOC_BRAMBLES1;
    
    movement[LOC_BRAMBLES3][2] = LOC_EAST_ROAD;
    movement[LOC_BRAMBLES3][3] = LOC_BRAMBLES2;
    
    movement[LOC_EAST_ROAD][2] = LOC_CASTLE_WALL;
    movement[LOC_EAST_ROAD][3] = LOC_BRAMBLES3;
    
    movement[LOC_CASTLE_WALL][3] = LOC_EAST_ROAD;
    movement[LOC_CASTLE_WALL][2] = LOC_GARDEN_PATH;
    
    movement[LOC_GARDEN_PATH][2] = LOC_CASTLE_KEEP;
    movement[LOC_GARDEN_PATH][3] = LOC_CASTLE_WALL;
    
    movement[LOC_CASTLE_KEEP][3] = LOC_GARDEN_PATH;
    movement[LOC_CASTLE_KEEP][2] = LOC_ENTRANCE_HALL;
    
    movement[LOC_ENTRANCE_HALL][3] = LOC_CASTLE_KEEP;
    movement[LOC_ENTRANCE_HALL][4] = LOC_PASSAGE;
    
    movement[LOC_PASSAGE][4] = LOC_THRONE_ROOM;
    movement[LOC_PASSAGE][5] = LOC_ENTRANCE_HALL;
    
    movement[LOC_THRONE_ROOM][5] = LOC_PASSAGE;
    
    /* Tower */
    movement[LOC_TOWER_1F][4] = LOC_TOWER_2F;
    movement[LOC_TOWER_1F][5] = LOC_THRONE_ROOM;
    
    movement[LOC_TOWER_2F][4] = LOC_TOWER_3F;
    movement[LOC_TOWER_2F][5] = LOC_TOWER_1F;
    
    movement[LOC_TOWER_3F][4] = LOC_TOWER_4F;
    movement[LOC_TOWER_3F][5] = LOC_TOWER_2F;
    
    movement[LOC_TOWER_4F][4] = LOC_TOWER_ROOF;
    movement[LOC_TOWER_4F][5] = LOC_TOWER_3F;
    
    movement[LOC_TOWER_ROOF][5] = LOC_TOWER_4F;
}

/* Function prototypes */
void wait_key(void);
void print_slow(const char *text);
int get_random(int max);
int calculate_damage(int attack, int defense);
void update_stats(void);
void print_status(void);
void look_location(void);
void move_player(void);
void search_location(void);
void talk_npc(void);
void cast_spell(void);
void save_game(void);
void load_game(void);
void combat(int enemy_id);
void combat_magic(int enemy_id, int *enemy_hp);
void shop_weapon(int town);
void shop_magic(int town);
void inn(int town);
void game_loop(void);

/* Utility functions */
void wait_key(void) {
    printf("\nPress Enter to continue...");
    getchar();
}

void print_slow(const char *text) {
    printf("%s\n", text);
}

int get_random(int max) {
    return rand() % max;
}

int calculate_damage(int attack, int defense) {
    int variance = get_random(70) + 70; /* 70-139% */
    int damage = (attack * variance / 100) - defense;
    if (damage < 1) damage = 1;
    return damage;
}

void update_stats(void) {
    /* Calculate attack from weapon */
    player.attack = player.strength + weapons[player.weapon].attack;
    
    /* Calculate defense from armor */
    player.defense = armors[player.armor].defense;
    
    /* Ring bonuses */
    if (player.ring == RING_POWER) {
        player.attack += 5;
    } else if (player.ring == RING_SHIELD) {
        player.defense += 3;
    } else if (player.ring == RING_SILVER) {
        player.attack += 15;
        player.defense += 9;
        player.maxhp += 20;
        player.maxmp += 20;
    }
}

void print_status(void) {
    printf("\n%s\n", player.name);
    printf("Level %d    (Exp. %d/%d)     %d Gold\n", 
           player.level, player.exp, player.level * player.level * 4, player.gold);
    printf("HP %d/%d     MP %d/%d\n", player.hp, player.maxhp, player.mp, player.maxmp);
    printf("     Strength %d\n", player.strength);
    printf("Attack %d     Defense %d\n", player.attack, player.defense);
    
    printf("\nSword: %s\n", weapons[player.weapon].name);
    printf("Armor: %s\n", armors[player.armor].name);
    printf("Ring:  %s\n", rings[player.ring].name);
    
    printf("\nSpells:\n");
    for (int i = 0; i < NUM_SPELLS; i++) {
        if (player.spells[i]) {
            printf("      %s\n", spell_data[i].name);
        }
    }
    
    if (player.water_shoes) printf("\nYou also have water shoes");
    if (player.the_key) printf(" and a key");
    if (player.water_shoes || player.the_key) printf("\n");
}

void look_location(void) {
    struct LocationData *loc = &locations[player.location];
    printf("\n%s\n", loc->name);
    printf("%s\n", loc->description);
    
    /* Special location descriptions */
    switch (player.location) {
        case LOC_TREE_SQUARE:
            printf("The square is full of people bustling about. Nearby are an inn, a weapon/armor shop, and a mysterious shop with symbols carved over the door.\n");
            break;
        case LOC_FOREST_GATE:
            printf("A guard is standing nearby, keeping a sharp lookout for monsters. To the east you can see the shore of a lake.\n");
            break;
        case LOC_TALL_TREE:
            printf("You have climbed the tree, using branches and knotholes. It's very peaceful up here, except for a dark cloud you can see to the east.\n");
            break;
        case LOC_RUSHING_FALLS:
            printf("The stream cascades over a cliff and falls about twenty feet to a pool. A trail leads down the cliff nearby.\n");
            break;
        case LOC_CASTLE_KEEP:
            printf("There is a carved skull set into the door. And the door is shut tight.\n");
            break;
        case LOC_THRONE_ROOM:
            printf("On the throne sits a man, dressed in black and red, with a dark crown on his head.\n");
            break;
        case LOC_TOWER_ROOF:
            if (player.location == LOC_TOWER_ROOF) {
                printf("Nearby are standing three figures in purple robes, facing outward and chanting.\n");
            }
            break;
    }
    
    /* Show available directions */
    printf("\nExits: ");
    int has_exit = 0;
    if (movement[player.location][0] >= 0) { printf("North "); has_exit = 1; }
    if (movement[player.location][1] >= 0) { printf("South "); has_exit = 1; }
    if (movement[player.location][2] >= 0) { printf("East "); has_exit = 1; }
    if (movement[player.location][3] >= 0) { printf("West "); has_exit = 1; }
    if (movement[player.location][4] >= 0) { printf("Up "); has_exit = 1; }
    if (movement[player.location][5] >= 0) { printf("Down "); has_exit = 1; }
    if (!has_exit) printf("None");
    printf("\n");
}

void move_player(void) {
    printf("\nWhich direction? (n/s/e/w/u/d): ");
    char dir = getchar();
    getchar(); /* consume newline */
    dir = tolower(dir);
    
    int dest = -1;
    switch (dir) {
        case 'n': dest = movement[player.location][0]; break;
        case 's': dest = movement[player.location][1]; break;
        case 'e': dest = movement[player.location][2]; break;
        case 'w': dest = movement[player.location][3]; break;
        case 'u': dest = movement[player.location][4]; break;
        case 'd': dest = movement[player.location][5]; break;
        default: printf("Invalid direction.\n"); return;
    }
    
    if (dest < 0) {
        printf("You can't go that way.\n");
        return;
    }
    
    /* Special movement checks */
    if (dest == LOC_LAKE_HOBS_WEST || dest == LOC_LAKE_HOBS || dest == LOC_LAKE_HOBS_EAST) {
        if (!player.water_shoes) {
            printf("You can't swim across the lake. Sorry.\n");
            return;
        }
    }
    
    if (dest == LOC_CASTLE_KEEP && !player.the_key) {
        printf("The keep door flatly refuses to open.\n");
        return;
    }
    
    if (dest == LOC_CASTLE_KEEP && player.the_key) {
        printf("The key turns in the lock with an ominous click.\n");
    }
    
    if (dest == LOC_THRONE_ROOM) {
        printf("The throne room is cloaked in shadows. A raspy voice says, \"It is him! Destroy him, my servant!\"\n");
    }
    
    player.location = dest;
    look_location();
    
    /* Random encounter check */
    if (locations[player.location].enemy_type >= 0) {
        if (get_random(3) == 0) { /* 33% chance */
            int enemy_id = locations[player.location].enemy_type + get_random(3);
            if (enemy_id < NUM_ENEMIES) {
                combat(enemy_id);
            }
        }
    }
}

void search_location(void) {
    printf("\n");
    switch (player.location) {
        case LOC_TALL_TREE:
            if (!player.tree_gold) {
                printf("You reach into a hole in the tree and fumble around. Finally you find a pouch containing 80 gold.\n");
                player.gold += 80;
                player.tree_gold = 1;
            } else {
                printf("You search carefully, but find nothing.\n");
            }
            break;
            
        case LOC_RUSHING_POOL:
            printf("You search the pool but find nothing. But you know there must be something around here.\n");
            printf("The roar of the falls catches your attention. You push through the curtain of water...\n");
            player.location = LOC_DARK_TUNNEL;
            look_location();
            break;
            
        case LOC_DARKEST_TUNNEL:
            printf("Your foot kicks something in the dark. You pick it up and find that it's a scroll with the spell of Heal Major written on it.\n");
            player.spells[SPELL_HEAL_MAJOR] = 1;
            break;
            
        case LOC_CAVES_END:
            printf("Light shines through a crack in the ceiling, falling on a treasure chest. But before you can lift the lid...\n");
            combat(ENEMY_SERPENT);
            if (player.hp > 0) {
                printf("You found the Water Shoes!\n");
                player.water_shoes = 1;
            }
            break;
            
        case LOC_LAKE_HOBS:
            printf("Looking into the deep blue of the lake, you see something that glints a different shade. You dive down toward it.\n");
            printf("Then you find something that didn't glint at all...\n");
            printf("You found the Heal Ring!\n");
            player.ring = RING_HEAL;
            break;
            
        case LOC_CROSSROADS:
            printf("In the dust of the road, something catches your eye. You pick it up.\n");
            printf("It is a dull key, made out of some strange metal, with a skull carved into the handle.\n");
            player.the_key = 1;
            break;
            
        case LOC_BRAMBLES2:
            printf("Hidden among the towering brambles is a treasure chest. But before you can lift the lid...\n");
            combat(ENEMY_DEMON_KNIGHT);
            if (player.hp > 0) {
                printf("You found the Dragon Slayer!\n");
                player.weapon = WEAPON_DRAGON_SLAYER;
            }
            break;
            
        case LOC_PASSAGE:
            printf("You search the room. Then you start on the bookcases. When you touch a black-bound book, the bookcase swings around.\n");
            printf("You found the Rune Ring!\n");
            player.ring = RING_RUNE;
            break;
            
        default:
            printf("You search carefully, but find nothing.\n");
            break;
    }
}

void talk_npc(void) {
    printf("\n");
    switch (player.location) {
        case LOC_TREE_SQUARE:
            printf("A well-dressed woman in a carriage tells you, \"Yes, we're leaving town. The Bad King's monsters have been sighted on this shore of the lake more and more lately. We're getting as far away as we can from here.\"\n");
            break;
            
        case LOC_SQUIRRELS_INN:
            inn(0);
            break;
            
        case LOC_GRUBBYS_ARMORY:
            shop_weapon(0);
            break;
            
        case LOC_MURAS_MAGIC:
            shop_magic(0);
            break;
            
        case LOC_FOREST_GATE:
            printf("The guard at the gate says, \"You know, if you're trying to cross the lake, you might try looking for the Water Shoes. I heard they were stolen by a monster a long time ago.\"\n");
            break;
            
        case LOC_TALL_TREE:
            printf("A squirrel chitters at you from the trees. \"Hey! Hey you! I know something you don't! A long time ago, a warrior fleeing the Bad King's monsters stopped by the shore of the lake. He took something shiny off his finger and threw it in the lake.\"\n");
            break;
            
        case LOC_LAKE_SHORE:
            printf("The traveler says, \"Ahhh. Hi there. I've just escaped from the Bad King's realm. I was a prisoner in his dungeons, but I found the castle's skeleton key. I made my way here, avoiding patrols the whole way.\"\n");
            printf("He slaps at his pockets in alarm. \"Shoot! I lost the key!\"\n");
            break;
            
        case LOC_STONY_GATE:
            printf("The tired-looking guard says, \"Oh. Good day, I guess. Pardon me if I seem tired, but I just returned from a three day search with no luck. I was looking for a legendary sword that used to belong to the hero Garion. He fell in battle with the Red Dragon of Thorns.\"\n");
            break;
            
        case LOC_EMPTY_SQUARE:
            printf("You call out a greeting. From behind the corner of a building, a voice whispers, \"The Rune Ring shields you from magical harm. It wards against foul vapors.\"\n");
            break;
            
        case LOC_TYRS_BATTLE_GEAR:
            shop_weapon(1);
            break;
            
        case LOC_HAPPY_FACE_INN:
            inn(1);
            break;
            
        case LOC_THURAS_MAGIC:
            shop_magic(1);
            break;
            
        case LOC_THRONE_ROOM:
            printf("The Bad King gazes down coldly.\n");
            printf("\"So, %s, you are the 'hero' I've been hearing about. Ready yourself! We shall find the truth!!!\"\n", player.name);
            printf("He pulls a dark sword from his belt and advances.\n");
            combat(ENEMY_BAD_KING);
            if (player.hp > 0) {
                printf("\nCongratulations, mighty %s.\n", player.name);
                printf("Your heroic deeds will live on in memory for years to come.\n\n");
                printf("The Bad King's evil castle crumbled to the ground the moment %s left it.\n", player.name);
                printf("The dark clouds dispersed.\n\n");
                printf("As %s walked back along the road, new clouds gathered, and a warm cleansing rain began to fall.\n\n", player.name);
                printf("%s left Dragon Slayer imbedded in a stone at the ruins of the castle, as a reminder of the evil that once existed there.\n", player.name);
                printf("And as years slipped into decades, and flowering vines claimed the ruins, the sword remained, waiting for the far-off day when it would be needed again.\n\n");
                printf("            THE END\n");
                exit(0);
            }
            break;
            
        case LOC_TOWER_ROOF:
            printf("You call out a greeting. The three wizards turn as one and raise their hands.\n");
            combat(ENEMY_WIND_WIZARDS);
            break;
            
        default:
            printf("You call out a greeting, but no one answers.\n");
            break;
    }
}

void cast_spell(void) {
    printf("\n");
    int has_spell = 0;
    for (int i = 0; i < NUM_SPELLS; i++) {
        if (player.spells[i] && (i == SPELL_HEAL || i == SPELL_HEAL_MAJOR || i == SPELL_RETURN)) {
            printf("%c. %s (%d MP)\n", 'a' + has_spell, spell_data[i].name, spell_data[i].mp_cost);
            has_spell++;
        }
    }
    
    if (!has_spell) {
        printf("You don't have any spells you can use now.\n");
        return;
    }
    
    printf("x. Cancel\n");
    printf("Which will you cast? ");
    
    char choice = getchar();
    getchar();
    
    if (choice == 'x') return;
    
    int idx = choice - 'a';
    if (idx == 0 && player.spells[SPELL_HEAL]) {
        if (player.mp < 1) {
            printf("Not enough MP.\n");
            return;
        }
        printf("You have cast Heal.\n");
        int heal = get_random(4) + player.level + 5;
        player.hp += heal;
        if (player.hp > player.maxhp) player.hp = player.maxhp;
        player.mp -= 1;
        printf("Recovered %d HP.\n", heal);
    } else if (idx == 1 && player.spells[SPELL_HEAL_MAJOR]) {
        if (player.mp < 5) {
            printf("Not enough MP.\n");
            return;
        }
        printf("You have cast Heal Major.\n");
        int heal = get_random(15) + player.level * 3 + 20;
        player.hp += heal;
        if (player.hp > player.maxhp) player.hp = player.maxhp;
        player.mp -= 5;
        printf("Recovered %d HP.\n", heal);
    } else if (idx == 2 && player.spells[SPELL_RETURN]) {
        if (player.mp < 1) {
            printf("Not enough MP.\n");
            return;
        }
        printf("You have cast Return.\n");
        player.mp -= 1;
        player.location = LOC_TREE_SQUARE;
        look_location();
    }
}

void save_game(void) {
    FILE *f = fopen(SAVE_FILE, "wb");
    if (!f) {
        printf("Error saving game.\n");
        return;
    }
    fwrite(&player, sizeof(player), 1, f);
    fclose(f);
    printf("\n*** Game Saved ***\n");
}

void load_game(void) {
    FILE *f = fopen(SAVE_FILE, "rb");
    if (!f) {
        printf("No save file found.\n");
        return;
    }
    fread(&player, sizeof(player), 1, f);
    fclose(f);
    update_stats();
    printf("\n*** Game Loaded ***\n");
}

void combat(int enemy_id) {
    struct EnemyData *enemy = &enemies[enemy_id];
    int enemy_hp = enemy->hp;
    int enemy_maxhp = enemy->hp;
    int enemy_asleep = 0;
    
    printf("\nYou are attacked by %s%s!\n", 
           enemy_id == ENEMY_IMP ? "an " : 
           enemy_id == ENEMY_BAD_KING ? "the " : "a ",
           enemy->name);
    
    while (player.hp > 0 && enemy_hp > 0) {
        printf("\nYour HP: %d/%d  MP: %d/%d\n", player.hp, player.maxhp, player.mp, player.maxmp);
        printf("Enemy HP: %d/%d\n", enemy_hp, enemy_maxhp);
        printf("\nCommand? (f/c/r): ");
        
        char cmd = getchar();
        getchar();
        
        if (cmd == 'f') {
            /* Player attacks */
            printf("%s attacks!\n", player.name);
            int damage = calculate_damage(player.attack, enemy->defense);
            
            /* Dragon Slayer special: extra damage to dragons */
            if (player.weapon == WEAPON_DRAGON_SLAYER && 
                (enemy_id == ENEMY_GREEN_DRAGON || enemy_id == ENEMY_BLUE_DRAGON || 
                 enemy_id == ENEMY_RED_DRAGON || enemy_id == ENEMY_BLACK_DRAGON)) {
                damage += 50;
            }
            
            if (enemy_id == ENEMY_BAD_KING && player.weapon != WEAPON_DRAGON_SLAYER) {
                printf("The sword had no effect.\n");
            } else {
                enemy_hp -= damage;
                printf("The %s receives %d damage.\n", enemy->name, damage);
            }
        } else if (cmd == 'c') {
            combat_magic(enemy_id, &enemy_hp);
        } else if (cmd == 'r') {
            /* Try to run */
            printf("%s turned and ran.\n", player.name);
            if (get_random(7) - enemy_id < 1) {
                printf("But there was no escape.\n");
            } else {
                return;
            }
        } else {
            continue;
        }
        
        /* Enemy turn */
        if (enemy_hp > 0 && !enemy_asleep) {
            printf("\nThe %s attacks.\n", enemy->name);
            int damage = calculate_damage(enemy->attack, player.defense);
            
            /* Rune Ring protects against magic */
            if (player.ring == RING_RUNE) {
                damage = damage * 70 / 100;
            }
            
            player.hp -= damage;
            printf("%s takes %d damage.\n", player.name, damage);
        } else if (enemy_asleep) {
            enemy_asleep--;
            if (enemy_asleep == 0) {
                printf("The %s has awoken!\n", enemy->name);
            }
        }
    }
    
    if (player.hp <= 0) {
        printf("\nPoor %s. It seems that the %s was too much for you.\n", player.name, enemy->name);
        printf("Press any key to continue.\n");
        getchar();
        exit(0);
    }
    
    if (enemy_hp <= 0) {
        printf("\nYou have defeated the %s.\n", enemy->name);
        
        /* Special drops */
        if (enemy_id == ENEMY_IMP && player.location == LOC_FOREST_PATH) {
            printf("Received Water Shoes!\n");
            player.water_shoes = 1;
        }
        
        if (enemy->gold > 0) {
            printf("Found %d Gold.\n", enemy->gold);
            player.gold += enemy->gold;
        }
        
        if (enemy->exp > 0) {
            printf("Experience up %d.\n", enemy->exp);
            player.exp += enemy->exp;
            
            /* Level up check */
            int needed = player.level * player.level * 4;
            if (player.exp >= needed) {
                player.level++;
                player.maxhp += 10 + get_random(5);
                player.maxmp += 5 + get_random(3);
                player.strength += 2 + get_random(2);
                player.hp = player.maxhp;
                player.mp = player.maxmp;
                printf("You have reached Level %d!\n", player.level);
                update_stats();
            }
        }
    }
}

void combat_magic(int enemy_id, int *enemy_hp) {
    struct EnemyData *enemy = &enemies[enemy_id];
    
    printf("\n");
    int has_spell = 0;
    int spell_map[8];
    int idx = 0;
    
    for (int i = 0; i < NUM_SPELLS; i++) {
        if (player.spells[i]) {
            printf("%c. %s (%d MP)\n", 'a' + idx, spell_data[i].name, spell_data[i].mp_cost);
            spell_map[idx] = i;
            idx++;
            has_spell = 1;
        }
    }
    
    if (!has_spell) {
        printf("You don't know any spells.\n");
        return;
    }
    
    printf("x. Cancel\n");
    printf("Enter Letter? ");
    
    char choice = getchar();
    getchar();
    
    if (choice == 'x') return;
    
    int spell_idx = choice - 'a';
    if (spell_idx < 0 || spell_idx >= idx) return;
    
    int spell = spell_map[spell_idx];
    
    if (player.mp < spell_data[spell].mp_cost) {
        printf("Not enough MP.\n");
        return;
    }
    
    player.mp -= spell_data[spell].mp_cost;
    printf("%s has cast %s.\n", player.name, spell_data[spell].name);
    
    switch (spell) {
        case SPELL_HEAL: {
            int heal = get_random(4) + player.level + 5;
            player.hp += heal;
            if (player.hp > player.maxhp) player.hp = player.maxhp;
            printf("Recovered %d HP.\n", heal);
            break;
        }
        case SPELL_FIRE: {
            int damage = get_random(5) + player.level + 8;
            /* Bonus vs certain enemies */
            if (enemy_id == ENEMY_VAMPIRE || enemy_id == ENEMY_GREEN_DRAGON) {
                damage += player.level + 14;
            }
            *enemy_hp -= damage;
            printf("The %s receives %d damage.\n", enemy->name, damage);
            break;
        }
        case SPELL_ICE: {
            int damage = get_random(5) + player.level + 8;
            if (enemy_id == ENEMY_SERPENT || enemy_id == ENEMY_WYVERN) {
                damage += player.level + 14;
            }
            *enemy_hp -= damage;
            printf("The %s receives %d damage.\n", enemy->name, damage);
            break;
        }
        case SPELL_LIT: {
            int damage = get_random(5) + player.level + 8;
            if (enemy_id == ENEMY_KNIGHT || enemy_id == ENEMY_BLUE_DRAGON) {
                damage += player.level + 14;
            }
            *enemy_hp -= damage;
            printf("The %s receives %d damage.\n", enemy->name, damage);
            break;
        }
        case SPELL_SLEEP: {
            if (enemy_id > ENEMY_WARLOCK) {
                printf("The spell had no effect.\n");
            } else {
                printf("The %s is asleep.\n", enemy->name);
                /* Sleep lasts 2-3 turns */
            }
            break;
        }
        case SPELL_HEAL_MAJOR: {
            int heal = get_random(15) + player.level * 3 + 20;
            player.hp += heal;
            if (player.hp > player.maxhp) player.hp = player.maxhp;
            printf("Recovered %d HP.\n", heal);
            break;
        }
        case SPELL_RETURN: {
            if (enemy_id > ENEMY_WARLOCK) {
                printf("Return fell apart in mid-chant.\n");
            } else {
                printf("Mist carries %s away.\n", player.name);
                return;
            }
            break;
        }
        case SPELL_NUKE: {
            int damage = get_random(41) + player.level * 3 + 80;
            if (enemy_id > ENEMY_DEMON_KNIGHT) {
                damage = damage * 67 / 100;
            }
            *enemy_hp -= damage;
            printf("The %s receives %d damage.\n", enemy->name, damage);
            break;
        }
    }
}

void shop_weapon(int town) {
    printf("\n");
    if (town == 0) {
        printf("The shopkeeper says, \"Welcome to Grubby's Armory! What can I get for you?\"\n");
        printf("a. Short Sword, 10g\n");
        printf("b. Long Sword, 70g\n");
        printf("c. 2-Hand Broadsword, 300g\n");
        printf("d. Leather, 10g\n");
        printf("e. Chain Mail, 60g\n");
        printf("f. Copper Plate, 315g\n");
    } else {
        printf("The shopkeeper says, \"Well, boy? What'll it be?\"\n");
        printf("a. 2-Hand Broadsword, 260g\n");
        printf("b. Silver Sword, 1000g\n");
        printf("c. Copper Plate, 300g\n");
        printf("d. Iron Plate, 850g\n");
        printf("e. Battle Suit, 6970g\n");
    }
    printf("x. Cancel\n");
    printf("Enter a letter: ");
    
    char choice = getchar();
    getchar();
    
    if (choice == 'x') return;
    
    int cost = 0;
    int item = 0;
    int is_weapon = 1;
    
    if (town == 0) {
        switch (choice) {
            case 'a': cost = 10; item = WEAPON_SHORT_SWORD; break;
            case 'b': cost = 70; item = WEAPON_LONG_SWORD; break;
            case 'c': cost = 300; item = WEAPON_BROADSWORD; break;
            case 'd': cost = 10; item = ARMOR_LEATHER; is_weapon = 0; break;
            case 'e': cost = 60; item = ARMOR_CHAIN_MAIL; is_weapon = 0; break;
            case 'f': cost = 315; item = ARMOR_COPPER_PLATE; is_weapon = 0; break;
            default: return;
        }
    } else {
        switch (choice) {
            case 'a': cost = 260; item = WEAPON_BROADSWORD; break;
            case 'b': cost = 1000; item = WEAPON_SILVER_SWORD; break;
            case 'c': cost = 300; item = ARMOR_COPPER_PLATE; is_weapon = 0; break;
            case 'd': cost = 850; item = ARMOR_IRON_PLATE; is_weapon = 0; break;
            case 'e': cost = 6970; item = ARMOR_BATTLE_SUIT; is_weapon = 0; break;
            default: return;
        }
    }
    
    if (player.gold < cost) {
        printf("\"Sorry, you don't have enough gold.\"\n");
        return;
    }
    
    player.gold -= cost;
    if (is_weapon) {
        player.weapon = item;
    } else {
        player.armor = item;
    }
    update_stats();
    printf("\"Thank you!\"\n");
}

void shop_magic(int town) {
    printf("\n");
    if (town == 0) {
        printf("The cloaked man behind the counter says, \"Ah, welcome to my shop. What can I get for you, good sir?\"\n");
        printf("a. Heal Spell, 30g\n");
        printf("b. Fire Spell, 70g\n");
        printf("c. Ice Spell, 70g\n");
        printf("d. Lit Spell, 70g\n");
        printf("e. Power Ring, 65g\n");
        printf("f. Shield Ring, 85g\n");
    } else {
        printf("The cloaked figure says, \"Ah, a visitor from Forest-For-the-Trees? What can I getcha?\"\n");
        printf("a. Sleep Spell, 400g\n");
        printf("b. Heal Major Spell, 950g\n");
        printf("c. Return Spell, 950g\n");
        printf("d. Nuke Spell, 2250g\n");
        printf("e. Silver Ring, 1420g\n");
    }
    printf("x. Cancel\n");
    printf("Enter a letter: ");
    
    char choice = getchar();
    getchar();
    
    if (choice == 'x') return;
    
    int cost = 0;
    int spell = -1;
    int ring = -1;
    
    if (town == 0) {
        switch (choice) {
            case 'a': cost = 30; spell = SPELL_HEAL; break;
            case 'b': cost = 70; spell = SPELL_FIRE; break;
            case 'c': cost = 70; spell = SPELL_ICE; break;
            case 'd': cost = 70; spell = SPELL_LIT; break;
            case 'e': cost = 65; ring = RING_POWER; break;
            case 'f': cost = 85; ring = RING_SHIELD; break;
            default: return;
        }
    } else {
        switch (choice) {
            case 'a': cost = 400; spell = SPELL_SLEEP; break;
            case 'b': cost = 950; spell = SPELL_HEAL_MAJOR; break;
            case 'c': cost = 950; spell = SPELL_RETURN; break;
            case 'd': cost = 2250; spell = SPELL_NUKE; break;
            case 'e': cost = 1420; ring = RING_SILVER; break;
            default: return;
        }
    }
    
    if (player.gold < cost) {
        printf("\"Sorry, you seem to have misjudged your funds.\"\n");
        return;
    }
    
    player.gold -= cost;
    if (spell >= 0) {
        player.spells[spell] = 1;
    }
    if (ring >= 0) {
        player.ring = ring;
    }
    update_stats();
    printf("\"Thank you very much, sir.\"\n");
}

void inn(int town) {
    int cost = town == 0 ? 10 : 50;
    printf("\n");
    if (town == 0) {
        printf("The innkeeper says, \"Well, it's %d gold for a room for the night. How about it?\"\n", cost);
    } else {
        printf("The innkeeper says, \"Oh, a new visitor! Glad to see you! To celebrate, I'll give you a special rate, %d gold a night! How bout it?\"\n", cost);
    }
    printf("(y)es or (n)o? ");
    
    char choice = getchar();
    getchar();
    
    if (choice == 'y') {
        if (player.gold < cost) {
            printf("\"Sorry, you seem to be short on funds.\"\n");
            return;
        }
        player.gold -= cost;
        player.hp = player.maxhp;
        player.mp = player.maxmp;
        printf("You spend a relaxing night in the inn, and awake refreshed.\n");
    }
}

void game_loop(void) {
    while (1) {
        printf("\nCommand? (m/l/t/s/c/a/b/q): ");
        char cmd = getchar();
        getchar();
        
        switch (tolower(cmd)) {
            case 'm': move_player(); break;
            case 'l': look_location(); break;
            case 't': talk_npc(); break;
            case 's': search_location(); break;
            case 'c': cast_spell(); break;
            case 'a': print_status(); break;
            case 'b': save_game(); break;
            case 'q':
                printf("Are you sure you want to quit? (y)es or (n)o? ");
                char quit = getchar();
                getchar();
                if (quit == 'y') return;
                break;
            case 'h':
                printf("\nCommands:\n");
                printf("  m - Move to a different location\n");
                printf("  l - Look around\n");
                printf("  t - Talk to someone\n");
                printf("  s - Search for treasure\n");
                printf("  c - Cast a spell\n");
                printf("  a - View status\n");
                printf("  b - Save game\n");
                printf("  q - Quit\n");
                break;
            default:
                printf("Invalid command. Press 'h' for help.\n");
                break;
        }
    }
}

int main(void) {
    srand(time(NULL));
    
    printf("========================================\n");
    printf("          THE BAD KING\n");
    printf("  A Text RPG by Griffin Knodle (1991)\n");
    printf("  Ported from Windows 3.x to Linux\n");
    printf("========================================\n\n");
    
    printf("Do you need instructions? (y/n): ");
    char yn = getchar();
    getchar();
    
    if (tolower(yn) == 'y') {
        printf("\nThere are two parts to the game, travel and battle.\n\n");
        printf("Travel commands:\n");
        printf("  m - Move to a different location\n");
        printf("  l - Look around\n");
        printf("  t - Talk to someone (also used to buy from shops)\n");
        printf("  s - Search for treasure and secret passages\n");
        printf("  c - Cast a magic spell\n");
        printf("  a - View your status\n");
        printf("  b - Save your game\n\n");
        printf("Battle commands:\n");
        printf("  f - Fight with your sword\n");
        printf("  c - Cast a magic spell\n");
        printf("  r - Run away (doesn't work with tough monsters)\n\n");
        printf("Press Enter to begin.\n");
        getchar();
    }
    
    printf("\nWhat is your name? (Enter 'quit' to quit): ");
    fgets(player.name, MAX_NAME, stdin);
    player.name[strcspn(player.name, "\n")] = '\0';
    
    if (strcmp(player.name, "quit") == 0) {
        return 0;
    }
    
    printf("\n*** Good Luck, %s! ***\n", player.name);
    
    /* Initialize player */
    player.hp = player.maxhp = 30;
    player.mp = player.maxmp = 15;
    player.strength = 5;
    player.attack = 5;
    player.defense = 0;
    player.exp = 0;
    player.gold = 20;
    player.level = 1;
    player.weapon = WEAPON_NONE;
    player.armor = ARMOR_NONE;
    player.ring = RING_NONE;
    player.water_shoes = 0;
    player.the_key = 0;
    memset(player.spells, 0, sizeof(player.spells));
    player.location = LOC_TREE_SQUARE;
    player.tree_gold = 0;
    
    init_movement();
    
    look_location();
    game_loop();
    
    printf("\nThanks for playing The Bad King!\n");
    return 0;
}
