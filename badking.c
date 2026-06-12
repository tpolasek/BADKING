/* badking.c - The Bad King RPG
 * Original game by Griffin Knodle (1991), Flatrat Production
 * Faithfully ported from decompiled BADKING.EXE (Borland C++, 16-bit Win16)
 *
 * Game logic matches the original binary exactly:
 *   - Encounter zones use original weighted tables with overflow wrapping
 *   - Enemy AI includes spell casting (Fire, Ice, Lit, Sleep, Heal, Breath attacks)
 *   - Rune Ring only affects enemy spells (33% reduction), blocks Sleep/Poison Breath
 *   - Dragon Slayer is required to damage the Bad King (other weapons do 0)
 *   - Level-up: HP += random(4)+7, MP += random(2)+2, no full heal
 *   - Movement uses destination listing (a/b/c/d), not directional input
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* --- Constants --- */
#define MAX_NAME 20
#define NUM_LOCATIONS 49
#define NUM_SPELLS 8
#define SAVE_FILE "badking.sav"

/* Location IDs (0-indexed) */
enum {
    LOC_TREE_SQUARE, LOC_SQUIRRELS_INN, LOC_GRUBBYS_ARMORY, LOC_MURAS_MAGIC,
    LOC_FOREST_GATE, LOC_FOREST_PATH, LOC_TALL_TREE, LOC_BEAR_ROCK,
    LOC_SHADOWY_PATH, LOC_BIG_OAKS, LOC_MEADOW, LOC_BABBLING_BROOK,
    LOC_RUSHING_FALLS, LOC_SLIPPERY_PATH, LOC_RUSHING_POOL,
    LOC_DARK_TUNNEL, LOC_DARKER_TUNNEL, LOC_DARKEST_TUNNEL, LOC_CAVES_END,
    LOC_LAKE_SHORE, LOC_LAKE_HOBS_WEST, LOC_LAKE_HOBS, LOC_LAKE_HOBS_EAST,
    LOC_EAST_SHORE,
    LOC_DUSTY_ROAD, LOC_STONY_GATE, LOC_EMPTY_SQUARE,
    LOC_TYRS_BATTLE_GEAR, LOC_HAPPY_FACE_INN, LOC_THURAS_MAGIC,
    LOC_DUSTY_ROAD_EAST, LOC_CROSSROADS, LOC_NORTH_ROAD,
    LOC_BRAMBLES1, LOC_BRAMBLES2, LOC_BRAMBLES3, LOC_EAST_ROAD,
    LOC_CASTLE_WALL, LOC_GARDEN_PATH, LOC_CASTLE_KEEP,
    LOC_ENTRANCE_HALL, LOC_PASSAGE, LOC_THRONE_ROOM,
    LOC_TOWER_1F, LOC_TOWER_2F, LOC_TOWER_3F, LOC_TOWER_4F,
    LOC_TOWER_ROOF
};

/* Enemy IDs */
enum {
    ENEMY_IMP, ENEMY_SLIME, ENEMY_GOBLIN, ENEMY_VIPER, ENEMY_MAGE,
    ENEMY_VAMPIRE, ENEMY_KNIGHT, ENEMY_WYVERN, ENEMY_GREEN_DRAGON,
    ENEMY_DEMON_KNIGHT, ENEMY_WARLOCK, ENEMY_SERPENT,
    ENEMY_BLUE_DRAGON, ENEMY_RED_DRAGON, ENEMY_BAD_KING,
    ENEMY_BLACK_DRAGON, ENEMY_WIND_WIZARDS
};

/* Weapon/Armor/Ring IDs */
enum {
    WEAPON_NONE, WEAPON_SHORT_SWORD, WEAPON_LONG_SWORD, WEAPON_BROADSWORD,
    WEAPON_SILVER_SWORD, WEAPON_DRAGON_SLAYER
};
enum {
    ARMOR_NONE, ARMOR_LEATHER, ARMOR_CHAIN_MAIL, ARMOR_COPPER_PLATE,
    ARMOR_IRON_PLATE, ARMOR_BATTLE_SUIT
};
enum { RING_NONE, RING_POWER, RING_SHIELD, RING_HEAL, RING_SILVER, RING_RUNE };

/* Spell IDs */
enum {
    SPELL_HEAL, SPELL_FIRE, SPELL_ICE, SPELL_LIT, SPELL_SLEEP,
    SPELL_HEAL_MAJOR, SPELL_RETURN, SPELL_NUKE
};

/* --- Game State --- */
struct GameState {
    char name[MAX_NAME];
    int hp, maxhp, mp, maxmp;
    int strength, attack, defense;
    int exp, gold, level;
    int weapon, armor, ring;
    int prev_ring; /* track previous ring for Silver Ring add/remove */
    int water_shoes, the_key;
    int spells[NUM_SPELLS];
    int location;
    int tree_gold;
};

struct GameState player;

/* Enemy: name, hp, attack, defense, gold, exp */
static const struct { const char *name; int hp, atk, def, gold, exp; } enemies[] = {
    {"Imp",           8,   3,  1,   5,   3},
    {"Slime",        12,   5,  2,   8,   5},
    {"Goblin",       18,   8,  3,  12,   8},
    {"Viper",        25,  10,  4,  18,  12},
    {"Mage",         30,  12,  5,  25,  15},
    {"Vampire",      40,  15,  8,  35,  20},
    {"Knight",       50,  18, 10,  45,  25},
    {"Wyvern",       60,  22, 12,  55,  30},
    {"Green Dragon", 80,  28, 15,  75,  40},
    {"Demon Knight",100,  32, 18, 100,  50},
    {"Warlock",     120,  35, 20, 120,  60},
    {"Serpent",     140,  38, 22, 140,  70},
    {"Blue Dragon", 160,  42, 25, 160,  80},
    {"Red Dragon",  200,  48, 30, 200, 100},
    {"Bad King",    250,  55, 35,   0,   0},
    {"Black Dragon",180,  45, 28, 180,  90},
    {"Wind Wizards", 90,  30, 16,  90,  45}
};

/* Weapon: attack bonus */
static const int weapon_atk[] = {0, 3, 8, 16, 40, 100};
/* Armor: defense bonus */
static const int armor_def[] = {0, 2, 5, 12, 24, 55};

/* Spell: mp cost */
static const int spell_cost[] = {1, 2, 2, 2, 3, 5, 1, 10};
static const char *spell_name[] = {
    "Heal", "Fire", "Ice", "Lit", "Sleep", "Heal Major", "Return", "Nuke"
};

