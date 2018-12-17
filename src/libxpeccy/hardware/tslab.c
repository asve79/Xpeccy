#include "hardware.h"

#include <stdio.h>
#include <string.h>

void tslReset(Computer* comp) {
	comp->vid->tsconf.scrPal = 0xf0;
	memset(comp->vid->tsconf.cram,0x00,0x200);
	comp->rom = 0;
	comp->dos = 0;
	comp->tsconf.p21af = 0x04;
	comp->tsconf.Page0 = 0;
	comp->vid->nextbrd = 0xf7;

	comp->vid->tsconf.p00af = 0;
	comp->tsconf.p01af = 0x05;
	comp->vid->tsconf.xOffset = 0;
	comp->vid->tsconf.yOffset = 0;
	comp->vid->tsconf.p07af = 0x0f;

	comp->vid->tsconf.vidPage = 5;
	comp->vid->tsconf.T0XOffset = 0;
	comp->vid->tsconf.T0YOffset = 0;
	comp->vid->tsconf.T1XOffset = 0;
	comp->vid->tsconf.T1YOffset = 0;
	comp->vid->tsconf.tconfig = 0;
	comp->vid->tsconf.intLine = 0;
	comp->vid->intp.x = 0;
	comp->vid->intp.y = 0;
	comp->vid->inten = 1;
	comp->tsconf.vdos = 0;
	tslUpdatePorts(comp->vid);
	tslMapMem(comp);
}

void tslMapMem(Computer* comp) {
// bank0 maping taken from Unreal(TSConf)
	if (comp->tsconf.vdos) {
		memSetBank(comp->mem,0x00,MEM_RAM,0xff, MEM_16K,NULL,NULL,NULL);		// vdos on : ramFF in bank0
	} else if (comp->tsconf.p21af & 8) {
		if (comp->tsconf.p21af & 4)
			memSetBank(comp->mem,0x00,MEM_RAM,comp->tsconf.Page0, MEM_16K,NULL,NULL,NULL);
		else
			memSetBank(comp->mem,0x00,MEM_RAM, (comp->tsconf.Page0 & 0xfc) | ((comp->rom) ? 1 : 0) | (comp->dos ? 0 : 2), MEM_16K,NULL,NULL,NULL);
	} else {
		if (comp->tsconf.p21af & 4)
			memSetBank(comp->mem,0x00,MEM_ROM,comp->tsconf.Page0, MEM_16K,NULL,NULL,NULL);
		else
			memSetBank(comp->mem,0x00,MEM_ROM, (comp->tsconf.Page0 & 0xfc) | ((comp->rom) ? 1 : 0) | (comp->dos ? 0 : 2), MEM_16K,NULL,NULL,NULL);
	}
}

static const unsigned char tslCoLevs[32] = {
	0,11,21,32,42,53,64,74,
	85,95,106,117,127,138,148,159,
	170,180,191,201,212,223,233,244,
	255,255,255,255,255,255,255,255
};

static const unsigned char tsl5bLevs[32] = {
	0,8,16,24,32,41,49,57,
	65,74,82,90,98,106,115,123,
	131,139,148,156,164,172,180,189,
	197,205,213,222,230,238,246,255
};

void tslUpdatePal(Computer* comp) {
	int col;
	for (int i = 0; i < 256; i++) {
		col = (comp->vid->tsconf.cram[(i << 1) + 1] << 8) | (comp->vid->tsconf.cram[i << 1]);
		if (col & 0x8000) {
			comp->vid->pal[i].r = tsl5bLevs[(col >> 10) & 0x1f];
			comp->vid->pal[i].g = tsl5bLevs[(col >> 5) & 0x1f];
			comp->vid->pal[i].b = tsl5bLevs[col  & 0x1f];
		} else {
			comp->vid->pal[i].r = tslCoLevs[(col >> 10) & 0x1f];
			comp->vid->pal[i].g = tslCoLevs[(col >> 5) & 0x1f];
			comp->vid->pal[i].b = tslCoLevs[col  & 0x1f];
		}
	}
}

unsigned char tslMRd(Computer* comp, unsigned short adr, int m1) {
	if (m1 && (comp->dif->type == DIF_BDI)) {
		if (comp->dos && (adr > 0x4000) && (!comp->tsconf.vdos)) {
			comp->dos = 0;
			if (comp->rom) comp->hw->mapMem(comp);	// don't switch ROM0 to ROM2
		}
		if (!comp->dos && ((adr & 0xff00) == 0x3d00) && (comp->rom) && ((comp->tsconf.p21af & 0x04) == 0x00)) {
			comp->dos = 1;
			comp->hw->mapMem(comp);
		}
	}
	return memRd(comp->mem,adr);
}

