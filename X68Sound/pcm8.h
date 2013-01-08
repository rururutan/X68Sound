class Pcm8 {
	int	Scale;		// 
	int Pcm;		// 16bit PCM Data
	int Pcm16Prev;	// 16bit,8bitPCMの1つ前のデータ
	int InpPcm,InpPcm_prev,OutPcm;		// HPF用 16bit PCM Data
	int OutInpPcm,OutInpPcm_prev;		// HPF用
	int	AdpcmRate;	// 187500(15625*12), 125000(10416.66*12), 93750(7812.5*12), 62500(5208.33*12), 46875(3906.25*12), ...
	int	RateCounter;
	int	N1Data;	// ADPCM 1サンプルのデータの保存
	int N1DataFlag;	// 0 or 1

	volatile int	Mode;
	volatile int	Volume;	// x/16
	volatile int	PcmKind;	// 0～4:ADPCM  5:16bitPCM  6:8bitPCM  7:謎

	inline void adpcm2pcm(unsigned char adpcm);
	inline void	pcm16_2pcm(int pcm16);

public:
	inline int	DmaGetByte();
	unsigned char	DmaLastValue;
	unsigned char	AdpcmReg;

	volatile unsigned char *DmaMar;
	volatile unsigned int DmaMtc;
	volatile unsigned char *DmaBar;
	volatile unsigned int DmaBtc;
	volatile int	DmaOcr;				// 0:チェイン動作なし 0x08:アレイチェイン 0x0C:リンクアレイチェイン


	inline int DmaArrayChainSetNextMtcMar();
	inline int DmaLinkArrayChainSetNextMtcMar();

	Pcm8(void);
	~Pcm8() {};
	inline void	Init();
	inline void	InitSamprate();
	inline void Reset();
	inline int	GetPcm();
	inline int	GetPcm62();

	inline int	Out(void *adrs, int mode, int len);
	inline int	Aot(void *tbl, int mode, int cnt);
	inline int	Lot(void *tbl, int mode);
	inline int	SetMode(int mode);
	inline int	GetRest();
	inline int	GetMode();


};


Pcm8::Pcm8(void) {
	Mode = 0x00080403;
	SetMode(Mode);
}



inline void Pcm8::Init() {
	AdpcmReg = 0xC7;	// ADPCM動作停止

	Scale = 0;
	Pcm = 0;
	Pcm16Prev = 0;
	InpPcm = InpPcm_prev = OutPcm = 0;
	OutInpPcm = OutInpPcm_prev = 0;
	AdpcmRate = 15625*12;
	RateCounter = 0;
	N1Data = 0;
	N1DataFlag = 0;
	DmaLastValue = 0;

	DmaMar = NULL;
	DmaMtc = 0;
	DmaBar = NULL;
	DmaBtc = 0;
	DmaOcr = 0;
	
}
inline void Pcm8::InitSamprate() {
	RateCounter = 0;
}
inline void Pcm8::Reset() {		// ADPCM キーオン時の処理
	Scale = 0;
	Pcm = 0;
	Pcm16Prev = 0;
	InpPcm = InpPcm_prev = OutPcm = 0;
	OutInpPcm = OutInpPcm_prev = 0;

	N1Data = 0;
	N1DataFlag = 0;

}