/* Location: name, description */
static const struct { const char *name; const char *desc; } loc_data[] = {
    {"Tree Square",
     "You are standing in the town square of Forest-For-the-Trees.\n"
     "The square is full of people bustling about. Nearby are an inn,\n"
     "a weapon/armor shop, and a mysterious shop with symbols carved\n"
     "over the door. Next to you, a carriage is passing."},
    {"Squirrel's Nook Inn",
     "You are in the common room of the Squirrel's Nook Inn. There\n"
     "are several patrons, but they seem to be more interested in\n"
     "their ale. The innkeeper is standing behind the counter."},
    {"Grubby's Armory",
     "Inside the shop, the air is filled with the smells of forged\n"
     "metal. You hear the sound of hammering from the back room.\n"
     "A well-dressed man walks over to show you his wares."},
    {"Mura's Magic Shop",
     "Strange smells assault you as you enter the shop. Behind a\n"
     "counter is standing a robed and hooded man. He smiles politely\n"
     "at you, and gestures to a table of scrolls and rings."},
    {"Forest Gate",
     "You are standing outside the gate of Forest-For-the-Trees.\n"
     "A guard is standing nearby, keeping a sharp lookout for\n"
     "monsters. To the east you can see the shore of a lake.\n"
     "In all other directions, the forest stretches away."},
    {"Forest Path",
     "You are on a path that runs east and west through the\n"
     "forest. Nearby is a tall oak tree."},
    {"Tall Tree",
     "You have climbed the tree, using branches and knotholes.\n"
     "It's very peaceful up here, except for a dark cloud you\n"
     "can see to the east."},
    {"Bear Rock",
     "You are at the landmark called Bear Rock. It is a large\n"
     "granite rock that looks kind of like a bear's head. The\n"
     "path continues on to the east and west."},
    {"Shadowy Path",
     "The forest around here is so dense that it's hard to see\n"
     "the path. It looks like it thins out a ways to the east."},
    {"Big Oaks",
     "You are walking through a grove of immense oak trees. You\n"
     "can see rays of sunlight towards the west. A squirrel is\n"
     "watching you."},
    {"Meadow",
     "You are in a meadow of tall grasses and sticker bushes. You\n"
     "hear a stream somewhere up ahead."},
    {"Babbling Brook",
     "You are on the banks of a small stream. A little ways\n"
     "downstream, you hear a roaring noise."},
    {"Rushing Falls",
     "You are standing at the top of a waterfall. The stream\n"
     "cascades over a cliff and falls about twenty feet to a\n"
     "pool. A trail leads down the cliff nearby."},
    {"Slippery Path",
     "You are on a narrow path leading down the cliff. It's a bit\n"
     "less steep here, but it's still treacherous, as the spray\n"
     "from the falls coats the ground."},
    {"Rushing Pool",
     "You are standing on the edge of a foaming pool. The falls\n"
     "crash down in a solid sheet of water. At the bottom end\n"
     "of the pool, the stream continues on into dense undergrowth."},
    {"Dark Tunnel",
     "You are standing in a dimly lit tunnel. The falls crash\n"
     "into the pool behind you."},
    {"Darker Tunnel",
     "Further down the tunnel, it becomes very hard to see.\n"
     "Water drips from the ceiling, but other than that it's\n"
     "eerily quiet."},
    {"Darkest Tunnel",
     "You can't see anything in this part of the tunnel. Who\n"
     "knows what might be in the dark."},
    {"Cave's End",
     "You have reached the end of the cave. Light falls through\n"
     "cracks in the ceiling. You hear a hissing noise nearby."},
    {"Lake Shore",
     "You are standing on the shore of Lake Hobs. Clouds fly by\n"
     "in the sky, driven by a stiff wind. But in the distance\n"
     "to the east you see a large dark cloud that doesn't move.\n"
     "There is a traveler resting nearby."},
    {"Lake Hobs - West",
     "You hadn't realized how big Lake Hobs was until you tried\n"
     "to walk across it. The east shore hardly seems any closer."},
    {"Lake Hobs",
     "You are in the middle of Lake Hobs. The water is a deep\n"
     "blue here. You're starting to make out what looks like\n"
     "a road on the far shore."},
    {"Lake Hobs - East",
     "You are drawing near to the eastern shore, at last. Gulls\n"
     "circle overhead, crying sadly. They seem to be warning you."},
    {"East Shore",
     "The east shore of the lake is barren. No plants grow,\n"
     "and even the rocks look sad."},
    {"Dusty Road",
     "This dusty road runs through fields of brown grass. You\n"
     "can tell that this is the Bad King's realm. To the\n"
     "north, a path leads off towards a stone wall with\n"
     "colorful flags flying from it."},
    {"Stony Gate",
     "You've reached a gate. The wall is high and strong-\n"
     "looking. An exhausted guard is leaning against the\n"
     "wall next to the gate."},
    {"Empty Square",
     "Your footsteps echo hollowly on the stones. The central\n"
     "square of Bodies-By-the-Lake seems to be deserted, although\n"
     "you think you hear footsteps pattering away."},
    {"Tyr's Battle Gear",
     "The shop carries an impressive array of weapons and\n"
     "armor. The shopkeeper, an imposing man with a florid\n"
     "face, eyes you impatiently."},
    {"Happy Face Inn",
     "The common room of the inn is painted a garish yellow\n"
     "color. The innkeeper gives you a somewhat forced grin."},
    {"Thura's Magic Shop",
     "This shop looks strangely familiar. So does the robed\n"
     "figure behind the counter, for that matter."},
    {"Dusty Road - East",
     "This stretch of road looks much the same as the last."},
    {"Crossroads",
     "Two roads meet here. The south road is impassable, though.\n"
     "An evil-looking statue watches over the east road.\n"
     "You figure it must be the Bad King."},
    {"North Road",
     "Up ahead, the road disappears into a patch of brambles."},
    {"Brambles",
     "The bramble patch is a thing of twisted, rotting evil.\n"
     "The vines tower overhead like blackened fingers reaching\n"
     "for the sky. You hear things skittering out of sight, and\n"
     "sometimes you think you hear something much bigger in\n"
     "the distance."},
    {"Brambles",
     "The bramble patch is a thing of twisted, rotting evil.\n"
     "The vines tower overhead like blackened fingers reaching\n"
     "for the sky. You hear things skittering out of sight, and\n"
     "sometimes you think you hear something much bigger in\n"
     "the distance."},
    {"Brambles",
     "The bramble patch is a thing of twisted, rotting evil.\n"
     "The vines tower overhead like blackened fingers reaching\n"
     "for the sky. You hear things skittering out of sight, and\n"
     "sometimes you think you hear something much bigger in\n"
     "the distance."},
    {"East Road",
     "A steady rain falls on your face as you trudge along\n"
     "the road. Trees grow alongside, but they look more\n"
     "like prisoners than plants."},
    {"Castle Wall",
     "You have reached the wall of an imposing castle. The\n"
     "whole thing is made out of some kind of dark stone. The\n"
     "gate is open; it looks uncomfortably like a gaping mouth."},
    {"Garden Path",
     "You are on a path paved with flat gray stone. Flowers\n"
     "and herbs grow in regimented rows on either side,\n"
     "some reaching immense size."},
    {"Castle Keep",
     "You are standing at the door to the castle's immense\n"
     "tower. There is a carved skull set into the door. And the\n"
     "door is shut tight."},
    {"Entrance Hall",
     "The entrance hall is hung with gaudy tapestries and\n"
     "decorated with jeweled suits of armor and shelves of\n"
     "books. It looks like the only exits are the front door\n"
     "and a staircase leading up."},
    {"Passage",
     "Torches light the hallway with a flickering glow. Ghastly\n"
     "portraits line the walls. Not all of them are human.\n"
     "One in particular catches your eye. It shows a red dragon\n"
     "curled around a sword, with towering brambles behind\n"
     "it. Its eyes seem to be watching you."},
    {"Throne Room",
     "The throne room is lit with an eerie green light. On\n"
     "the throne sits a man, dressed in black and red, with\n"
     "a dark crown on his head."},
    {"Tower - 1F",
     "You are in a tower room, with stairs leading up and down.\n"
     "You hear the sound of driving rain, and through the\n"
     "window you can see a storm raging outside."},
    {"Tower - 2F",
     "You are in a tower room, with stairs leading up and down.\n"
     "You hear the sound of driving rain, and through the\n"
     "window you can see a storm raging outside."},
    {"Tower - 3F",
     "You are in a tower room, with stairs leading up and down.\n"
     "You hear the sound of driving rain, and through the\n"
     "window you can see a storm raging outside."},
    {"Tower - 4F",
     "You are in a tower room, with stairs leading up and down.\n"
     "You hear the sound of driving rain, and through the\n"
     "window you can see a storm raging outside."},
    {"Tower Roof",
     "Wind buffets you as you stand on top of the tower."},
};

/* Movement: up to 4 exits per location (destination ID, -1 = none) */
static int exits[NUM_LOCATIONS][4];

