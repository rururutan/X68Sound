// JULIET.CPP
// Programmed by ROMEOユーザー数名 / and GORRY.

#include	<windows.h>
#include	"romeo.h"
#include	"juliet.h"


#ifndef SUCCESS
#define	SUCCESS		0
#endif

#ifndef FAILURE
#define	FAILURE		(!SUCCESS)
#endif

#ifndef LABEL
#define	LABEL		__declspec(naked)
#endif


enum {
	ROMEO_AVAIL			= 0x01,
	ROMEO_YMF288		= 0x02,			// 必ず存在する筈？
	ROMEO_YM2151		= 0x04
};

typedef struct {
	HMODULE			mod;

	PCIFINDDEV		finddev;
	PCICFGREAD32	read32;
	PCIMEMWR8		out8;
	PCIMEMWR16		out16;
	PCIMEMWR32		out32;
	PCIMEMRD8		in8;
	PCIMEMRD16		in16;
	PCIMEMRD32		in32;

	ULONG			addr;
	ULONG			irq;
	ULONG			avail;
	ULONG			snoopcount;

	BYTE			YM2151_outop[8];
	BYTE			YM2151_ttl[8*4];

	BYTE			YMF288_outop[8];
	BYTE			YMF288_ttl[8*4];
	BYTE			YMF288_PSG;
} _ROMEO;


#define	ROMEO_TPTR(member)	(int)&(((_ROMEO *)NULL)->member)


typedef struct {
	char	*symbol;
	int		addr;
} DLLPROCESS;


static const DLLPROCESS	dllproc[] = {
				{FN_PCIFINDDEV,		ROMEO_TPTR(finddev)},
				{FN_PCICFGREAD32,	ROMEO_TPTR(read32)},
				{FN_PCIMEMWR8,		ROMEO_TPTR(out8)},
				{FN_PCIMEMWR16,		ROMEO_TPTR(out16)},
				{FN_PCIMEMWR32,		ROMEO_TPTR(out32)},
				{FN_PCIMEMRD8,		ROMEO_TPTR(in8)},
				{FN_PCIMEMRD16,		ROMEO_TPTR(in16)},
				{FN_PCIMEMRD32,		ROMEO_TPTR(in32)}};


static	_ROMEO		romeo = {NULL};

static const BYTE FMoutop[] = {0x08, 0x08, 0x08, 0x08, 0x0c, 0x0e, 0x0e, 0x0f};
static	char	YM2151vol = 0;


BOOL juliet_load(void) {

		int			i;
const	DLLPROCESS	*dp;
		BOOL		r = SUCCESS;

	juliet_unload();

	romeo.mod = LoadLibrary(PCIDEBUG_DLL);
	if (romeo.mod == NULL) {
		return(FAILURE);
	}
	for (i=0, dp=dllproc; i<sizeof(dllproc)/sizeof(DLLPROCESS); i++, dp++) {
		FARPROC proc;
		proc = GetProcAddress(romeo.mod, dp->symbol);
		if (proc == NULL) {
			r = FAILURE;
			break;
		}
		*(DWORD *)(((BYTE *)&romeo) + (dp->addr)) = (DWORD)proc;
	}
	if (r) {
		juliet_unload();
	}
	return(r);
}


void juliet_unload(void) {

	if (romeo.mod) {
		FreeLibrary(romeo.mod);
	}
	ZeroMemory(&romeo, sizeof(romeo));
	FillMemory(romeo.YM2151_ttl, 8*4, 0x7f);
	FillMemory(romeo.YMF288_ttl, 8*4, 0x7f);
	romeo.YMF288_PSG = 0x3f;
}


// ----

// pciFindPciDevice使うと、OS起動後一発目に見つけられないことが多いので、自前で検索する
static ULONG searchRomeo(void)
{
	int bus, dev, func;
	for (bus=0; bus<256; bus++) {
		for (dev=0; dev<32; dev++) {
			for (func=0; func<8; func++) {
				ULONG addr = pciBusDevFunc(bus, dev, func);
				ULONG dev_vend = romeo.read32(addr, 0x0000);
				if ( (dev_vend&0xffff)!=ROMEO_VENDORID ) continue;
				dev_vend >>= 16;
				if ( (dev_vend==ROMEO_DEVICEID)||(dev_vend==ROMEO_DEVICEID2) ) return addr;
			}
		}
	}
	return ((ULONG)0xffffffff);
}