void tslMWr(Computer* comp, unsigned short adr, unsigned char val) {
	if ((comp->tsconf.flag & 0x10) && ((adr & 0xf000) == comp->tsconf.tsMapAdr)) {
		if ((adr & 0xe00) == 0x000) {				// palete
			comp->vid->tsconf.cram[adr & 0x1ff] = val;
			tslUpdatePal(comp);
		} else if ((adr & 0xe00) == 0x200) {			// sprites
			comp->vid->tsconf.sfile[adr & 0x1ff] = val;
		}
	}
	memWr(comp->mem,adr,val);
}

// in

unsigned char tsInFF(Computer* comp, unsigned short port) {			// dos
	unsigned char res = 0xff;
	if (comp->dif->fdc->flp->virt) {
		comp->tsconf.vdos = 1;
		tslMapMem(comp);
	} else {
		difIn(comp->dif, port, &res, 1);
	}
	return res;
}

unsigned char tsInBDI(Computer* comp, unsigned short port) {			// dos
	unsigned char res = 0xff;
	if (comp->tsconf.vdos) {
		comp->tsconf.vdos = 0;
		tslMapMem(comp);
	} else {
		res = tsInFF(comp, port);
	}
	return res;
}

unsigned char tsIn57(Computer* comp, unsigned short port) {
	unsigned char res = sdcRead(comp->sdc);
//	printf("in #57(%.4X) = %.2X\n",port,res);
	return res;
}

unsigned char tsIn77(Computer* comp, unsigned short port) {
	unsigned char res = 0x00;
	if (comp->sdc->image != NULL) res |= 0x01;	// inserted
	if (comp->sdc->lock) res |= 0x02;		// wrprt
	return res;
}

unsigned char tsInBFF7(Computer* comp, unsigned short port) {
	return (comp->pEFF7 & 0x80) ? cmsRd(comp) : 0xff;
}

// out

void tsOutBDI(Computer* comp, unsigned short port, unsigned char val) {		// dos
	if (comp->tsconf.vdos) {
		comp->tsconf.vdos = 0;
		tslMapMem(comp);
	} else {
		if (comp->dif->fdc->flp->virt) {
			comp->tsconf.vdos = 1;
			tslMapMem(comp);
		} else {
			difOut(comp->dif, port, val, 1);
		}
	}
}

void tsOutFF(Computer* comp, unsigned short port, unsigned char val) {		// dos
	comp->dif->fdc->flp = comp->dif->fdc->flop[val & 3];
	if (comp->dif->fdc->flp->virt) {
		comp->tsconf.vdos = 1;
		tslMapMem(comp);
	} else if (comp->tsconf.vdos) {
		// comp->dif->fdc->fptr = comp->dif->fdc->flop[val & 3];	// out VGSys[1:0]
	} else {
		difOut(comp->dif, port, val, 1);
	}
}

void tsOutFE(Computer* comp, unsigned short port, unsigned char val) {
	comp->vid->brdcol = 0xf0 | (val & 7);
	comp->vid->nextbrd = comp->vid->brdcol;
	comp->beep->lev = (val & 0x10) ? 1 : 0;
	comp->tape->levRec = (val & 0x08) ? 1 : 0;
}

/*
void tsOutFB(Computer* comp, unsigned short port, unsigned char val) {
	sdrvOut(comp->sdrv, 0xfb, val);
}
*/

void tsOut57(Computer* comp, unsigned short port, unsigned char val) {
//	printf("out #57(%.4X),%.2X\n",port,val);
	sdcWrite(comp->sdc,val);
}

void tsOut77(Computer* comp, unsigned short port, unsigned char val) {
	comp->sdc->on = val & 1;
	comp->sdc->cs = (val & 2) ? 1 : 0;
}

void tsOut7FFD(Computer* comp, unsigned short port, unsigned char val) {
	if (comp->p7FFD & 0x20) return;
	comp->p7FFD = val;
	comp->rom = (val & 0x10) ? 1 : 0;
	int num = (val & 7) | ((val & 0xc0) >> 3);	// page (512K)
	if (comp->tsconf.p21af & 0x80) {				// 1x : !a13
		if (~port & 0x2000) num &= 7;
	} else if (comp->tsconf.p21af & 0x40) {			// 01 : 128
		num &= 7;
	}
	memSetBank(comp->mem,0xc0,MEM_RAM,num, MEM_16K,NULL,NULL,NULL);
	comp->vid->tsconf.vidPage = (val & 8) ? 7 : 5;
	tslMapMem(comp);
}

void tsOutBFF7(Computer* comp, unsigned short port, unsigned char val) {
	if (comp->pEFF7 & 0x80) cmsWr(comp,val);
}