static void init_exits(void) {
    memset(exits, -1, sizeof(exits));
    int i = 0;
    /* [0] Tree Square */
    exits[i][0]=LOC_SQUIRRELS_INN; exits[i][1]=LOC_GRUBBYS_ARMORY;
    exits[i][2]=LOC_FOREST_GATE;   exits[i][3]=LOC_MURAS_MAGIC;
    i=LOC_SQUIRRELS_INN;  exits[i][0]=LOC_TREE_SQUARE;
    i=LOC_GRUBBYS_ARMORY; exits[i][0]=LOC_TREE_SQUARE;
    i=LOC_MURAS_MAGIC;    exits[i][0]=LOC_TREE_SQUARE;
    i=LOC_FOREST_GATE;    exits[i][0]=LOC_TREE_SQUARE; exits[i][1]=LOC_LAKE_SHORE;
    exits[i][2]=LOC_FOREST_PATH;
    i=LOC_FOREST_PATH;    exits[i][0]=LOC_FOREST_GATE;  exits[i][1]=LOC_TALL_TREE;
    exits[i][2]=LOC_BEAR_ROCK;
    i=LOC_TALL_TREE;      exits[i][0]=LOC_FOREST_PATH;
    i=LOC_BEAR_ROCK;      exits[i][0]=LOC_FOREST_PATH;  exits[i][1]=LOC_SHADOWY_PATH;
    i=LOC_SHADOWY_PATH;   exits[i][0]=LOC_BEAR_ROCK;    exits[i][1]=LOC_BIG_OAKS;
    i=LOC_BIG_OAKS;       exits[i][0]=LOC_SHADOWY_PATH; exits[i][1]=LOC_MEADOW;
    i=LOC_MEADOW;         exits[i][0]=LOC_BIG_OAKS;     exits[i][1]=LOC_BABBLING_BROOK;
    i=LOC_BABBLING_BROOK; exits[i][0]=LOC_MEADOW;       exits[i][1]=LOC_RUSHING_FALLS;
    i=LOC_RUSHING_FALLS;  exits[i][0]=LOC_BABBLING_BROOK; exits[i][1]=LOC_SLIPPERY_PATH;
    i=LOC_SLIPPERY_PATH;  exits[i][0]=LOC_RUSHING_POOL; exits[i][1]=LOC_RUSHING_FALLS;
    i=LOC_RUSHING_POOL;   exits[i][0]=LOC_SLIPPERY_PATH; exits[i][1]=LOC_DARK_TUNNEL;
    i=LOC_DARK_TUNNEL;    exits[i][0]=LOC_RUSHING_POOL; exits[i][1]=LOC_DARKER_TUNNEL;
    i=LOC_DARKER_TUNNEL;  exits[i][0]=LOC_DARK_TUNNEL;  exits[i][1]=LOC_DARKEST_TUNNEL;
    i=LOC_DARKEST_TUNNEL; exits[i][0]=LOC_DARKER_TUNNEL; exits[i][1]=LOC_CAVES_END;
    i=LOC_CAVES_END;      exits[i][0]=LOC_DARKEST_TUNNEL;
    i=LOC_LAKE_SHORE;     exits[i][0]=LOC_FOREST_GATE;  exits[i][1]=LOC_LAKE_HOBS_WEST;
    i=LOC_LAKE_HOBS_WEST; exits[i][0]=LOC_LAKE_SHORE;   exits[i][1]=LOC_LAKE_HOBS;
    i=LOC_LAKE_HOBS;      exits[i][0]=LOC_LAKE_HOBS_WEST; exits[i][1]=LOC_LAKE_HOBS_EAST;
    i=LOC_LAKE_HOBS_EAST; exits[i][0]=LOC_LAKE_HOBS;    exits[i][1]=LOC_EAST_SHORE;
    i=LOC_EAST_SHORE;     exits[i][0]=LOC_LAKE_HOBS_EAST; exits[i][1]=LOC_DUSTY_ROAD;
    i=LOC_DUSTY_ROAD;     exits[i][0]=LOC_EAST_SHORE;   exits[i][1]=LOC_STONY_GATE;
    i=LOC_STONY_GATE;     exits[i][0]=LOC_DUSTY_ROAD;   exits[i][1]=LOC_EMPTY_SQUARE;
    i=LOC_EMPTY_SQUARE;   exits[i][0]=LOC_STONY_GATE;   exits[i][1]=LOC_TYRS_BATTLE_GEAR;
    exits[i][2]=LOC_HAPPY_FACE_INN; exits[i][3]=LOC_DUSTY_ROAD_EAST;
    i=LOC_TYRS_BATTLE_GEAR; exits[i][0]=LOC_EMPTY_SQUARE;
    i=LOC_HAPPY_FACE_INN;  exits[i][0]=LOC_EMPTY_SQUARE;
    i=LOC_THURAS_MAGIC;    exits[i][0]=LOC_EMPTY_SQUARE;
    i=LOC_DUSTY_ROAD_EAST; exits[i][0]=LOC_EMPTY_SQUARE; exits[i][1]=LOC_CROSSROADS;
    i=LOC_CROSSROADS;      exits[i][0]=LOC_DUSTY_ROAD_EAST; exits[i][1]=LOC_NORTH_ROAD;
    i=LOC_NORTH_ROAD;      exits[i][0]=LOC_CROSSROADS;  exits[i][1]=LOC_BRAMBLES1;
    i=LOC_BRAMBLES1;       exits[i][0]=LOC_NORTH_ROAD;   exits[i][1]=LOC_BRAMBLES2;
    i=LOC_BRAMBLES2;       exits[i][0]=LOC_BRAMBLES1;    exits[i][1]=LOC_BRAMBLES3;
    i=LOC_BRAMBLES3;       exits[i][0]=LOC_BRAMBLES2;    exits[i][1]=LOC_EAST_ROAD;
    i=LOC_EAST_ROAD;       exits[i][0]=LOC_BRAMBLES3;    exits[i][1]=LOC_CASTLE_WALL;
    i=LOC_CASTLE_WALL;     exits[i][0]=LOC_EAST_ROAD;    exits[i][1]=LOC_GARDEN_PATH;
    i=LOC_GARDEN_PATH;     exits[i][0]=LOC_CASTLE_WALL;  exits[i][1]=LOC_CASTLE_KEEP;
    i=LOC_CASTLE_KEEP;     exits[i][0]=LOC_GARDEN_PATH;  exits[i][1]=LOC_ENTRANCE_HALL;
    i=LOC_ENTRANCE_HALL;   exits[i][0]=LOC_CASTLE_KEEP;  exits[i][1]=LOC_PASSAGE;
    i=LOC_PASSAGE;         exits[i][0]=LOC_ENTRANCE_HALL; exits[i][1]=LOC_THRONE_ROOM;
    i=LOC_THRONE_ROOM;     exits[i][0]=LOC_PASSAGE;      exits[i][1]=LOC_TOWER_1F;
    i=LOC_TOWER_1F;        exits[i][0]=LOC_THRONE_ROOM;  exits[i][1]=LOC_TOWER_2F;
    i=LOC_TOWER_2F;        exits[i][0]=LOC_TOWER_1F;     exits[i][1]=LOC_TOWER_3F;
    i=LOC_TOWER_3F;        exits[i][0]=LOC_TOWER_2F;     exits[i][1]=LOC_TOWER_4F;
    i=LOC_TOWER_4F;        exits[i][0]=LOC_TOWER_3F;     exits[i][1]=LOC_TOWER_ROOF;
    i=LOC_TOWER_ROOF;      exits[i][0]=LOC_TOWER_4F;
}

/* --- Encounter Zone Table ---
 * Original uses location ranges with weighted enemy selection.
 * Locations are 1-indexed in original; we convert to 0-indexed.
 * Format: { first_loc, last_loc, base_enemy, rand_range, cap, wrap_base }
 * Enemy = base + random(rand_range); if enemy > cap: enemy = wrap_base + r
 */
static int pick_encounter(int loc) {
    int r, e;
    /* loc+1 converts from 0-indexed to original 1-indexed */
    int l = loc + 1;
    if (l >= 1  && l <= 4)  return -1; /* town safe */
    if (l == 5)             return ENEMY_IMP; /* Forest Gate: always Imp */
    if ((l >= 6 && l <= 7) || l == 0x14) {
        /* Forest Path, Tall Tree, Lake Shore */
        r = rand()%2; return r; /* Imp or Slime */
    }
    if (l >= 8 && l <= 0x0b) {
        /* Bear Rock, Shadowy Path, Big Oaks, Meadow */
        r = rand()%3; return r + 1; /* Slime, Goblin, Viper */
    }
    if (l >= 0x0c && l <= 0x0f) {
        /* Babbling Brook, Rushing Falls, Slippery Path, Rushing Pool */
        r = rand()%5; e = r + 2;
        if (e > 4) e = 4;
        return e; /* Goblin(2), Viper(3), Mage(4) - Mage heavy */
    }
    if (l >= 0x10 && l <= 0x13) {
        /* Dark Tunnel, Darker Tunnel, Darkest Tunnel, Cave's End */
        r = rand()%5; e = r + 4;
        if (e > 6) e = r + 2;
        return e; /* Mage(4), Vampire(5), Knight(6) */
    }
    if (l >= 0x15 && l <= 0x17) {
        /* Lake Hobs West/East/Center */
        return ENEMY_WYVERN;
    }
    if (l == 0x18 || l == 0x19 || l == 0x1a || l == 0x1f) {
        /* East Shore, Dusty Road, Stony Gate, Dusty Road East */
        r = rand()%5; e = r + 5;
        if (e > 7) e = r + 3;
        return e; /* Vampire(5), Knight(6), Wyvern(7) */
    }
    if (l >= 0x1b && l <= 0x1e) return -1; /* Village safe */
    if (l >= 0x20 && l <= 0x25) {
        /* Crossroads, North Road, Brambles x3, East Road */
        r = rand()%5; e = r + 6;
        if (e > 8) e = r + 4;
        return e; /* Knight(6), Wyvern(7), Green Dragon(8), Demon Knight(9) */
    }
    if (l >= 0x26 && l <= 0x2a) {
        /* Castle Wall, Garden Path, Castle Keep, Entrance Hall, Passage */
        r = rand()%5; e = r + 8;
        if (e > 10) e = r + 6;
        return e; /* Green Dragon(8), Demon Knight(9), Warlock(10), Serpent(11), Blue Dragon(12) */
    }
    if (l == 0x2b) return -1; /* Throne Room safe */
    if (l >= 0x2c && l <= 0x30) {
        /* Tower 1F-4F, Roof */
        r = rand()%2; return r + 9; /* Demon Knight or Warlock */
    }
    return -1;
}