inline int Pcm8::DmaArrayChainSetNextMtcMar() {
	if ( DmaBtc == 0 ) {
		return 1;
	}
	--DmaBtc;

	int	mem0,mem1,mem2,mem3,mem4,mem5;
	mem0 = MemRead((unsigned char *)DmaBar++);
	mem1 = MemRead((unsigned char *)DmaBar++);
	mem2 = MemRead((unsigned char *)DmaBar++);
	mem3 = MemRead((unsigned char *)DmaBar++);
	mem4 = MemRead((unsigned char *)DmaBar++);
	mem5 = MemRead((unsigned char *)DmaBar++);
	if ((mem0|mem1|mem2|mem3|mem4|mem5) == -1) {
		// バスエラー(ベースアドレス/ベースカウンタ)
		return 1;
	} 
	DmaMar = (volatile unsigned char *)((mem0<<24)|(mem1<<16)|(mem2<<8)|(mem3));	// MAR
	DmaMtc = (mem4<<8)|(mem5);	// MTC

	if ( DmaMtc == 0 ) {	// MTC == 0 ?
		// カウントエラー(メモリアドレス/メモリカウンタ)
		return 1;
	}
	return 0;
}
inline int Pcm8::DmaLinkArrayChainSetNextMtcMar() {
	if (DmaBar == (unsigned char *)0) {
		return 1;
	}

	int	mem0,mem1,mem2,mem3,mem4,mem5;
	int mem6,mem7,mem8,mem9;
	mem0 = MemRead((unsigned char *)DmaBar++);
	mem1 = MemRead((unsigned char *)DmaBar++);
	mem2 = MemRead((unsigned char *)DmaBar++);
	mem3 = MemRead((unsigned char *)DmaBar++);
	mem4 = MemRead((unsigned char *)DmaBar++);
	mem5 = MemRead((unsigned char *)DmaBar++);
	mem6 = MemRead((unsigned char *)DmaBar++);
	mem7 = MemRead((unsigned char *)DmaBar++);
	mem8 = MemRead((unsigned char *)DmaBar++);
	mem9 = MemRead((unsigned char *)DmaBar++);
	if ((mem0|mem1|mem2|mem3|mem4|mem5|mem6|mem7|mem8|mem9) == -1) {
		// バスエラー(ベースアドレス/ベースカウンタ)
		return 1;
	}
	DmaMar = (volatile unsigned char *)((mem0<<24)|(mem1<<16)|(mem2<<8)|(mem3));	// MAR
	DmaMtc = (mem4<<8)|(mem5);	// MTC
	DmaBar = (volatile unsigned char *)((mem6<<24)|(mem7<<16)|(mem8<<8)|(mem9));	// BAR

	if ( DmaMtc == 0 ) {	// MTC == 0 ?
		// カウントエラー(メモリアドレス/メモリカウンタ)
		return 1;
	}
	return 0;
}

inline int	Pcm8::DmaGetByte() {
	if (DmaMtc == 0) {
		return 0x80000000;
	}
	{
		int mem;
		mem = MemRead((unsigned char *)DmaMar);
		if (mem == -1) {
			// バスエラー(メモリアドレス/メモリカウンタ)
			return 0x80000000;
		}
		DmaLastValue = mem;
		DmaMar += 1;
	}

	--DmaMtc;

	try {
	if (DmaMtc == 0) {
		if (DmaOcr & 0x08) {	// チェイニング動作
			if (!(DmaOcr & 0x04)) {	// アレイチェイン
				if (DmaArrayChainSetNextMtcMar()) {
					throw "";
				}
			} else {						// リンクアレイチェイン
				if (DmaLinkArrayChainSetNextMtcMar()) {
					throw "";
				}
			}
		}
	}
	} catch (void *) {
	}
	
	return DmaLastValue;
}








#define	MAXPCMVAL	(2047)
static int	HPF_shift_tbl[16+1]={ 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4,};
	

// adpcmを入力して InpPcm の値を変化させる
// -2047<<(4+4) <= InpPcm <= +2047<<(4+4)
inline void	Pcm8::adpcm2pcm(unsigned char adpcm) {

	
	int	dltL;
	dltL = dltLTBL[Scale];
	dltL = (dltL&(adpcm&4?-1:0)) + ((dltL>>1)&(adpcm&2?-1:0)) + ((dltL>>2)&(adpcm&1?-1:0)) + (dltL>>3);
	int sign = adpcm&8?-1:0;
	dltL = (dltL^sign)+(sign&1);
	Pcm += dltL;
	

	if ((unsigned int)(Pcm+MAXPCMVAL) > (unsigned int)(MAXPCMVAL*2)) {
		if ((int)(Pcm+MAXPCMVAL) >= (int)(MAXPCMVAL*2)) {
			Pcm = MAXPCMVAL;
		} else {
			Pcm = -MAXPCMVAL;
		}
	}

	InpPcm = (Pcm&(int)0xFFFFFFFC)<<(4+4);

	Scale += DCT[adpcm];
	if ((unsigned int)Scale > (unsigned int)48) {
		if ((int)Scale >= (int)48) {
			Scale = 48;
		} else {
			Scale = 0;
		}
	}
}