void tsOutDFF7(Computer* comp, unsigned short port, unsigned char val) {
	if (comp->pEFF7 & 0x80) comp->cmos.adr = val;
}

void tsOutEFF7(Computer* comp, unsigned short port, unsigned char val) {
	comp->pEFF7 = val;
}

// xxaf

unsigned char tsIn00AF(Computer* comp, unsigned short port) {
	unsigned char res = comp->tsconf.pwr_up ? 0x40 : 0x00;	// b6: PWR_UP (1st run)
	comp->tsconf.pwr_up = 0;
	res |= 3;						// 11 : 5bit VDAC
	return res;
}

void tsOut00AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.p00af = val;}
void tsOut01AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.vidPage = val;}
void tsOut02AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.soxl = val;}
void tsOut03AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.soxh = val & 1;}

void tsOut04AF(Computer* comp, unsigned short port, unsigned char val) {
	comp->vid->tsconf.soyl = val;
	comp->vid->tsconf.scrLine = 0;
}

void tsOut05AF(Computer* comp, unsigned short port, unsigned char val) {
	comp->vid->tsconf.soyh = val & 1;
	comp->vid->tsconf.scrLine = 0;
}

void tsOut06AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.tconfig = val;}
void tsOut07AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.p07af = val;}
void tsOut0FAF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->nextbrd = val;}

void tsOut10AF(Computer* comp, unsigned short port, unsigned char val) {
	comp->tsconf.Page0 = val;
	tslMapMem(comp);
}

void tsOut11AF(Computer* comp, unsigned short port, unsigned char val) {memSetBank(comp->mem,0x40,MEM_RAM,val, MEM_16K,NULL,NULL,NULL);}
void tsOut12AF(Computer* comp, unsigned short port, unsigned char val) {memSetBank(comp->mem,0x80,MEM_RAM,val, MEM_16K,NULL,NULL,NULL);}
void tsOut13AF(Computer* comp, unsigned short port, unsigned char val) {memSetBank(comp->mem,0xc0,MEM_RAM,val, MEM_16K,NULL,NULL,NULL);}

unsigned char tsIn12AF(Computer* comp, unsigned short port) {return comp->mem->map[2].num;}
unsigned char tsIn13AF(Computer* comp, unsigned short port) {return comp->mem->map[3].num;}

void tsOut15AF(Computer* comp, unsigned short port, unsigned char val) {
	comp->tsconf.flag = val & 0x10;		// FM_EN
	comp->tsconf.tsMapAdr = ((val & 0x0f) << 12);
}

void tsOut16AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.TMPage = val;}
void tsOut17AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.T0GPage = val & 0xf8;}
void tsOut18AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.T1GPage = val & 0xf8;}
void tsOut19AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.SGPage = val & 0xf8;}

void tsOut1AAF(Computer* comp, unsigned short port, unsigned char val) {comp->dma.src.l = val;}
void tsOut1BAF(Computer* comp, unsigned short port, unsigned char val) {comp->dma.src.h = val;}
void tsOut1CAF(Computer* comp, unsigned short port, unsigned char val) {comp->dma.src.x = val;}
void tsOut1DAF(Computer* comp, unsigned short port, unsigned char val) {comp->dma.dst.l = val;}
void tsOut1EAF(Computer* comp, unsigned short port, unsigned char val) {comp->dma.dst.h = val;}
void tsOut1FAF(Computer* comp, unsigned short port, unsigned char val) {comp->dma.dst.x = val;}

void tsOut20AF(Computer* comp, unsigned short port, unsigned char val) {
	switch (val & 3) {
		case 0: compSetTurbo(comp,1); break;
		case 1: compSetTurbo(comp,2); break;
		case 2: compSetTurbo(comp,4); break;	// normal
		case 3: compSetTurbo(comp,4); break;	// overclock
	}
}

void tsOut21AF(Computer* comp, unsigned short port, unsigned char val) {
	comp->tsconf.p21af = val;
	comp->p7FFD &= ~0x10;
	if (val & 1) comp->p7FFD |= 0x10;
	tslMapMem(comp);
}

void tsOut22AF(Computer* comp, unsigned short port, unsigned char val) {
	comp->vid->tsconf.hsint = (val << 1);		// base value, real pos = base + shift by line rendering
	comp->vid->intp.x = (val << 1);
}

void tsOut23AF(Computer* comp, unsigned short port, unsigned char val) {
	comp->vid->tsconf.ilinl = val;
	comp->vid->intp.y = comp->vid->tsconf.intLine;
}

void tsOut24AF(Computer* comp, unsigned short port, unsigned char val) {
	comp->vid->tsconf.ilinh = val;
	comp->vid->intp.y = comp->vid->tsconf.intLine;
}