/* --- Utility --- */
static int rng(int max) { return rand() % max; }

/* Damage calc: FUN_1000_25af - uses floating point multiply in original */
static int calc_damage(int attack, int defense) {
    int variance = rng(31) + 70; /* 70-100 */
    if (attack > 100 && player.weapon == WEAPON_DRAGON_SLAYER) {
        variance = rng(8) + 93; /* 93-100 */
    }
    int dmg = (int)((float)attack * (float)variance / 100.0f) - defense;
    if (dmg < 1) dmg = 1;
    return dmg;
}

/* FUN_1000_2817: float multiply (param1 * param2 / 100) */
static int pct_mult(int value, int pct) {
    return (int)((float)value * (float)pct / 100.0f);
}

/* --- Stats Update --- */
static void update_stats(void) {
    /* Remove previous Silver Ring bonus (maxmp only) */
    if (player.prev_ring == RING_SILVER) {
        player.maxmp -= 20;
        player.mp -= 20;
        if (player.mp < 0) player.mp = 0;
    }
    player.attack = player.strength + weapon_atk[player.weapon];
    player.defense = armor_def[player.armor];
    switch (player.ring) {
    case RING_POWER: player.attack += 5; break;
    case RING_SHIELD: player.defense += 3; break;
    case RING_SILVER:
        player.attack += 15;
        player.defense += 9;
        player.maxmp += 20;
        player.mp += 20;
        break;
    /* RING_HEAL and RING_RUNE: no stat bonuses */
    }
    player.prev_ring = player.ring;
}

/* --- Look --- */
static void look_location(void) {
    printf("\n%s\n%s\n", loc_data[player.location].name,
           loc_data[player.location].desc);
    /* Tower Roof: show wizards only if not already fought (ring != RUNE) */
    if (player.location == LOC_TOWER_ROOF && player.ring != RING_RUNE) {
        printf("Nearby are standing three figures in purple robes, facing\n"
               "outward and chanting.\n");
    }
    /* Show exits */
    int has_exit = 0;
    printf("\n");
    for (int d = 0; d < 4; d++) {
        int dest = exits[player.location][d];
        if (dest >= 0) {
            printf("%c. %s\n", 'a' + d, loc_data[dest].name);
            has_exit = 1;
        }
    }
    if (!has_exit) printf("There are no visible exits.\n");
}

/* --- Forward declarations --- */
static void do_combat(int enemy_id, int special_type);
static void do_talk(void);
static void do_search(void);
static void do_cast_spell(void);
static void shop_weapon(int town);
static void shop_magic(int town);
static void inn(int town);

/* --- Move --- */
static void do_move(void) {
    printf("\nWhere to?\n");
    int dest_list[4];
    int count = 0;
    for (int d = 0; d < 4; d++) {
        int dest = exits[player.location][d];
        if (dest >= 0) {
            printf("%c. %s\n", 'a' + count, loc_data[dest].name);
            dest_list[count++] = dest;
        }
    }
    printf("x. Cancel\n");
    printf("Enter a letter: ");
    int ch = getchar(); getchar();
    if (ch == 'x' || ch < 'a' || ch >= 'a' + count) return;
    int dest = dest_list[ch - 'a'];

    /* Water shoes check: Lake Hobs locations */
    if ((dest == LOC_LAKE_HOBS_WEST || dest == LOC_LAKE_HOBS ||
         dest == LOC_LAKE_HOBS_EAST) && !player.water_shoes) {
        printf("\nYou can't swim across the lake. Sorry.\n");
        return;
    }
    /* Key check: Castle Keep */
    if (dest == LOC_CASTLE_KEEP && !player.the_key) {
        printf("\nThe keep door flatly refuses to open.\n");
        return;
    }
    if (dest == LOC_CASTLE_KEEP && player.the_key) {
        printf("\nThe key turns in the lock with an ominous click.\n");
    }
    /* Throne Room warning */
    if (dest == LOC_THRONE_ROOM) {
        printf("\nThe throne room is cloaked in shadows. A raspy voice says,\n"
               "\"It is him! Destroy him, my servant!\"\n");
    }

    player.location = dest;
    look_location();

    /* Random encounter: ~67% chance (random(3) != 0) */
    int enemy_id = pick_encounter(player.location);
    if (enemy_id >= 0 && rng(3) != 0) {
        do_combat(enemy_id, 0);
    }
}

/* --- Search --- */
static void do_search(void) {
    printf("\n");
    switch (player.location) {
    case LOC_TALL_TREE:
        if (!player.tree_gold) {
            printf("You reach into a hole in the tree and fumble around.\n"
                   "Finally you find a pouch containing 80 gold.\n");
            player.gold += 80;
            player.tree_gold = 1;
        } else {
            printf("You search carefully, but find nothing.\n");
        }
        break;
    case LOC_RUSHING_POOL:
        printf("You search the pool but find nothing. But you know there must\n"
               "be something around here. The roar of the falls catches your\n"
               "attention. You push through the curtain of water...\n");
        player.location = LOC_DARK_TUNNEL;
        look_location();
        break;
    case LOC_DARKEST_TUNNEL:
        printf("Your foot kicks something in the dark. You pick it up and find\n"
               "that it's a scroll with the spell of Heal Major written on it.\n");
        player.spells[SPELL_HEAL_MAJOR] = 1;
        break;
    case LOC_CAVES_END:
        printf("Light shines through a crack in the ceiling, falling on a\n"
               "treasure chest. But before you can lift the lid...\n");
        do_combat(ENEMY_SERPENT, 2); /* special_type 2 = Water Shoes reward */
        break;
    case LOC_LAKE_HOBS:
        printf("Looking into the deep blue of the lake, you see something\n"
               "that glints a different shade. You dive down towards it.\n"
               "Then you find something that didn't glint at all...\n");
        printf("Received Heal Ring!\n");
        player.ring = RING_HEAL;
        update_stats();
        break;
    case LOC_CROSSROADS:
        printf("In the dust of the road, something catches your eye.\n"
               "You pick it up. It is a dull key, made out of some strange\n"
               "metal, with a skull carved into the handle.\n");
        player.the_key = 1;
        break;
    case LOC_BRAMBLES2:
        printf("Hidden among the towering brambles is a treasure chest.\n"
               "But before you can lift the lid...\n");
        do_combat(ENEMY_DEMON_KNIGHT, 4); /* special_type 4 = Dragon Slayer */
        break;
    case LOC_PASSAGE:
        printf("You search the room. Then you start on the bookcases.\n"
               "When you touch a black-bound book, the bookcase swings around.\n");
        printf("Received Rune Ring!\n");
        player.ring = RING_RUNE;
        update_stats();
        break;
    default:
        printf("You search carefully, but find nothing.\n");
        break;
    }
}