BOOL juliet_prepare(void) {

	ULONG	pciaddr;

	if (romeo.mod == NULL) {
		return(FAILURE);
	}

	pciaddr = searchRomeo();
	if ( pciaddr!=(ULONG)0xffffffff ) {
		romeo.addr = romeo.read32(pciaddr, ROMEO_BASEADDRESS1);
		romeo.irq  = romeo.read32(pciaddr, ROMEO_PCIINTERRUPT) & 0xff;
		if (romeo.addr) {
			romeo.avail = ROMEO_AVAIL | ROMEO_YMF288;
			juliet_YM2151Reset();
			juliet_YMF288Reset();
		}
		return (SUCCESS);
	}
	return (FAILURE);
}


// ---- YM2151部
// リセットと同時に、OPMチップの有無も確認
void juliet_YM2151Reset(void)
{
	BYTE flag;

	if (romeo.avail & ROMEO_AVAIL) {
		juliet_YM2151Mute(TRUE);
		romeo.out32(romeo.addr + ROMEO_YM2151CTRL, 0x00);
		Sleep(10);					// 44.1kHz x 192 clk = 4.35ms 以上ないと、DACのリセットかからない
		flag = romeo.in8(romeo.addr + ROMEO_YM2151DATA) + 1;
		romeo.out32(romeo.addr + ROMEO_YM2151CTRL, 0x80);
		Sleep(10);					// リセット解除後、一応安定するまでちょっと待つ
		flag |= romeo.in8(romeo.addr + ROMEO_YM2151DATA);
		if ( !flag ) {					// flag!=0 だと OPM チップがない
			romeo.avail |= ROMEO_YM2151;
			// Busy検出用にSnoopカウンタを使う
			romeo.out32(romeo.addr + ROMEO_SNOOPCTRL, (unsigned int)0x80000000);
			romeo.snoopcount = 0xffffffff;
		}
	}
}

int juliet_YM2151IsEnable(void)
{
	return (( romeo.avail&ROMEO_YM2151 )?TRUE:FALSE);
}

int juliet_YM2151IsBusy(void)
{
	int ret = FALSE;
	if ( romeo.avail&ROMEO_YM2151 ) {
		if ( (romeo.snoopcount==romeo.in32(romeo.addr + ROMEO_SNOOPCTRL)) ||
		     (romeo.in8(romeo.addr + ROMEO_YM2151DATA)&0x80 ) ) ret = TRUE;
	}
	return ret;
}

static void YM2151W(BYTE addr, BYTE data) {

	// 書き込み直後だと、ROMEOチップでの遅延のため、まだ書き込みが起こっていない（＝Busyが
	// 立っていない）可能性がある。ので、Snoopカウンタで書き込み発生を見張る
	while ( romeo.snoopcount==romeo.in32(romeo.addr + ROMEO_SNOOPCTRL) ) Sleep(0);
	romeo.snoopcount = romeo.in32(romeo.addr + ROMEO_SNOOPCTRL);

	// カウンタ増えた時点ではまだBusyの可能性があるので、OPMのBusyも見張る
	while ( romeo.in8(romeo.addr + ROMEO_YM2151DATA)&0x80 ) Sleep(0);

	romeo.out8(romeo.addr + ROMEO_YM2151ADDR, addr);
	romeo.out8(romeo.addr + ROMEO_YM2151DATA, data);
}

static void YM2151volset(BYTE ch, BYTE mask, char vol) {

	BYTE	data;
	BYTE	out;

	ch &= 7;
	out = romeo.YM2151_outop[ch];
	ch += 0x60;
	do {
		if (mask & 1) {
			data = romeo.YM2151_ttl[ch & 0x1f];
			if (out & 1) {
				data -= vol;
				if (data & 0x80) {
					data = ((vol < 0)?0x7f:0);
				}
			}
			YM2151W(ch, data);
		}
		ch += 0x08;
		out >>= 1;
		mask >>= 1;
	} while(mask);
}

void juliet_YM2151Mute(BOOL mute) {

	BYTE	ch;
	char	vol;

	if (romeo.avail & ROMEO_YM2151) {
		vol = (mute?-127:YM2151vol);
		for (ch=0; ch<8; ch++) {
			YM2151volset(ch, romeo.YM2151_outop[ch & 7], vol);
		}
	}
}