void tsOut26AF(Computer* comp, unsigned short port, unsigned char val) {
	comp->dma.len = val;
}

unsigned char tsIn27AF(Computer* comp, unsigned short port) {return 0x00;}

void tsOut27AF(Computer* comp, unsigned short port, unsigned char val) {
	int cnt, cnt2;
	unsigned char tmp;
	unsigned char* ptr = NULL;
	int sadr = (comp->dma.src.x << 14) | ((comp->dma.src.h & 0x3f) << 8) | (comp->dma.src.l & 0xfe);
	int dadr = (comp->dma.dst.x << 14) | ((comp->dma.dst.h & 0x3f) << 8) | (comp->dma.dst.l & 0xfe);
	int lcnt = (comp->dma.len + 1) << 1;
	comp->vid->tsconf.dmabytes = (comp->dma.len + 1) * (comp->dma.num + 1);
	switch (val & 0x87) {
		case 0x01:		// ram->ram
			for (cnt = 0; cnt <= comp->dma.num; cnt++) {
				for (cnt2 = 0; cnt2 < lcnt; cnt2++) {
					comp->mem->ramData[dadr + cnt2] = comp->mem->ramData[sadr + cnt2];
				}
				sadr += (val & 0x20) ? ((val & 0x08) ? 0x200 : 0x100) : lcnt;		// SALGN
				dadr += (val & 0x10) ? ((val & 0x08) ? 0x200 : 0x100) : lcnt;		// DALGN
			}
			break;
		case 0x81:		// blitter
			for (cnt = 0; cnt <= comp->dma.num; cnt++) {
				for (cnt2 = 0; cnt2 < lcnt; cnt2++) {
					tmp = comp->mem->ramData[sadr + cnt2];
					if (val & 0x08) {
						if (tmp != 0) comp->mem->ramData[dadr + cnt2] = tmp;
					} else {
						if (tmp & 0xf0) {
							comp->mem->ramData[dadr + cnt2] &= 0x0f;
							comp->mem->ramData[dadr + cnt2] |= (tmp & 0xf0);
						}
						if (tmp & 0x0f) {
							comp->mem->ramData[dadr + cnt2] &= 0xf0;
							comp->mem->ramData[dadr + cnt2] |= (tmp & 0x0f);
						}
					}
					// comp->mem->ramData[dadr + cnt2] = comp->mem->ramData[sadr + cnt2];
				}
				sadr += (val & 0x20) ? ((val & 0x08) ? 0x200 : 0x100) : lcnt;		// SALGN
				dadr += (val & 0x10) ? ((val & 0x08) ? 0x200 : 0x100) : lcnt;		// DALGN
			}
			break;
		case 0x02:		// SPI->RAM
//			printf("spi->ram\t%.2X:%.4X,%.2X:%.3X\n",comp->dma.dst.x,dadr & 0x3fff,comp->dma.num,lcnt);
			for (cnt = 0; cnt <= comp->dma.num; cnt++) {
				for (cnt2 = 0; cnt2 < lcnt; cnt2++) {
					comp->mem->ramData[dadr + cnt2] = sdcRead(comp->sdc);
				}
				dadr += (val & 0x10) ? ((val & 0x08) ? 0x200 : 0x100) : lcnt;
			}
			break;
		case 0x82:		// RAM->SPI
			for (cnt = 0; cnt <= comp->dma.num; cnt++) {
				for (cnt2 = 0; cnt2 < lcnt; cnt2++) {
					sdcWrite(comp->sdc, comp->mem->ramData[sadr + cnt2]);
				}
				sadr += (val & 0x20) ? ((val & 0x08) ? 0x200 : 0x100) : lcnt;
			}
			break;
		case 0x03:		// IDE->RAM
			for (cnt = 0; cnt <= comp->dma.num; cnt++) {
				for (cnt2 = 0; cnt2 < lcnt; cnt2++) {
					if (!ideIn(comp->ide, 0x00, &tmp, 1)) tmp = 0xff;
					comp->mem->ramData[dadr + cnt2] = tmp;
				}
				dadr += (val & 0x10) ? ((val & 0x08) ? 0x200 : 0x100) : lcnt;
			}
			break;
		case 0x83:		// RAM->IDE
			for (cnt = 0; cnt <= comp->dma.num; cnt++) {
				for (cnt2 = 0; cnt2 < lcnt; cnt2++) {
					ideOut(comp->ide, 0x00, comp->mem->ramData[sadr + cnt2], 1);
				}
				sadr += (val & 0x20) ? ((val & 0x08) ? 0x200 : 0x100) : lcnt;
			}
			break;
		case 0x04:		// FILL->RAM
			for (cnt = 0; cnt <= comp->dma.num; cnt++) {
				for (cnt2 = 0; cnt2 < lcnt; cnt2++) {
					comp->mem->ramData[dadr + cnt2] = comp->mem->ramData[sadr + (cnt2 & 1)];
				}
				dadr += (val & 0x10) ? ((val & 0x08) ? 0x200 : 0x100) : lcnt;		// DALGN
			}
			break;
		case 0x84:
		case 0x85:		// RAM->SFILE
			ptr = (val & 1) ? comp->vid->tsconf.sfile : comp->vid->tsconf.cram;
			for (cnt2 = 0; cnt2 < lcnt; cnt2++) {
				*(ptr + ((dadr + cnt2) & 0x1ff)) = comp->mem->ramData[sadr + cnt2];
			}
			if (~val & 1) tslUpdatePal(comp);
			break;
		default:
			printf("0x27AF: unsupported src-dst: %.2X\n",val & 0x87);
			// comp->brk = 1;
			break;
	}
	comp->dma.src.x = ((sadr & 0x3fc000) >> 14);
	comp->dma.src.h = ((sadr & 0x3f00) >> 8);
	comp->dma.src.l = sadr & 0xff;
	comp->dma.dst.x = ((dadr & 0x3fc000) >> 14);
	comp->dma.dst.h = ((dadr & 0x3f00) >> 8);
	comp->dma.dst.l = dadr & 0xff;
	if (comp->vid->inten & 4)
		comp->vid->intDMA = 1;
}