/* --- Talk --- */
static void do_talk(void) {
    printf("\n");
    switch (player.location) {
    case LOC_TREE_SQUARE:
        printf("A well-dressed woman in a carriage tells you, \"Yes, we're\n"
               "leaving town. The Bad King's monsters have been sighted on\n"
               "this shore of the lake more and more lately. We're getting\n"
               "as far away as we can from here.\"\n");
        break;
    case LOC_SQUIRRELS_INN:  inn(0); break;
    case LOC_GRUBBYS_ARMORY: shop_weapon(0); break;
    case LOC_MURAS_MAGIC:    shop_magic(0); break;
    case LOC_FOREST_GATE:
        printf("The guard at the gate says, \"You know, if you're trying to\n"
               "cross the lake, you might try looking for the Water Shoes.\n"
               "I heard they were stolen by a monster a long time ago.\"\n");
        break;
    case LOC_TALL_TREE:
        printf("A squirrel chitters at you from the trees. \"Hey! Hey you!\n"
               "I know something you don't! A long time ago, a warrior\n"
               "fleeing the Bad King's monsters stopped by the shore of\n"
               "the lake. He took something shiny off his finger and threw\n"
               "it in the lake.\"\n");
        break;
    case LOC_LAKE_SHORE:
        printf("The traveler says, \"Ahhh. Hi there. I've just escaped from\n"
               "the Bad King's realm. I was a prisoner in his dungeons, but\n"
               "I found the castle's skeleton key. I made my way here,\n"
               "avoiding patrols the whole way. I had to dive into a ditch\n"
               "by the crossroads once, but I made it.\"\n"
               "He slaps at his pockets in alarm. \"Shoot! I lost the key!\"\n");
        break;
    case LOC_STONY_GATE:
        printf("The tired-looking guard says, \"Oh. Good day, I guess.\n"
               "Pardon me if I seem tired, but I just returned from a three\n"
               "day search with no luck. I was looking for a legendary sword\n"
               "that used to belong to the hero Garion. He fell in battle\n"
               "with the Red Dragon of Thorns.\"\n");
        break;
    case LOC_EMPTY_SQUARE:
        printf("You call out a greeting. From behind the corner of a\n"
               "building, a voice whispers, \"The Rune Ring shields you\n"
               "from magical harm. It wards against foul vapors.\"\n");
        break;
    case LOC_TYRS_BATTLE_GEAR: shop_weapon(1); break;
    case LOC_HAPPY_FACE_INN:   inn(1); break;
    case LOC_THURAS_MAGIC:     shop_magic(1); break;
    case LOC_THRONE_ROOM:
        printf("The Bad King gazes down coldly.\n\n"
               "\"So, %s, you are the 'hero' I've been hearing about.\n"
               "Ready yourself! We shall find the truth!!!\"\n"
               "He pulls a dark sword from his belt and advances.\n", player.name);
        do_combat(ENEMY_BAD_KING, 5); /* special_type 5 = endgame */
        break;
    case LOC_TOWER_ROOF:
        printf("You call out a greeting. The three wizards turn as one\n"
               "and raise their hands.\n");
        do_combat(ENEMY_WIND_WIZARDS, 7); /* special_type 7 = Rune Ring reward */
        break;
    default:
        printf("You call out a greeting, but no one answers.\n");
        break;
    }
}

/* --- Out-of-Combat Cast Spell (FUN_1000_18a7) --- */
static void do_cast_spell(void) {
    printf("\n");
    int count = 0;
    int spell_list[3]; /* at most Heal, Heal Major, Return */
    if (player.spells[SPELL_HEAL]) {
        printf("%c. Heal       (1 MP)\n", 'a' + count);
        spell_list[count++] = SPELL_HEAL;
    }
    if (player.spells[SPELL_HEAL_MAJOR]) {
        printf("%c. Heal Major (5 MP)\n", 'a' + count);
        spell_list[count++] = SPELL_HEAL_MAJOR;
    }
    if (player.spells[SPELL_RETURN]) {
        printf("%c. Return     (1 MP)\n", 'a' + count);
        spell_list[count++] = SPELL_RETURN;
    }
    if (count == 0) {
        printf("You don't have any spells you can use now.\n");
        return;
    }
    printf("x. Cancel\n\nWhich will you cast? ");
    int ch = getchar(); getchar();
    if (ch == 'x') return;
    int idx = ch - 'a';
    if (idx < 0 || idx >= count) {
        printf("Enter the letter of your choice.\n");
        return;
    }
    int spell = spell_list[idx];
    switch (spell) {
    case SPELL_HEAL:
        if (player.mp < 1) { printf("Not enough MP.\n"); return; }
        printf("\nYou have cast Heal.\n");
        { int heal = rng(3) + player.level + 5;
          player.hp += heal;
          if (player.hp > player.maxhp) player.hp = player.maxhp;
          player.mp -= 1;
          printf("Recovered %d HP.\n", heal);
        }
        break;
    case SPELL_HEAL_MAJOR:
        if (player.mp < 5) { printf("Not enough MP.\n"); return; }
        printf("\nYou have cast Heal Major.\n");
        { int heal = rng(15) + player.level * 3 + 20;
          player.hp += heal;
          if (player.hp > player.maxhp) player.hp = player.maxhp;
          player.mp -= 5;
          printf("Recovered %d HP.\n", heal);
        }
        break;
    case SPELL_RETURN:
        if (player.mp < 1) { printf("Not enough MP.\n"); return; }
        printf("\nYou have cast Return.\n");
        if (player.location == LOC_THRONE_ROOM) {
            printf("Return fell apart in mid-chant.\n");
            return;
        }
        player.mp -= 1;
        player.location = LOC_TREE_SQUARE;
        look_location();
        break;
    }
}

/* Player sleep counter set by enemy Sleep spell */
static int g_player_sleep;

/* --- Enemy Spell AI (FUN_1000_2284) ---
 * Returns heal amount if enemy healed itself, 0 otherwise.
 * Damage to player is applied directly via player.hp.
 * Sets g_player_sleep if enemy casts Sleep successfully. */
static int enemy_spell(int enemy_id) {
    int action = -1, local_4 = 0;

    switch (enemy_id) {
    case ENEMY_VIPER:          action = 1; local_4 = -3; break; /* Fire */
    case ENEMY_VAMPIRE:        action = 4; break;              /* Sleep */
    case ENEMY_WYVERN:         action = 6; break;              /* Flaming breath */
    case ENEMY_GREEN_DRAGON:   action = 6; local_4 = 15; break;
    case ENEMY_DEMON_KNIGHT:   action = 0; break;              /* Heal self */
    case ENEMY_WARLOCK:        action = rng(2) + 3; break;    /* Lit or Sleep */
    case ENEMY_SERPENT:        /* falls through to physical */
    case ENEMY_BLUE_DRAGON:    action = 8; break;              /* Poison breath */
    case ENEMY_RED_DRAGON:     action = 5; local_4 = 55; break; /* Blitz breath */
    case ENEMY_BAD_KING:
        action = rng(3) + 1; local_4 = 55;
        if (action == 3) action = 5; /* Blitz breath */
        break;
    case ENEMY_BLACK_DRAGON:   action = rng(2) + 7; break;   /* Big nuke or poison */
    case ENEMY_WIND_WIZARDS:   action = rng(2) + 3; local_4 = 40; break;
    default:
        /* Non-spellcasters: fall back to physical attack */
        { int dmg = calc_damage(enemies[enemy_id].atk, player.defense);
          player.hp -= dmg;
          printf("\nThe %s attacks.\n%s takes %d damage.\n",
                 enemies[enemy_id].name, player.name, dmg);
          return 0;
        }
    }

    /* Spell flavor text */
    if (action <= 3 || action == 4) {
        printf("\nThe %s has cast ", enemies[enemy_id].name);
        switch (action) {
        case 0: printf("Heal.\n"); break;
        case 1: printf("Fire.\n"); break;
        case 2: printf("Ice.\n"); break;
        case 3: printf("Lit.\n"); break;
        case 4: printf("Sleep.\n"); break;
        }
    } else {
        printf("\nThe %s has blown ", enemies[enemy_id].name);
        switch (action) {
        case 5: printf("blitz breath.\n"); break;
        case 6: printf("flaming breath.\n"); break;
        case 7: /* big attack */
            printf("fire.\n"); break;
        case 8: printf("poison breath.\n"); break;
        }
    }

    int dmg;
    switch (action) {
    case 0: /* Heal self */
        dmg = rng(4) + 15;
        printf("Recovered %d HP.\n", dmg);
        return dmg; /* caller applies to enemy_hp */
    case 1: /* Fire */
        dmg = rng(5) + local_4 + 8;
        if (player.ring == RING_RUNE) dmg = pct_mult(dmg, 33);
        player.hp -= dmg;
        printf("%s takes %d damage.\n", player.name, dmg);
        break;
    case 2: /* Ice */
        dmg = rng(5) + local_4 + 18;
        if (player.ring == RING_RUNE) dmg = pct_mult(dmg, 33);
        player.hp -= dmg;
        printf("%s takes %d damage.\n", player.name, dmg);
        break;
    case 3: /* Lit */
        dmg = rng(5) + local_4 + 20;
        if (player.ring == RING_RUNE) dmg = pct_mult(dmg, 33);
        player.hp -= dmg;
        printf("%s takes %d damage.\n", player.name, dmg);
        break;
    case 4: /* Sleep */
        if (player.ring == RING_RUNE) {
            printf("%s resisted.\n", player.name);
        } else {
            g_player_sleep = rng(4);
            if (g_player_sleep == 0) {
                printf("The spell had no effect.\n");
                g_player_sleep = 0;
            } else {
                printf("%s is asleep.\n", player.name);
            }
        }
        return 0; /* no damage, caller handles sleep flag */
    case 5: /* Blitz breath */
        dmg = rng(21) + local_4 * 2 + rng(local_4) + rng(local_4) + rng(local_4) + 10;
        if (player.ring == RING_RUNE) dmg = pct_mult(dmg, 33);
        player.hp -= dmg;
        printf("%s receives %d damage.\n", player.name, dmg);
        break;
    case 6: /* Flaming breath */
        dmg = rng(11) + local_4 + 5;
        if (player.ring == RING_RUNE) dmg = pct_mult(dmg, 33);
        player.hp -= dmg;
        printf("%s receives %d damage.\n", player.name, dmg);
        break;
    case 7: /* Big attack */
        dmg = rng(21) + 75;
        if (player.ring == RING_RUNE) dmg = pct_mult(dmg, 33);
        player.hp -= dmg;
        printf("%s receives %d damage.\n", player.name, dmg);
        break;
    case 8: /* Poison breath */
        if (player.ring == RING_RUNE) {
            printf("%s resisted.\n", player.name);
        } else {
            player.hp = 0; /* instant death */
        }
        break;
    }
    return 0;
}

