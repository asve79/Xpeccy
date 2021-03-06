#ifndef X_FILER_H
#define X_FILER_H

#include "libxpeccy/filetypes/filetypes.h"
#include "libxpeccy/spectrum.h"
#include <QFileDialog>

enum {
	FL_NONE = 0,
	FL_TAP,
	FL_TZX,
	FL_WAV,
	FL_SCL,
	FL_TRD,
	FL_FDI,
	FL_UDI,
	FL_DSK,
	FL_TD0,
	FL_SNA,
	FL_Z80,
	FL_SPG,
	FL_RZX,
	FL_HOBETA,
	FL_RAW,
	FL_GB,
	FL_GBC,
	FL_MSX,
	FL_MX1,
	FL_MX2,
	FL_NES,
	FL_T64,
	FL_BKBIN,
	FL_BKIMG
};

enum {
	FG_DISK = -1,
	FG_ALL = 0,
	FG_TAPE = (1 << 10),
	FG_DISK_A,
	FG_DISK_B,
	FG_DISK_C,
	FG_DISK_D,
	FG_SNAPSHOT,
	FG_RZX,
	FG_HOBETA,
	FG_RAW,
	FG_GAMEBOY,
	FG_MSX,
	FG_NES,
	FG_CMDTAPE,
	FG_BKDATA,
	FG_BKDISK
};

enum {
	FH_SPECTRUM = (1 << 12),
	FH_MSX,
	FH_GAMEBOY,
	FH_NES,
	FH_CMD,
	FH_BK,
	FH_DISKS,
	FH_SLOTS
};

void initFileDialog(QWidget*);
//void loadFile(Computer*,const char*, int, int);
//bool saveFile(Computer*,const char*, int, int);
int load_file(Computer* comp, const char* name, int id, int drv);
int save_file(Computer* comp, const char* name, int id, int drv);

bool saveChangedDisk(Computer*,int);

#endif