void tsOut28AF(Computer* comp, unsigned short port, unsigned char val) {
	comp->dma.num = val;
}

void tsOut29AF(Computer* comp, unsigned short port, unsigned char val) {
	// comp->tsconf.FDDVirt = val;
	comp->dif->fdc->flop[0]->virt = (val & 0x01) ? 1 : 0;
	comp->dif->fdc->flop[1]->virt = (val & 0x02) ? 1 : 0;
	comp->dif->fdc->flop[2]->virt = (val & 0x04) ? 1 : 0;
	comp->dif->fdc->flop[3]->virt = (val & 0x08) ? 1 : 0;
}

void tsOut2AAF(Computer* comp, unsigned short port, unsigned char val) {
	comp->vid->inten = val;
	if (~val & 2) comp->vid->intLINE = 0;
	if (~val & 4) comp->vid->intDMA = 0;
}

void tsOut40AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.t0xl = val;}
void tsOut41AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.t0xh = val & 1;}
void tsOut42AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.t0yl = val;}
void tsOut43AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.t0yh = val & 1;}
void tsOut44AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.t1xl = val;}
void tsOut45AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.t1xh = val & 1;}
void tsOut46AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.t1yl = val;}
void tsOut47AF(Computer* comp, unsigned short port, unsigned char val) {comp->vid->tsconf.t1yh = val & 1;}


unsigned char tsInDBG(Computer* comp, unsigned short port) {
#ifdef ISDEBUG
    printf("WARNING UNKNOWN IN request. port %d (%#08x) vol 0xFFh\n", port, port);
#endif
    return 255;
}

void tsOutDBG(Computer* comp, unsigned short port, unsigned char val) {
#ifdef ISDEBUG
    printf("WARNING UNKNOWN OUT request. port %d (%#08x) vol 0x%02hhxh\n", port, port, val);
#endif
}

unsigned char tsInc2EF(Computer* comp, unsigned short port) { // ERS_SR port
    unsigned char val=0;
    int ret = rs232_havein(comp->rs232->tty_fd);
    if (ret > 0 ) val = 1;
#ifdef ISDEBUG
    printf("ERS_SR IN request. descriptor %d. port %d [%#08x] vol 0x%02hhxh ioctl returns %d\n", comp->rs232->tty_fd, port, port, val,ret);
#endif
    return val;
}

unsigned char  tsInc3EF(Computer* comp, unsigned short port) { // ERS_ST port
    unsigned char val = rs232_canwrite(comp->rs232->tty_fd);
#ifdef ISDEBUG
    printf("ERS_ST IN request. port %d [%#08x] vol 0x%02hhxh\n", port, port, val);
#endif
    return val;
}

unsigned char tsIn00EF(Computer* comp, unsigned short port) {  //ERS_RD port
    unsigned char val;
    int readed=read(comp->rs232->tty_fd,&val,1);
#ifdef ISDEBUG
    printf("ERS_RD IN request. port %d [%#08x] vol 0x%02hhxh readed %d\n", port, port, val, readed);
#endif
    return val;
}