/* --- Combat Spell (FUN_1000_1d4e) --- */
static int combat_spell(int enemy_id, int *enemy_hp, int *enemy_asleep) {
    int i, idx = 0, spell_list[8];
    printf("\n");
    for (i = 0; i < NUM_SPELLS; i++) {
        if (player.spells[i]) {
            printf("%c. %s (%d MP)\n", 'a' + idx, spell_name[i], spell_cost[i]);
            spell_list[idx++] = i;
        }
    }
    if (idx == 0) {
        printf("You don't know any spells.\n");
        return 1;
    }
    printf("x. Cancel\nEnter Letter? ");
    int ch = getchar(); getchar();
    if (ch == 'x') return 1;
    int si = ch - 'a';
    if (si < 0 || si >= idx) return 1;
    int spell = spell_list[si];

    if (player.mp < spell_cost[spell]) {
        printf("Not enough MP.\n"); return 1;
    }
    player.mp -= spell_cost[spell];
    printf("%s has cast %s.\n", player.name, spell_name[spell]);

    int dmg;
    switch (spell) {
    case SPELL_HEAL:
        dmg = rng(4) + player.level + 5;
        player.hp += dmg;
        if (player.hp > player.maxhp) player.hp = player.maxhp;
        printf("Recovered %d HP.\n", dmg);
        return 0;
    case SPELL_FIRE:
        dmg = rng(5) + player.level + 8;
        if (enemy_id == ENEMY_VAMPIRE || enemy_id == ENEMY_DEMON_KNIGHT)
            dmg += player.level + 14;
        if (enemy_id == ENEMY_RED_DRAGON)   dmg = pct_mult(dmg, 10);
        if (enemy_id == ENEMY_BAD_KING)     { printf("The spell had no effect.\n"); return 0; }
        if (enemy_id == ENEMY_BLACK_DRAGON) dmg = pct_mult(dmg, 33);
        *enemy_hp -= dmg;
        printf("The %s receives %d damage.\n", enemies[enemy_id].name, dmg);
        return 0;
    case SPELL_ICE:
        dmg = rng(5) + player.level + 8;
        if (enemy_id == ENEMY_WYVERN || enemy_id == ENEMY_SERPENT)
            dmg += player.level + 14;
        if (enemy_id == ENEMY_DEMON_KNIGHT || enemy_id == ENEMY_WARLOCK ||
            enemy_id == ENEMY_BLACK_DRAGON) dmg = pct_mult(dmg, 33);
        if (enemy_id == ENEMY_BAD_KING) { printf("The spell had no effect.\n"); return 0; }
        *enemy_hp -= dmg;
        printf("The %s receives %d damage.\n", enemies[enemy_id].name, dmg);
        return 0;
    case SPELL_LIT:
        dmg = rng(5) + player.level + 8;
        if (enemy_id == ENEMY_KNIGHT || enemy_id == ENEMY_BLUE_DRAGON)
            dmg += player.level + 14;
        if (enemy_id == ENEMY_DEMON_KNIGHT || enemy_id == ENEMY_WARLOCK ||
            enemy_id == ENEMY_RED_DRAGON || enemy_id == ENEMY_BLACK_DRAGON)
            dmg = pct_mult(dmg, 33);
        if (enemy_id == ENEMY_BAD_KING) { printf("The spell had no effect.\n"); return 0; }
        *enemy_hp -= dmg;
        printf("The %s receives %d damage.\n", enemies[enemy_id].name, dmg);
        return 0;
    case SPELL_SLEEP:
        *enemy_asleep = rng(4);
        if (enemy_id > ENEMY_WARLOCK) *enemy_asleep = 0;
        if (*enemy_asleep == 0) {
            printf("The spell had no effect.\n");
        } else {
            printf("The %s is asleep.\n", enemies[enemy_id].name);
        }
        return 0;
    case SPELL_HEAL_MAJOR:
        dmg = rng(15) + player.level * 3 + 20;
        player.hp += dmg;
        if (player.hp > player.maxhp) player.hp = player.maxhp;
        printf("Recovered %d HP.\n", dmg);
        return 0;
    case SPELL_RETURN:
        if (enemy_id > ENEMY_WARLOCK) {
            printf("The spell had no effect.\n");
            return 0;
        }
        printf("Mist carries %s away.\n", player.name);
        return 2; /* signal: escape */
    case SPELL_NUKE:
        dmg = rng(41) + player.level * 3 + 80;
        if (enemy_id == ENEMY_DEMON_KNIGHT || enemy_id == ENEMY_WARLOCK ||
            enemy_id == ENEMY_RED_DRAGON || enemy_id == ENEMY_BLACK_DRAGON ||
            enemy_id == ENEMY_WIND_WIZARDS) dmg = pct_mult(dmg, 33);
        if (enemy_id == ENEMY_BAD_KING) { printf("The spell had no effect.\n"); return 0; }
        *enemy_hp -= dmg;
        printf("The %s receives %d damage.\n", enemies[enemy_id].name, dmg);
        return 0;
    }
    return 0;
}

