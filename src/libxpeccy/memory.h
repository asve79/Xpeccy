#ifndef _MEMOR_H
#define _MEMOR_H

#ifdef __cplusplus
extern "C" {
#endif

#define	MEM_RAM		1
#define	MEM_ROM		2

#define	MEM_BANK0	0
#define	MEM_BANK1	1
#define	MEM_BANK2	2
#define	MEM_BANK3	3
// memory flags
#define	MEM_BREAK	1
// mempage flags
#define	MEM_RDONLY	1
// membyte flags
#define	MEM_BRK_FETCH	1
#define	MEM_BRK_READ	(1<<1)
#define	MEM_BRK_WRITE	(1<<2)

typedef struct {
	int type;
	int num;
	int flags;
	unsigned char data[0x4000];
	unsigned char flag[0x4000];
} MemPage;

typedef struct {
	int flags;
	MemPage ram[256];
	MemPage rom[32];
	MemPage* pt0;
	MemPage* pt1;
	MemPage* pt2;
	MemPage* pt3;
	int memSize;
	int memMask;
	int romMask;	// 0:16K, 1:32K, 3:64K, 7:128K, 15:256K, 31:512K
} Memory;

Memory* memCreate();
void memDestroy(Memory*);

unsigned char memRd(Memory*,unsigned short);
void memWr(Memory*,unsigned short,unsigned char);

void memSetSize(Memory*,int);
void memSetBank(Memory*,int,int,unsigned char);

void memSetPage(Memory*,int,int,char*);
void memGetPage(Memory*,int,int,char*);

unsigned char* memGetPagePtr(Memory*,int,int);

unsigned char memGetCellFlags(Memory*, unsigned short);
void memSwitchCellFlags(Memory*,unsigned short,unsigned char);

MemPage* memGetBankPtr(Memory*,unsigned short);

#if __cplusplus
}
#endif

#endif