void tsOut00EF(Computer* comp, unsigned short port, unsigned char val) { //ERS_RD port
    //printf("DBG: descriptor %d\n",comp->rs232);
#ifdef ISDEBUG
    printf("ERS_RD OUT request. port %d [%#08x] vol 0x%02hhxh\n", port, port, val);
#endif
    rs232_write(comp->rs232->tty_fd, val);
}

unsigned char tsInF8EF(Computer* comp, unsigned short port) {  //DLL or DAT port
    unsigned char val;
    if (comp->tsconf.pfbef & 0x80){ // if LCR & 0x80 == 1, then return DLL status
#ifdef ISDEBUG
        printf("DLL IN request (ECR & x80==1). port %d [%#08x] vol 0x%02hhxh readed %d\n", port, port, comp->tsconf.pf8ef, 1);
#endif
        return comp->tsconf.pf8ef;
    } else {
        long readed=read(comp->rs232->tty_fd,&val,1);
#ifdef ISDEBUG
        printf("DAT IN request. port %d [%#08x] vol 0x%02hhxh readed %lu\n", port, port, val, readed);
#endif
        return val;
    }
}

void tsOutF8EF(Computer* comp, unsigned short port, unsigned char val) { //DLL or DAT port
    //printf("DBG: descriptor %d\n",comp->rs232);
    if (comp->tsconf.pfbef & 0x80){ // if LCR & 0x80 == 1, then return DLL status
        comp->tsconf.pf8ef = val;
#ifdef ISDEBUG
        printf("DLL set to 0x%02hhxh\n",val);
#endif
    } else {
#ifdef ISDEBUG
        printf("DAT OUT request. port %d [%#08x] vol 0x%02hhxh\n", port, port, val);
#endif
        rs232_write(comp->rs232->tty_fd, val);
    }
}

unsigned char tsInF9EF(Computer* comp, unsigned short port) {  //DLM or IER port
    unsigned char val;
    if (comp->tsconf.pfbef & 0x80){ // if LCR & 0x80 == 1, then return DLL status
#ifdef ISDEBUG
        printf("DLM IN request (ECR & x80==1). port %d [%#08x] vol 0x%02hhxh readed %d\n", port, port, comp->tsconf.pf8ef, 1);
#endif
        return comp->tsconf.pf9ef;
    } else {
#ifdef ISDEBUG
        printf("IER IN request. port %d [%#08x] vol 0x%02hhxh notthing to do\n", port, port);
#endif
        return 0;
    }
}

void tsOutF9EF(Computer* comp, unsigned short port, unsigned char val) { //DLM or DAT port
    //printf("DBG: descriptor %d\n",comp->rs232);
    if (comp->tsconf.pfbef & 0x80){ // if LCR & 0x80 == 1, then return DLM status
        comp->tsconf.pf9ef = val;
#ifdef ISDEBUG
        printf("DLM set to 0x%02hhxh\n",val);
    } else {
        printf("IER OUT request. port %d [%#08x] notrhing to do\n", port, port);
#endif
    }
}

unsigned char tsInFAEF(Computer* comp, unsigned short port) {  //ISR port (not used)
#ifdef ISDEBUG
    printf("ISR IN request. port %d [%#08x]. not used. return 0\n", port, port);
#endif
    return 0;
}

void tsOutFAEF(Computer* comp, unsigned short port, unsigned char val) { //FCR port fifo control
#ifdef ISDEBUG
    if (val & 0x01){
        if (val & 0x02){
            printf("FIFO clear rcv bufer. dummy\n");
        }
        if (val & 0x04){
            printf("FIFO clear send bufer. dummy\n");
        }
    } else {
        printf("FIFO control diabled (send vol 0x%02hhxh)\n",val);
    }
#endif
}

unsigned char tsInFBEF(Computer* comp, unsigned short port) {  //LCR port (line control)
    unsigned char val=comp->tsconf.pfbef;
#ifdef ISDEBUG
    printf("LCR IN request. port %d [%#08x] vol 0x%02hhxh\n", port, port, val);
#endif
    return val;
}

void tsOutFBEF(Computer* comp, unsigned short port, unsigned char val) { //SET LCR port
    comp->tsconf.pfbef = val;
    printf("LCR set to 0x%02hhxh\n",val);
}

unsigned char tsInFCEF(Computer* comp, unsigned short port) {  //MCR port (modem control)
    /* dummy. temporaty nothing to to with serial port */
    unsigned char val=comp->tsconf.pfcef;
#ifdef ISDEBUG
    printf("MCR IN request. port %d [%#08x] vol 0x%02hhxh\n", port, port, val);
#endif
    return val;
}

