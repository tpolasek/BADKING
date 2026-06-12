// The Bad King - Linux Terminal Port
// Original: BADKING.EXE (1991, Flatrat Production, Griffin Knodle)
// Ported from Ghidra decompilation of 16-bit Windows NE executable
// Compiled with: g++ -o badking badking.cpp

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

// ============================================================
// Platform abstraction - replaces Windows I/O
// ============================================================

static int rand_seed_lo = 0;
static int rand_seed_hi = 0;

// FUN_1000_29ab equivalent - random(max)
int game_random(int max) {
    if (max <= 0) return 0;
    return rand() % max;
}

// FUN_1000_299b equivalent - seed random
void seed_random() {
    srand((unsigned int)time(nullptr) ^ (unsigned int)getpid());
}

// FUN_1000_69c4 equivalent - get single character
char get_char() {
    char buf[16];
    if (fgets(buf, sizeof(buf), stdin) == nullptr) return 'x';
    return buf[0];
}

// FUN_1000_4b78 equivalent - read line
char* read_line(char* buf) {
    if (fgets(buf, 256, stdin) == nullptr) return nullptr;
    int len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
    return buf;
}

// FUN_1000_4e7c equivalent - print (printf wrapper)
// Overloaded for different argument counts
void print_str(const char* s) { printf("%s", s); fflush(stdout); }
void print_fmt(const char* fmt, const char* s) { printf(fmt, s); fflush(stdout); }
void print_int(const char* fmt, int i) { printf(fmt, i); fflush(stdout); }
void print_2int(const char* fmt, int a, int b) { printf(fmt, a, b); fflush(stdout); }
void print_4int(const char* fmt, int a, int b, int c, int d) { printf(fmt, a, b, c, d); fflush(stdout); }
void print_str_int(const char* fmt, const char* s, int i) { printf(fmt, s, i); fflush(stdout); }
void print_str_2int(const char* fmt, const char* s, int a, int b) { printf(fmt, s, a, b); fflush(stdout); }

// ============================================================
// Game Data Tables (extracted from binary data segment)
// ============================================================

// Enemy data: 17 enemies (index 0-16)
struct Enemy {
    const char* name;
    int max_hp;
    int attack;
    int defense;
    int gold;
    int exp;
};

const Enemy enemies[17] = {
    {"Imp",            4,    3,   0,    4,     1},
    {"Slime",          3,    3,  99,    3,     2},
    {"Goblin",        12,    8,   4,   10,     5},
    {"Mage",          12,    3,   2,   10,     6},
    {"Viper",         26,   12,   8,   31,    19},
    {"Vampire",       22,   16,  19,   59,    34},
    {"Knight",        38,   20,  17,   97,    41},
    {"Wyvern",        50,   30,  26,  162,    72},
    {"Green Dragon",  80,   58,  50,  374,   167},
    {"Demon Knight",  80,   62,  98,   50,   399},
    {"Warlock",      100,   36,  38,  296,   278},
    {"Serpent",      100,   55,  55,  923,   863},
    {"Blue Dragon",  100,   20,  13,  440,   324},
    {"Red Dragon",   390,   73,  36,  440,  1820},
    {"Bad King",     530,   93, 136,    0,     0},
    {"Black Dragon", 530,   78,  75,    0,     0},
    {"Wind Wizards", 530,   40, 115,    0,     0},
};

// Map connections: [location-1][direction 0-3 = a,b,c,d]
// 0 = no exit
const int map_connections[48][4] = {
    {2,3,4,5},     // 1: Tree Square
    {1,0,0,0},     // 2: Squirrel's Nook Inn
    {1,0,0,0},     // 3: Grubby's Armory
    {1,0,0,0},     // 4: Mura's Magic Shop
    {1,6,20,0},    // 5: Forest Gate
    {5,7,8,0},     // 6: Forest Path
    {6,0,0,0},     // 7: Tall Tree
    {6,9,0,0},     // 8: Bear Rock
    {8,10,0,0},    // 9: Shadowy Path
    {9,11,0,0},    // 10: Big Oaks
    {10,12,0,0},   // 11: Meadow
    {11,13,0,0},   // 12: Babbling Brook
    {12,14,0,0},   // 13: Rushing Falls
    {13,15,0,0},   // 14: Slippery Path
    {14,0,0,0},    // 15: Rushing Pool
    {15,17,0,0},   // 16: Dark Tunnel
    {16,18,0,0},   // 17: Darker Tunnel
    {17,19,0,0},   // 18: Darkest Tunnel
    {18,0,0,0},    // 19: Cave's End
    {5,21,0,0},    // 20: Lake Shore
    {20,22,0,0},   // 21: Lake Hobs - West
    {21,23,0,0},   // 22: Lake Hobs
    {22,24,0,0},   // 23: Lake Hobs - East
    {23,25,31,0},  // 24: East Shore
    {24,26,0,0},   // 25: Dusty Road
    {29,28,30,25}, // 26: Stony Gate (N=29,S=28,E=30,W=25)
    {26,0,0,0},    // 27: Empty Square... wait
};

// Hmm, the map above is wrong for some entries. Let me redo from the extracted data properly.
// The extracted data was: Loc N: N=x S=x E=x W=x
// But in the binary the order might differ from what I labeled. Let me use the raw data.

// Actually looking at the code in FUN_1000_0b13:
// iVar3 = DAT_1008_4f6f + -0x61;  // direction index 0-3 from 'a'-'d'
// *(int *)(iVar2 * 8 + iVar3 * 2 + 0x122)
// where iVar2 = location - 1
// So map[location-1][direction] where direction is 0=a,1=b,2=c,3=d