/* --- Combat (main loop FUN_1000_0323 area + FUN_1000_062f) --- */
static void do_combat(int enemy_id, int special_type) {
    int enemy_hp = enemies[enemy_id].hp;
    int enemy_maxhp = enemy_hp;
    int enemy_asleep = 0;
    int player_asleep = 0;
    int escaped = 0;
    /* track Demon Knight heal via enemy_hp directly */

    /* Encounter text */
    printf("\n");
    if (enemy_id == ENEMY_IMP)
        printf("You are attacked by an Imp.\n");
    else if (enemy_id == ENEMY_BAD_KING)
        printf("You are attacked by the Bad King!\n");
    else if (enemy_id == ENEMY_WIND_WIZARDS)
        printf("You are attacked by the Wind Wizards.\n");
    else
        printf("You are attacked by a %s.\n", enemies[enemy_id].name);

    while (player.hp > 0 && enemy_hp > 0 && !escaped) {
        /* Player is asleep: skip turn, just press key */
        if (player_asleep > 0) {
            printf("\nYour HP: %d/%d  MP: %d/%d\n", player.hp, player.maxhp,
                   player.mp, player.maxmp);
            if (player_asleep > 1)
                printf("\n%s is asleep.\n", player.name);
            printf("\nPress a key to continue.");
            getchar();
            player_asleep--;
            if (player_asleep == 0) {
                printf("\n%s has awakened!\n", player.name);
            }
            /* Enemy still gets a turn while player sleeps */
            goto enemy_turn;
        }

        /* Show sleeping enemy text */
        if (enemy_asleep > 0) {
            printf("\nYour HP: %d/%d  MP: %d/%d\n", player.hp, player.maxhp,
                   player.mp, player.maxmp);
            if (enemy_asleep > 1)
                printf("\n%s is asleep.\n", enemies[enemy_id].name);
            printf("\nPress a key to continue.");
            getchar();
            enemy_asleep--;
            if (enemy_asleep == 0) {
                printf("\nThe %s has awoken!\n", enemies[enemy_id].name);
            }
            continue; /* skip player turn while enemy sleeps */
        }

        printf("\nYour HP: %d/%d  MP: %d/%d\n", player.hp, player.maxhp,
               player.mp, player.maxmp);
        printf("Enemy HP: %d/%d\n", enemy_hp, enemy_maxhp);
        printf("\nCommand? ");
        int cmd = getchar(); getchar();

        if (cmd == 'f') {
            /* Player physical attack (FUN_1000_1ce8) */
            printf("\n%s attacks!\n", player.name);
            if (enemy_id == ENEMY_BAD_KING && player.weapon != WEAPON_DRAGON_SLAYER) {
                printf("The sword had no effect.\n");
            } else {
                int dmg = calc_damage(player.attack, enemies[enemy_id].def);
                enemy_hp -= dmg;
                printf("The %s receives %d damage.\n", enemies[enemy_id].name, dmg);
            }
        } else if (cmd == 'c') {
            int result = combat_spell(enemy_id, &enemy_hp, &enemy_asleep);
            if (result == 2) { escaped = 1; break; }
        } else if (cmd == 'r') {
            printf("\n%s turned and ran.\n", player.name);
            if (rng(7) - enemy_id < 1) {
                printf("But there was no escape.\n");
            } else {
                escaped = 1;
                break;
            }
        } else {
            continue;
        }

        /* Enemy turn */
enemy_turn:
        if (enemy_hp > 0 && !escaped) {
            if (enemy_asleep > 0) {
                enemy_asleep--;
                if (enemy_asleep > 0)
                    printf("\nThe %s is asleep.\n", enemies[enemy_id].name);
                else
                    printf("\nThe %s has awoken!\n", enemies[enemy_id].name);
            } else {
                /* 60% physical, 40% spell (for spell-capable enemies) */
                int roll = rng(5);
                if (roll < 3) {
                    /* Physical attack (FUN_1000_2222) */
                    printf("\nThe %s %s\n", enemies[enemy_id].name,
                           enemy_id == ENEMY_WIND_WIZARDS ? "attack." : "attacks.");
                    int dmg = calc_damage(enemies[enemy_id].atk, player.defense);
                    player.hp -= dmg;
                    printf("%s takes %d damage.\n", player.name, dmg);
                } else {
                    /* Spell attack (FUN_1000_2284) */
                    g_player_sleep = 0;
                    int heal = enemy_spell(enemy_id);
                    if (heal > 0) {
                        /* Enemy healed itself */
                        enemy_hp += heal;
                        if (enemy_hp > enemy_maxhp) enemy_hp = enemy_maxhp;
                    }
                    if (g_player_sleep > 0) {
                        player_asleep = g_player_sleep;
                    }
                }
            }
        }
        if (player.hp < 1) player.hp = 0;
    }

    if (player.hp <= 0) {
        printf("\nPoor %s. It seems that the %s %s too much for you.\n",
               player.name, enemies[enemy_id].name,
               enemy_id == ENEMY_WIND_WIZARDS ? "were" : "was");
        printf("Press any key to continue.\n");
        getchar();
        exit(0);
    }

    if (enemy_hp <= 0 && !escaped) {
        printf("\nYou have defeated the %s.\n", enemies[enemy_id].name);

        /* Special rewards by special_type */
        if (special_type == 2) {
            printf("Received Water Shoes!\n");
            player.water_shoes = 1;
        }
        if (special_type == 4) {
            printf("Received Dragon Slayer!\n");
            player.weapon = WEAPON_DRAGON_SLAYER;
            update_stats();
        }
        if (special_type == 5) {
            /* Bad King endgame */
            printf("\nCongratulations, mighty %s.\n\n", player.name);
            printf("Your heroic deeds will live on in memory for years to come.\n\n");
            printf("The Bad King's evil castle crumbled to the ground the moment\n"
                   "%s left it. The dark clouds dispersed.\n\n", player.name);
            printf("As %s walked back along the road, new clouds gathered, and\n"
                   "a warm cleansing rain began to fall.\n\n", player.name);
            printf("%s left Dragon Slayer imbedded in a stone at the ruins of\n"
                   "the castle, as a reminder of the evil that once existed there.\n"
                   "And as years slipped into decades, and flowering vines claimed\n"
                   "the ruins, the sword remained, waiting for the far-off day when\n"
                   "it would be needed again.\n\n", player.name);
            printf("            THE END\n");
            exit(0);
        }
        if (special_type == 7) {
            printf("Received Rune Ring!\n");
            player.ring = RING_RUNE;
            update_stats();
        }

        /* Gold and experience */
        if (enemies[enemy_id].gold > 0) {
            printf("Found %d Gold.\n", enemies[enemy_id].gold);
            player.gold += enemies[enemy_id].gold;
        }
        if (enemies[enemy_id].exp > 0) {
            printf("Experience up %d.\n", enemies[enemy_id].exp);
            player.exp += enemies[enemy_id].exp;

            /* Level up check */
            while (player.exp >= player.level * player.level * 4) {
                player.level++;
                player.maxhp += rng(4) + 7;
                player.maxmp += rng(2) + 2;
                player.strength += rng(2) + 2;
                /* Original does NOT heal on level up */
                printf("You have reached Level %i!\n", player.level);
                update_stats();
            }
        }
    }
}

