#ifndef DATASEG_H
#define DATASEG_H

extern unsigned char seg_1008[];

#define DS(off) ((const char*)&seg_1008[(off)])
#define DS_MUT(off) ((char*)&seg_1008[(off)])
#define PLAYER_NAME DS_MUT(0x4f0a)
#define SAVE_BUF DS_MUT(0x4f57)
#define MISC_BUF DS_MUT(0x4f60)

#endif