void juliet_YM2151W(BYTE addr, BYTE data) {

	if (romeo.avail & ROMEO_YM2151) {
		if ((addr & 0xe0) == 0x60) {				// ttl
			data &= 0x7f;
			romeo.YM2151_ttl[addr & 0x1f] = data;
			if (romeo.YM2151_outop[addr & 7] & (1 << ((addr >> 3) & 3))) {
				data -= YM2151vol;
				if (data & 0x80) {
					data = ((YM2151vol < 0)?0x7f:0);
				}
			}
		}
		YM2151W(addr, data);
		if ((addr & 0xf8) == 0x20) {				// algorithm
			BYTE op;
			op = romeo.YM2151_outop[addr & 7] ^ FMoutop[data & 7];
			if (op) {
				romeo.YM2151_outop[addr & 7] = FMoutop[data & 7];
				YM2151volset(addr & 7, op, YM2151vol);
			}
		}
	}
}

// ---- YMF288部

void juliet_YMF288Reset(void) {

	if (romeo.avail & ROMEO_YMF288) {
		juliet_YMF288Mute(TRUE);
		romeo.out32(romeo.addr + ROMEO_YMF288CTRL, 0x00);
		Sleep(100);
		romeo.out32(romeo.addr + ROMEO_YMF288CTRL, 0x80);
		Sleep(100);
	}
}

int juliet_YMF288IsEnable(void) {

	return(TRUE);
}

int juliet_YMF288IsBusy(void) {

	return((!(romeo.avail&ROMEO_YMF288)) ||
			((romeo.in8(romeo.addr + ROMEO_YMF288ADDR1) & 0x80) != 0));
}


static void YMF288W(BYTE a1, BYTE addr, BYTE data) {

	while(romeo.in8(romeo.addr + ROMEO_YMF288ADDR1) & 0x80) {
		Sleep(0);
	}
	romeo.out8(romeo.addr + (a1?ROMEO_YMF288ADDR2:ROMEO_YMF288ADDR1), addr);
	while(romeo.in8(romeo.addr + ROMEO_YMF288ADDR1) & 0x80) {
		Sleep(0);
	}
	romeo.out8(romeo.addr + (a1?ROMEO_YMF288DATA2:ROMEO_YMF288DATA1), data);
}


static void YMF288volset(BYTE ch, BYTE mask, char vol) {

	BYTE	data;
	BYTE	out;
	BYTE	a1;
	BYTE	*datp;

	a1 = ch & 4;
	out = romeo.YMF288_outop[ch & 7];
	ch &= 3;
	datp = romeo.YMF288_ttl + (a1 << 2);
	ch += 0x40;
	do {
		if (mask & 1) {
			data = datp[ch & 0x0f];
			if (out & 1) {
				data -= vol;
				if (data & 0x80) {
					data = ((vol < 0)?0x7f:0);
				}
			}
			YMF288W(a1, ch, data);
		}
		ch += 0x04;
		out >>= 1;
		mask >>= 1;
	} while(mask);
}


void juliet_YMF288Mute(BOOL mute) {

	BYTE	ch;
	char	vol;

	if (romeo.avail & ROMEO_YMF288) {
		YMF288W(0, 0x07, mute?0x3f:romeo.YMF288_PSG);

		vol = (mute?-127:0);
		for (ch=0; ch<3; ch++) {
			YMF288volset(ch+0, romeo.YMF288_outop[ch+0], vol);
			YMF288volset(ch+4, romeo.YMF288_outop[ch+4], vol);
		}
	}
}

void juliet_YMF288A(BYTE addr, BYTE data) {

	if (romeo.avail & ROMEO_YMF288) {
		if (addr == 0x07) {							// psg mix
			romeo.YMF288_PSG = data;
		}
		else if ((addr & 0xf0) == 0x40) {			// ttl
			romeo.YMF288_ttl[addr & 0x0f] = data & 0x7f;
		}
		else if ((addr & 0xfc) == 0xb0) {			// algorithm
			romeo.YMF288_outop[addr & 3] = FMoutop[data & 7];
		}
		YMF288W(0, addr, data);
	}
}