void tsOutFCEF(Computer* comp, unsigned short port, unsigned char val) { //MCR port (modem control)
    /* dummy. temporaty nothing to to with serial port */
    comp->tsconf.pfcef = val;
#ifdef ISDEBUG
    printf("MCR set to 0x%02hhxh\n",val);
#endif
}

/* in progress */
unsigned char tsInFDEF(Computer* comp, unsigned short port) {  //LSR port (line status)
    unsigned char val=0;
    if (rs232_havein(comp->rs232->tty_fd) > 0) val = val | 1;

    val = val | 0x60; //emulate transmit empty, THR empty
    return val;
}


unsigned char tsInFEEF(Computer* comp, unsigned short port) {  //MSR port (modem status)
    unsigned char val=comp->tsconf.pfeef;
    comp->tsconf.pfeef = comp->tsconf.pfeef & 0xfe; //reset CTS status after read state
#ifdef ISDEBUG
    /* todo: check modem status if needed */
    printf("MSR IN request. port %d [%#08x] vol 0x%02hhxh\n", port, port, val);
    if (comp->tsconf.pfeef != val){
        printf("MSR: CTS was reset!\n");
    }
#endif
    return val;
}

unsigned char tsInFFEF(Computer* comp, unsigned short port) {  //SPR port
    unsigned char val=comp->tsconf.pffef;
#ifdef ISDEBUG
    printf("SPR IN request. port %d [%#08x] vol 0x%02hhxh\n", port, port, val);
#endif
    return val;
}

void tsOutFFEF(Computer* comp, unsigned short port, unsigned char val) { //SPR port
    comp->tsconf.pffef = val;
#ifdef ISDEBUG
    printf("SPR set to 0x%02hhxh",val);
#endif
}

// catch