// pcm16を入力して InpPcm の値を変化させる
// -2047<<(4+4) <= InpPcm <= +2047<<(4+4)
inline void	Pcm8::pcm16_2pcm(int pcm16) {
	Pcm += pcm16-Pcm16Prev;
	Pcm16Prev = pcm16;
	

	if ((unsigned int)(Pcm+MAXPCMVAL) > (unsigned int)(MAXPCMVAL*2)) {
		if ((int)(Pcm+MAXPCMVAL) >= (int)(MAXPCMVAL*2)) {
			Pcm = MAXPCMVAL;
		} else {
			Pcm = -MAXPCMVAL;
		}
	}

	InpPcm = (Pcm&(int)0xFFFFFFFC)<<(4+4);
}

// -32768<<4 <= retval <= +32768<<4
inline int Pcm8::GetPcm() {
	if (AdpcmReg & 0x80) {		// ADPCM 停止中
		return 0x80000000;
	}
	RateCounter -= AdpcmRate;
	while (RateCounter < 0) {
		if (PcmKind == 5) {	// 16bitPCM
			int dataH,dataL;
			dataH = DmaGetByte();
			if (dataH == 0x80000000) {
				RateCounter = 0;
				AdpcmReg = 0xC7;	// ADPCM 停止
				return 0x80000000;
			}
			dataL = DmaGetByte();
			if (dataL == 0x80000000) {
				RateCounter = 0;
				AdpcmReg = 0xC7;	// ADPCM 停止
				return 0x80000000;
			}
			pcm16_2pcm((int)(short)((dataH<<8)|dataL));	// OutPcm に値が入る
		} else if (PcmKind == 6) {	// 8bitPCM
			int data;
			data = DmaGetByte();
			if (data == 0x80000000) {
				RateCounter = 0;
				AdpcmReg = 0xC7;	// ADPCM 停止
				return 0x80000000;
			}
			pcm16_2pcm((int)(char)data);	// InpPcm に値が入る
		} else {
			if (N1DataFlag == 0) {		// 次のADPCMデータが内部にない場合
				int	N10Data;	// (N1Data << 4) | N0Data
				N10Data = DmaGetByte();	// DMA転送(1バイト)
				if (N10Data == 0x80000000) {
					RateCounter = 0;
					AdpcmReg = 0xC7;	// ADPCM 停止
					return 0x80000000;
				}
				adpcm2pcm(N10Data & 0x0F);	// InpPcm に値が入る
				N1Data = (N10Data >> 4) & 0x0F;
				N1DataFlag = 1;
			} else {
				adpcm2pcm(N1Data);			// InpPcm に値が入る
				N1DataFlag = 0;
			}
		}
		RateCounter += 15625*12;
	}
	OutPcm = ((InpPcm<<9) - (InpPcm_prev<<9) + 459*OutPcm) >> 9;
	InpPcm_prev = InpPcm;

	return (((OutPcm*Volume)>>4)*TotalVolume)>>8;
}