/* --- Shops & Inns --- */
static void shop_weapon(int town) {
    printf("\n");
    if (town == 0) {
        printf("The shopkeeper says, \"Welcome to Grubby's Armory! What\n"
               "can I get for you?\"\n");
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
    if (town == 0) printf("Enter a letter, a-f.\n");
    else           printf("Enter a letter, a-e.\n");
    printf("\"Well?\" ");
    int ch = getchar(); getchar();
    if (ch == 'x') return;
    int cost = 0, item = 0, is_weapon = 1;
    if (town == 0) {
        switch (ch) {
        case 'a': cost=10;  item=WEAPON_SHORT_SWORD; break;
        case 'b': cost=70;  item=WEAPON_LONG_SWORD; break;
        case 'c': cost=300; item=WEAPON_BROADSWORD; break;
        case 'd': cost=10;  item=ARMOR_LEATHER; is_weapon=0; break;
        case 'e': cost=60;  item=ARMOR_CHAIN_MAIL; is_weapon=0; break;
        case 'f': cost=315; item=ARMOR_COPPER_PLATE; is_weapon=0; break;
        default: return;
        }
    } else {
        switch (ch) {
        case 'a': cost=260;  item=WEAPON_BROADSWORD; break;
        case 'b': cost=1000; item=WEAPON_SILVER_SWORD; break;
        case 'c': cost=300;  item=ARMOR_COPPER_PLATE; is_weapon=0; break;
        case 'd': cost=850;  item=ARMOR_IRON_PLATE; is_weapon=0; break;
        case 'e': cost=6970; item=ARMOR_BATTLE_SUIT; is_weapon=0; break;
        default: return;
        }
    }
    if (player.gold < cost) {
        printf(town == 0 ? "\"Sorry, you don't have enough gold.\"\n"
                         : "\"You're broke, kid.\"\n");
        return;
    }
    player.gold -= cost;
    if (is_weapon) player.weapon = item;
    else           player.armor = item;
    update_stats();
    printf(town == 0 ? "\"Thank you!\"\n" : "\"Thankee.\"\n");
}

static void shop_magic(int town) {
    printf("\n");
    if (town == 0) {
        printf("The cloaked man behind the counter says, \"Ah, welcome to\n"
               "my shop. What can I get for you, good sir?\"\n");
        printf("a. Heal Spell, 30g\n");
        printf("b. Fire Spell, 70g\n");
        printf("c. Ice Spell, 70g\n");
        printf("d. Lit Spell, 70g\n");
        printf("e. Power Ring, 65g\n");
        printf("f. Shield Ring, 85g\n");
    } else {
        printf("The cloaked figure says, \"Ah, a visitor from Forest-For-\n"
               "the-Trees? You must have met my brother Mura. What a bore,\n"
               "eh? So what can I getcha?\"\n");
        printf("a. Sleep Spell, 400g\n");
        printf("b. Heal Major Spell, 950g\n");
        printf("c. Return Spell, 950g\n");
        printf("d. Nuke Spell, 2250g\n");
        printf("e. Silver Ring, 1420g\n");
    }
    printf("x. Cancel\n");
    if (town == 0) printf("Enter a letter, a-f.\n");
    else           printf("Enter a letter, a-e.\n");
    printf("\"Anything you like?\" ");
    int ch = getchar(); getchar();
    if (ch == 'x') return;
    int cost = 0, spell = -1, ring = -1;
    if (town == 0) {
        switch (ch) {
        case 'a': cost=30;  spell=SPELL_HEAL; break;
        case 'b': cost=70;  spell=SPELL_FIRE; break;
        case 'c': cost=70;  spell=SPELL_ICE; break;
        case 'd': cost=70;  spell=SPELL_LIT; break;
        case 'e': cost=65;  ring=RING_POWER; break;
        case 'f': cost=85;  ring=RING_SHIELD; break;
        default: return;
        }
    } else {
        switch (ch) {
        case 'a': cost=400;  spell=SPELL_SLEEP; break;
        case 'b': cost=950;  spell=SPELL_HEAL_MAJOR; break;
        case 'c': cost=950;  spell=SPELL_RETURN; break;
        case 'd': cost=2250; spell=SPELL_NUKE; break;
        case 'e': cost=1420; ring=RING_SILVER; break;
        default: return;
        }
    }
    if (player.gold < cost) {
        printf(town == 0 ? "\"Terribly sorry, you seem to have misjudged your funds.\"\n"
                         : "\"Oops! Looks like you're a bit short.\"\n");
        return;
    }
    player.gold -= cost;
    if (spell >= 0) player.spells[spell] = 1;
    if (ring >= 0) { player.ring = ring; update_stats(); }
    printf(town == 0 ? "\"Thank you very much, sir.\"\n"
                     : "\"Thanks. Enjoy!\"\n");
}

static void inn(int town) {
    int cost = town == 0 ? 10 : 50;
    printf("\n");
    if (town == 0) {
        printf("The innkeeper says, \"Well, it's %d gold for a room for\n"
               "the night. How about it?\"\n", cost);
    } else {
        printf("The innkeeper says, \"Oh, a new visitor! Glad to see you!\n"
               "To celebrate, I'll give you a special rate, %d gold a\n"
               "night! How bout it?\"\n", cost);
    }
    printf("\n(y)es or (n)o? ");
    int ch = getchar(); getchar();
    if (ch == 'y') {
        if (player.gold < cost) {
            printf(town == 0 ? "\"Sorry, you seem to be short on funds.\"\n"
                             : "\"Aw, not enough money?\"\n");
            return;
        }
        player.gold -= cost;
        player.hp = player.maxhp;
        player.mp = player.maxmp;
        printf("You spend a relaxing night in the inn, and awake refreshed.\n");
    }
}

/* --- Save / Load --- */
static void save_game(void) {
    if (player.location == LOC_THRONE_ROOM) {
        printf("\nNot a good place to save.\n");
        return;
    }
    FILE *f = fopen(SAVE_FILE, "wb");
    if (!f) { printf("Error saving game.\n"); return; }
    fwrite(&player, sizeof(player), 1, f);
    fclose(f);
    printf("\n*** Game Saved ***\n");
}

static void load_game(void) {
    FILE *f = fopen(SAVE_FILE, "rb");
    if (!f) { printf("No save file found.\n"); return; }
    fread(&player, sizeof(player), 1, f);
    fclose(f);
    update_stats();
    printf("\n*** Game Loaded ***\n");
}

/* --- Status (FUN_1000_1a17) --- */
static void print_status(void) {
    printf("\n%s\n", player.name);
    printf("Level %i    (Exp. %i/%i)     %i Gold\n",
           player.level, player.exp, player.level*player.level*4, player.gold);
    printf("HP %i/%i     MP %i/%i\n", player.hp, player.maxhp, player.mp, player.maxmp);
    printf("     Strength %i\n", player.strength);
    printf("Attack %i     Defense %i\n", player.attack, player.defense);
    printf("\nSword: ");
    switch (player.weapon) {
    case 1: printf("Short Sword"); break;
    case 2: printf("Long Sword"); break;
    case 3: printf("2-Hand Broadsword"); break;
    case 4: printf("Silver Sword"); break;
    case 5: printf("Dragon Slayer"); break;
    }
    printf("\nArmor: ");
    switch (player.armor) {
    case 1: printf("Leather"); break;
    case 2: printf("Chain Mail"); break;
    case 3: printf("Copper Plate"); break;
    case 4: printf("Iron Plate"); break;
    case 5: printf("Battle Suit"); break;
    }
    printf("\nRing: ");
    switch (player.ring) {
    case 1: printf("Power Ring"); break;
    case 2: printf("Shield Ring"); break;
    case 3: printf("Heal Ring"); break;
    case 4: printf("Silver Ring"); break;
    case 5: printf("Rune Ring"); break;
    }
    printf("\n\nSpells:\n");
    if (player.spells[SPELL_HEAL])       printf("      Heal\n");
    if (player.spells[SPELL_FIRE])       printf("      Fire\n");
    if (player.spells[SPELL_ICE])        printf("      Ice\n");
    if (player.spells[SPELL_LIT])        printf("      Lit\n");
    if (player.spells[SPELL_SLEEP])      printf("      Sleep\n");
    if (player.spells[SPELL_HEAL_MAJOR]) printf("      Heal Major\n");
    if (player.spells[SPELL_RETURN])     printf("      Return\n");
    if (player.spells[SPELL_NUKE])       printf("      Nuke\n");
    if (player.water_shoes) {
        printf("\nYou also have water shoes");
        if (player.the_key) printf(" and a key");
        printf("\n");
    }
}

/* --- Main --- */
int main(void) {
    srand(time(NULL));
    printf("\nA Flatrat Production\n\n");
    printf("The Bad King\n\n\n");
    printf("What is your name? (Enter 'quit' to quit)\n");
    fgets(player.name, MAX_NAME, stdin);
    player.name[strcspn(player.name, "\n")] = '\0';
    if (strcmp(player.name, "quit") == 0) return 0;

    /* Check for existing save */
    printf("\n");
    FILE *sf = fopen(SAVE_FILE, "rb");
    if (sf) {
        fclose(sf);
        printf("Your name is not on record, %s.\n", player.name);
        printf("  Start a new game?\n");
        printf("(y)es or (n)o? ");
        int ch = getchar(); getchar();
        if (ch == 'n') {
            /* Load existing save */
            load_game();
            init_exits();
            look_location();
            /* fall through to game loop */
        } else {
            goto new_game;
        }
    } else {
new_game:
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
        player.prev_ring = RING_NONE;
        player.water_shoes = 0;
        player.the_key = 0;
        player.location = LOC_TREE_SQUARE;
        player.tree_gold = 0;
        memset(player.spells, 0, sizeof(player.spells));

        printf("\nDo you need instructions(y/n)?\n");
        int yn = getchar(); getchar();
        if (yn == 'y') {
            printf("\nThere are two parts to the game, travel and battle.\n\n");
            printf("These are the commands used when walking around:\n");
            printf("  m     (m)ove.  Lets you walk to a different location.\n");
            printf("  l     (l)ook.  Gives you a description of where you are.\n");
            printf("  t     (t)alk.  Lets you talk to anyone nearby.  Use\n");
            printf("             this command to buy things from shops, too.\n");
            printf("  s     (s)earch.  Lets you search for treasure, secret\n");
            printf("             passages, and so on.\n");
            printf("  c     (c)ast.  Lets you use a magic spell.\n");
            printf("  a     Status.  Shows a list of statistics, such as HP,\n");
            printf("             MP, strength, etc.\n");
            printf("  b     Save.  Saves your game.\n");
            printf("\nThese are the commands used in battle:\n");
            printf("  f     (f)ight.  Lets you attack the enemy with your sword.\n");
            printf("  c     (c)ast.  Lets you use a magic spell.\n");
            printf("  r     (r)un.  Too hot for you?  Use 'r' to run away.\n");
            printf("             Careful!  It doesn't work with tough monsters.\n");
            printf("\nPress any key to begin.\n");
            getchar();
        }

        printf("\n*** Good Luck, %s ***\n", player.name);
        init_exits();
        look_location();
    }

    /* Game loop */
    for (;;) {
        printf("\nCommand? ");
        int cmd = tolower(getchar()); getchar();
        switch (cmd) {
        case 'm': do_move(); break;
        case 'l': look_location(); break;
        case 't': do_talk(); break;
        case 's': do_search(); break;
        case 'c': do_cast_spell(); break;
        case 'a': print_status(); break;
        case 'b': save_game(); break;
        case 'q':
            printf("\nAre you sure you want to quit? (y)es or (n)o? ");
            { int q = getchar(); getchar();
              if (q == 'y') {
                  printf("\nThis game is freeware. You're under no obligation\n"
                         "to pay for it. You're welcome to give it to a friend,\n"
                         "as long as you don't change it.\n"
                         "   However, if you'd like to support my game-making\n"
                         "efforts, I'd appreciate any contribution you want to\n"
                         "send. Or if you have any suggestions for me, or just\n"
                         "want to tell me what you thought of The Bad King, I'd\n"
                         "appreciate hearing from you.\n"
                         "   I'm working on a game called Tower Quest, which will\n"
                         "be a more complex RPG, with a more developed storyline\n"
                         "and a much longer playing time. I plan to start working\n"
                         "with graphics and action games later on. If you'd like\n"
                         "me to keep you informed on FlatRat Games, write me!\n\n"
                         "   Thanks for trying The Bad King!\n"
                         "Sincerely,\n\n"
                         "     Griffin Knodle\n"
                         "     14611 Vashon Hwy.SW.\n"
                         "     Vashon, WA  98070\n"
                         "On America Online:    GriffinJK\n\n"
                         "(Press a key to continue)\n");
                  getchar();
                  return 0;
              }
            }
            break;
        }
    }
}