static xPort tsPortMap[] = {
	// xxaf
	{0xffff,0x00af,2,2,2,tsIn00AF,	tsOut00AF},
	{0xffff,0x01af,2,2,2,NULL,	tsOut01AF},
	{0xffff,0x02af,2,2,2,NULL,	tsOut02AF},
	{0xffff,0x03af,2,2,2,NULL,	tsOut03AF},
	{0xffff,0x04af,2,2,2,NULL,	tsOut04AF},
	{0xffff,0x05af,2,2,2,NULL,	tsOut05AF},
	{0xffff,0x06af,2,2,2,NULL,	tsOut06AF},
	{0xffff,0x07af,2,2,2,NULL,	tsOut07AF},
	{0xffff,0x0faf,2,2,2,NULL,	tsOut0FAF},

	{0xffff,0x10af,2,2,2,NULL,	tsOut10AF},
	{0xffff,0x11af,2,2,2,NULL,	tsOut11AF},
	{0xffff,0x12af,2,2,2,tsIn12AF,	tsOut12AF},
	{0xffff,0x13af,2,2,2,tsIn13AF,	tsOut13AF},

	{0xffff,0x15af,2,2,2,NULL,	tsOut15AF},
	{0xffff,0x16af,2,2,2,NULL,	tsOut16AF},
	{0xffff,0x17af,2,2,2,NULL,	tsOut17AF},
	{0xffff,0x18af,2,2,2,NULL,	tsOut18AF},
	{0xffff,0x19af,2,2,2,NULL,	tsOut19AF},

	{0xffff,0x1aaf,2,2,2,NULL,	tsOut1AAF},
	{0xffff,0x1baf,2,2,2,NULL,	tsOut1BAF},
	{0xffff,0x1caf,2,2,2,NULL,	tsOut1CAF},
	{0xffff,0x1daf,2,2,2,NULL,	tsOut1DAF},
	{0xffff,0x1eaf,2,2,2,NULL,	tsOut1EAF},
	{0xffff,0x1faf,2,2,2,NULL,	tsOut1FAF},

	{0xffff,0x20af,2,2,2,NULL,	tsOut20AF},
	{0xffff,0x21af,2,2,2,NULL,	tsOut21AF},
	{0xffff,0x22af,2,2,2,NULL,	tsOut22AF},
	{0xffff,0x23af,2,2,2,NULL,	tsOut23AF},
	{0xffff,0x24af,2,2,2,NULL,	tsOut24AF},
	// {0xffff,0x25af,2,2,2,NULL,	NULL},		// INTVECT (obsolete)

	{0xffff,0x26af,2,2,2,NULL,	tsOut26AF},
	{0xffff,0x27af,2,2,2,tsIn27AF,	tsOut27AF},

	{0xffff,0x28af,2,2,2,NULL,	tsOut28AF},
	{0xffff,0x29af,2,2,2,NULL,	tsOut29AF},
	{0xffff,0x2aaf,2,2,2,NULL,	tsOut2AAF},
	{0xffff,0x2baf,2,2,2,NULL,	NULL},		// cache config

	{0xffff,0x40af,2,2,2,NULL,	tsOut40AF},
	{0xffff,0x41af,2,2,2,NULL,	tsOut41AF},
	{0xffff,0x42af,2,2,2,NULL,	tsOut42AF},
	{0xffff,0x43af,2,2,2,NULL,	tsOut43AF},
	{0xffff,0x44af,2,2,2,NULL,	tsOut44AF},
	{0xffff,0x45af,2,2,2,NULL,	tsOut45AF},
	{0xffff,0x46af,2,2,2,NULL,	tsOut46AF},
	{0xffff,0x47af,2,2,2,NULL,	tsOut47AF},
	// !dos
	{0x00f7,0x00fe,0,2,2,xInFE,	tsOutFE},	// fe
	{0x00ff,0x0057,0,2,2,tsIn57,	tsOut57},	// 57
	{0x00ff,0x0077,0,2,2,tsIn77,	tsOut77},	// 77
//	{0x00ff,0x00fb,0,2,2,NULL,	tsOutFB},	// fb
	{0x10ff,0xeff7,0,2,2,NULL,	tsOutEFF7},	// eff7
	{0x20ff,0xdff7,0,2,2,NULL,	tsOutDFF7},	// dff7
	{0x40ff,0xbff7,0,2,2,tsInBFF7,	tsOutBFF7},	// bff7
	{0x80ff,0x7ffd,0,2,2,NULL,	tsOut7FFD},	// 7ffd
	{0xc0ff,0xbffd,0,2,2,NULL,	xOutBFFD},	// bffd
	{0xc0ff,0xfffd,0,2,2,xInFFFD,	xOutFFFD},	// fffd
	{0xffff,0xfadf,0,2,2,xInFADF,	NULL},		// fadf
	{0xffff,0xfbdf,0,2,2,xInFBDF,	NULL},		// fbdf
	{0xffff,0xffdf,0,2,2,xInFFDF,	NULL},		// ffdf
	// dos
	{0x009f,0x001f,1,2,2,tsInBDI,	tsOutBDI},	// 1f,3f,5f,7f
	{0x00ff,0x00ff,1,2,2,tsInFF,	tsOutFF},	// ff

    {0xffff,0x00ef,2,2,2,tsIn00EF,	tsOut00EF}, //ERS_DS (ts)
    {0xffff,0xc0ef,2,2,2,tsInDBG,	tsOutDBG},
    {0xffff,0xc1ef,2,2,2,tsInDBG,	tsOutDBG},
    {0xffff,0xc2ef,2,2,2,tsInc2EF,	tsOutDBG},  //ERS_SR (ts)
    {0xffff,0xc3ef,2,2,2,tsInc3EF,	tsOutDBG},  //ERS_ST (ts)
    {0xffff,0xc4ef,2,2,2,tsInDBG,	tsOutDBG},
    {0xffff,0xc5ef,2,2,2,tsInDBG,	tsOutDBG},
    {0xffff,0xc6ef,2,2,2,tsInDBG,	tsOutDBG},
    {0xffff,0xc7ef,2,2,2,tsInDBG,	tsOutDBG},
    {0xffff,0xf8ef,2,2,2,tsInF8EF,	tsOutF8EF}, // DAT/DLL register
    {0xffff,0xf9ef,2,2,2,tsInF9EF,	tsOutF9EF}, // IER/DML register
    {0xffff,0xfaef,2,2,2,tsInFAEF,	tsOutFAEF}, // ISR/FCR register
    {0xffff,0xfbef,2,2,2,tsInFBEF,	tsOutFBEF}, // LCR register
    {0xffff,0xfcef,2,2,2,tsInFCEF,	tsOutFCEF}, // MCR register         w/ dummy
    {0xffff,0xfdef,2,2,2,tsInFDEF,	tsOutDBG},  // LSR register         w/ dummy
    {0xffff,0xfeef,2,2,2,tsInFEEF,	tsOutDBG},  // MSR status register  w/ dummy
    {0xffff,0xffef,2,2,2,tsInFFEF,	tsOutFAEF}, // SPR register

	{0x0000,0x0000,2,2,2,NULL,	NULL},
};

void tslOut(Computer* comp,unsigned short port,unsigned char val,int dos) {
	zx_dev_wr(comp, port, val, dos);
	hwOut(tsPortMap, comp, port, val, dos);
}

unsigned char tslIn(Computer* comp,unsigned short port,int dos) {
	unsigned char res = 0xff;
	if (zx_dev_rd(comp, port, &res, dos)) return res;
	res = hwIn(tsPortMap, comp, port, dos);
	return  res;
}