// -32768<<4 <= retval <= +32768<<4
inline int Pcm8::GetPcm62() {
	if (AdpcmReg & 0x80) {		// ADPCM 停止中
		return 0x80000000;
	}
	RateCounter -= AdpcmRate;
	while (RateCounter < 0) {
		if (PcmKind == 5) {	// 16bitPCM
			int dataH,dataL;
			dataH = DmaGetByte();
			if (dataH == 0x80000000) {
				RateCounter = 0;
				AdpcmReg = 0xC7;	// ADPCM 停止
				return 0x80000000;
			}
			dataL = DmaGetByte();
			if (dataL == 0x80000000) {
				RateCounter = 0;
				AdpcmReg = 0xC7;	// ADPCM 停止
				return 0x80000000;
			}
			pcm16_2pcm((int)(short)((dataH<<8)|dataL));	// OutPcm に値が入る
		} else if (PcmKind == 6) {	// 8bitPCM
			int data;
			data = DmaGetByte();
			if (data == 0x80000000) {
				RateCounter = 0;
				AdpcmReg = 0xC7;	// ADPCM 停止
				return 0x80000000;
			}
			pcm16_2pcm((int)(char)data);	// InpPcm に値が入る
		} else {
			if (N1DataFlag == 0) {		// 次のADPCMデータが内部にない場合
				int	N10Data;	// (N1Data << 4) | N0Data
				N10Data = DmaGetByte();	// DMA転送(1バイト)
				if (N10Data == 0x80000000) {
					RateCounter = 0;
					AdpcmReg = 0xC7;	// ADPCM 停止
					return 0x80000000;
				}
				adpcm2pcm(N10Data & 0x0F);	// InpPcm に値が入る
				N1Data = (N10Data >> 4) & 0x0F;
				N1DataFlag = 1;
			} else {
				adpcm2pcm(N1Data);			// InpPcm に値が入る
				N1DataFlag = 0;
			}
		}
		RateCounter += 15625*12*4;
	}
	OutInpPcm = (InpPcm<<9) - (InpPcm_prev<<9) +  OutInpPcm-(OutInpPcm>>5)-(OutInpPcm>>10);
	InpPcm_prev = InpPcm;
	OutPcm = OutInpPcm - OutInpPcm_prev + OutPcm-(OutPcm>>8)-(OutPcm>>9)-(OutPcm>>12);
	OutInpPcm_prev = OutInpPcm;
	return ((OutPcm>>9)*Volume)>>4;
}







inline int	Pcm8::Out(void *adrs, int mode, int len) {
	if (len <= 0) {
		if (len < 0) {
			return GetRest();
		} else {
			DmaMtc = 0;
			return 0;
		}
	}
	AdpcmReg = 0xC7;	// ADPCM 停止
	DmaMtc = 0;
	DmaMar = (unsigned char *)adrs;
	SetMode(mode);
	if ((mode&3) != 0) {
		DmaMtc = len;
		Reset();
		AdpcmReg = 0x47;	// ADPCM 動作開始
	}
	return 0;
}
inline int	Pcm8::Aot(void *tbl, int mode, int cnt) {
	if (cnt <= 0) {
		if (cnt < 0) {
			return GetRest();
		} else {
			DmaMtc = 0;
			return 0;
		}
	}
	AdpcmReg = 0xC7;	// ADPCM 停止
	DmaMtc = 0;
	DmaBar = (unsigned char *)tbl;
	DmaBtc = cnt;
	SetMode(mode);
	if ((mode&3) != 0) {
		DmaArrayChainSetNextMtcMar();
		Reset();
		AdpcmReg = 0x47;	// ADPCM 動作開始
	}
	return 0;
}
inline int	Pcm8::Lot(void *tbl, int mode) {
	AdpcmReg = 0xC7;	// ADPCM 停止
	DmaMtc = 0;
	DmaBar = (unsigned char *)tbl;
	SetMode(mode);
	if ((mode&3) != 0) {
		DmaLinkArrayChainSetNextMtcMar();
		Reset();
		AdpcmReg = 0x47;	// ADPCM 動作開始
	}
	return 0;
}

inline int	Pcm8::SetMode(int mode) {
	int	m;
	m = (mode>>16) & 0xFF;
	if (m != 0xFF) {
		m &= 15;
		Volume = PCM8VOLTBL[m];
		Mode = (Mode&0xFF00FFFF)|(m<<16);
	}
	m = (mode>>8) & 0xFF;
	if (m != 0xFF) {
		m &= 7;
		AdpcmRate = ADPCMRATEADDTBL[m];
		PcmKind = m;
		Mode = (Mode&0xFFFF00FF)|(m<<8);
	}
	m = (mode) & 0xFF;
	if (m != 0xFF) {
		m &= 3;
		if (m == 0) {
			AdpcmReg = 0xC7;	// ADPCM 停止
			DmaMtc = 0;
		} else {
			Mode = (Mode&0xFFFFFF00)|(m);
		}
	}
	return 0;
}

inline int	Pcm8::GetRest() {
	if (DmaMtc == 0) {
		return 0;
	}
	if (DmaOcr & 0x08) {	// チェイニング動作
		if (!(DmaOcr & 0x04)) {	// アレイチェイン
			return -1;
		} else {						// リンクアレイチェイン
			return -2;
		}
	}
	return DmaMtc;
}
inline int	Pcm8::GetMode() {
	return Mode;
}