void juliet_YMF288B(BYTE addr, BYTE data) {

	if (romeo.avail & ROMEO_YMF288) {
		if ((addr & 0xf0) == 0x40) {				// ttl
			romeo.YMF288_ttl[0x10 + addr & 0x0f] = data & 0x7f;
		}
		else if ((addr & 0xfc) == 0xb0) {			// algorithm
			romeo.YMF288_outop[4 + addr & 3] = FMoutop[data & 7];
		}
		YMF288W(1, addr, data);
	}
}


// ---- delay...

#define	FM_BUFBIT		10
#define	FM_BUFFERS		(1 << FM_BUFBIT)

typedef struct {
	DWORD	clock;
	DWORD	data;
} FMRINGDATA;

typedef struct {
	DWORD		base;
	DWORD		pos;
	DWORD		avail;
	DWORD		maxclock;
	FMRINGDATA	datas[FM_BUFFERS];
} FMRING;

static	FMRING	fmr;
		DWORD	basedclk = 0;

static void (*sendfmfn[3])(BYTE addr, BYTE data) =
							{juliet_YM2151W, juliet_YMF288A, juliet_YMF288B};


LABEL static void fmr_release(void) {

	__asm {
				dec		fmr.avail
				mov		edx, fmr.pos
				mov		eax, fmr.datas.data[edx * 8]
				inc		edx
				and		edx, (FM_BUFFERS - 1);
				mov		fmr.pos, edx
				push	eax
				shr		eax, 8
				push	eax
				shr		eax, 8
				call	sendfmfn[eax*4]
				add		esp, 8
				ret
	}
}


LABEL static void fmr_send(DWORD clock, DWORD data) {

	__asm {
				mov		ecx, fmr.avail
				cmp		ecx, FM_BUFFERS
				jne		short fmdatasend
				call	fmr_release
				dec		ecx
fmdatasend:		add		ecx, fmr.pos
				and		ecx, (FM_BUFFERS - 1);
				mov		eax, fmr.base
				add		eax, [esp + 4]
				mov		fmr.datas.clock[ecx * 8], eax
				mov		eax, [esp + 8]
				mov		fmr.datas.data[ecx * 8], eax
				inc		fmr.avail
				ret
	}
}


#define	_CPUID	__asm _emit 00fh __asm _emit 0a2h
#define	_RDTSC	__asm _emit 00fh __asm _emit 031h
#define	RDTSC_SFT	6

LABEL void juliet2_reset(void) {

	__asm {
				push	ebx
				push	esi
				push	edi
				mov		ecx, (type fmr) / 4
				xor		eax, eax
				mov		edi, offset fmr
				rep stosd
				push	100
				mov		edi, dword ptr Sleep
				_CPUID
				_RDTSC
				mov		esi, edx
				xchg	edi, eax
				call	eax
				_CPUID
				_RDTSC
				sub		eax, edi
				sbb		edx, esi
				shrd	eax, edx, RDTSC_SFT
				mov		fmr.maxclock, eax
				mov		ebx, 6
				xor		edx, edx
				div		ebx
				mov		basedclk, eax
				pop		edi
				pop		esi
				pop		ebx
				ret
	}
}


LABEL void juliet2_sync(DWORD delaytick) {

	__asm {
				push	ebx
				_CPUID
				_RDTSC
				shrd	eax, edx, RDTSC_SFT
				add		eax, basedclk
				add		eax, basedclk
				mov		fmr.base, eax
				pop		ebx
				ret
	}
}


LABEL void juliet2_exec(void) {

	__asm {
				cmp		fmr.avail, 0
				jne		short jul2exe
jul2exe_exit:	ret
jul2exe:		push	ebx
				_CPUID
				_RDTSC
				pop		ebx
				shrd	eax, edx, RDTSC_SFT
				neg		eax
				mov		edx, fmr.pos
				add		eax, fmr.datas[edx * 8].clock
				cmp		eax, fmr.maxclock
				jb		jul2exe_exit
				call	fmr_release
				cmp		fmr.avail, 0
				jne		short jul2exe
				ret
	}
}


void juliet2_YM2151W(BYTE addr, BYTE data, DWORD clock) {

	if (romeo.avail & ROMEO_YM2151) {
		fmr_send(clock, (addr << 8) | data);
	}
}

void juliet2_YMF288A(BYTE addr, BYTE data, DWORD clock) {

	if (romeo.avail & ROMEO_YMF288) {
		fmr_send(clock, 0x10000 | (addr << 8) | data);
	}
}
