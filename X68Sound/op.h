#define	KEYON	-1
#define	ATACK	0
#define	DECAY	1
#define	SUSTAIN	2
#define	SUSTAIN_MAX	3
#define	RELEASE	4
#define	RELEASE_MAX	5

#define CULC_DELTA_T	(0x7FFFFFFF)
#define	CULC_ALPHA		(0x7FFFFFFF)

#define	KC_KF_DT2_MAX	6912	//((9*12)<<6)

const int NEXTSTAT[RELEASE_MAX+1]={
	DECAY, SUSTAIN, SUSTAIN_MAX, SUSTAIN_MAX, RELEASE_MAX, RELEASE_MAX,
};
const int MAXSTAT[RELEASE_MAX+1]={
	ATACK, SUSTAIN_MAX, SUSTAIN_MAX, SUSTAIN_MAX, RELEASE_MAX, RELEASE_MAX,
};

class Op {
public:
	volatile int	inp;			// FM�ϒ��̓���
private:
	int LfoPitch;	// �O���lfopitch�l, CULC_DELTA_T�l�̎���DeltaT���Čv�Z����B
	int	T;		// ���ݎ��� (0 <= T < SIZESINTBL*PRECISION)
	int DeltaT;	// ��t
	int	Ame;		// 0(�g�������������Ȃ�), -1(�g��������������)
	int	LfoLevel;	// �O���lfopitch&Ame�l, CULC_ALPHA�l�̎���Alpha���Čv�Z����B
	int	Alpha;	// �ŏI�I�ȃG���x���[�v�o�͒l
public:
	volatile int	*out;			// �I�y���[�^�̏o�͐�
	volatile int	*out2;			// �I�y���[�^�̏o�͐�(alg=5����M1�p)
	volatile int	*out3;			// �I�y���[�^�̏o�͐�(alg=5����M1�p)
private:
	int	Pitch;	// 0<=pitch<10*12*64
	int	Dt1Pitch;	// Step �ɑ΂���␳��
	int	Mul;	// 0.5*2 1*2 2*2 3*2 ... 15*2
	int	Tl;		// (128-TL)*8

	int	Out2Fb;	// �t�B�[�h�o�b�N�ւ̏o�͒l
	int	Inp_last;	// �Ō�̓��͒l
	int	Fl;		// �t�B�[�h�o�b�N���x���̃V�t�g�l(31,7,6,5,4,3,2,1)
	int	Fl_mask;	// �t�B�[�h�o�b�N�̃}�X�N(0,-1)
	int	ArTime;	// AR��p t

	int	NoiseCounter;	// Noise�p�J�E���^
	int NoiseStep;	// Noise�p�J�E���g�_�E���l
	int NoiseCycle;	// Noise���� 32*2^25(0) �` 1*2^25(31) NoiseCycle==0�̎��̓m�C�Y�I�t
	int NoiseValue;	// �m�C�Y�l  1 or -1

	// �G���x���[�v�֌W
	int	Xr_stat;
	int	Xr_el;
	int	Xr_step;
	int	Xr_and;
	int	Xr_cmp;
	int	Xr_add;
	int Xr_limit;
	
	
	int	Note;	// ���K (0 <= Note < 10*12)
	int	Kc;		// ���K (1 <= Kc <= 128)
	int	Kf;		// ������ (0 <= Kf < 64)
	int Ar;		// 0 <= Ar < 31
	int D1r;	// 0 <= D1r < 31
	int	D2r;	// 0 <= D2r < 31
	int	Rr;		// 0 <= Rr < 15
	int	Ks;		// 0 <= Ks <= 3
	int	Dt2;	// Pitch �ɑ΂���␳��(0, 384, 500, 608)
	int	Dt1;	// DT1�̒l(0�`7)
	int Nfrq;	// Noiseflag,NFRQ�̒l

	struct {int and,cmp,add, limit;}
		StatTbl[RELEASE_MAX+1];	// ��Ԑ��ڃe�[�u��
	//           ATACK     DECAY   SUSTAIN     SUSTAIN_MAX RELEASE     RELEASE_MAX
	// and     :                               4097                    4097
	// cmp     :                               2048                    2048
	// add     :                               0                       0
	// limit   : 0         D1l     63          63          63          63
	// nextstat: DECAY     SUSTAIN SUSTAIN_MAX SUSTAIN_MAX RELEASE_MAX RELEASE_MAX

	int keyon;
	int csmkeyon;

	inline void	CulcArStep();
	inline void	CulcD1rStep();
	inline void	CulcD2rStep();
	inline void	CulcRrStep();
	inline void	CulcPitch();
	inline void	CulcDt1Pitch();
	inline void CulcNoiseCycle();

public:
	Op(void);
	~Op() {};
	inline void Init();
	inline void InitSamprate();
	inline void SetFL(int n);
	inline void SetKC(int n);
	inline void SetKF(int n);
	inline void SetDT1MUL(int n);
	inline void SetTL(int n);
	inline void SetKSAR(int n);
	inline void SetAMED1R(int n);
	inline void SetDT2D2R(int n);
	inline void SetD1LRR(int n);
	inline void KeyON(int csm);
	inline void KeyOFF(int csm);
	inline void Envelope(int env_counter);
	inline void SetNFRQ(int nfrq);

	inline void Output0(int lfopitch, int lfolevel);		// �I�y���[�^0�p
	inline void Output(int lfopitch, int lfolevel);		// ��ʃI�y���[�^�p
	inline void Output32(int lfopitch, int lfolevel);		// �X���b�g32�p
	inline void Output0_22(int lfopitch, int lfolevel);		// �I�y���[�^0�p
	inline void Output_22(int lfopitch, int lfolevel);		// ��ʃI�y���[�^�p
	inline void Output32_22(int lfopitch, int lfolevel);		// �X���b�g32�p
};






Op::Op(void) {
};

inline void Op::Init() {
	Note = 5*12+8;
	Kc = 5*16+8 + 1;
	Kf = 5;
	Ar = 10;
	D1r = 10;
	D2r = 5;
	Rr = 12;
	Ks = 1;
	Dt2 = 0;
	Dt1 = 0;

	ArTime = 0;
	Fl = 31;
	Fl_mask = 0;
	Out2Fb = 0;
	inp = 0;
	Inp_last = 0;
	DeltaT = 0;
	LfoPitch = CULC_DELTA_T;
	T = 0;
	LfoLevel = CULC_ALPHA;
	Alpha = 0;
	Tl = (128-127)<<3;
	Xr_el = 1024;
	Xr_step = 0;
	Mul = 2;
	Ame = 0;

//	NoiseStep = (int64_t)(1<<26)*(int64_t)62500/Samprate;
	NoiseStep = (int64_t)(1<<26)*(int64_t)OpmRate/Samprate;
	SetNFRQ(0);
	NoiseValue = 1;

	// ��Ԑ��ڃe�[�u�����쐬
//	StatTbl[ATACK].nextstat = DECAY;
//	StatTbl[DECAY].nextstat = SUSTAIN;
//	StatTbl[SUSTAIN].nextstat = SUSTAIN_MAX;
//	StatTbl[SUSTAIN_MAX].nextstat = SUSTAIN_MAX;
//	StatTbl[RELEASE].nextstat = RELEASE_MAX;
//	StatTbl[RELEASE_MAX].nextstat = RELEASE_MAX;

	StatTbl[ATACK].limit = 0;
	StatTbl[DECAY].limit = D1LTBL[0];
	StatTbl[SUSTAIN].limit = 63;
	StatTbl[SUSTAIN_MAX].limit = 63;
	StatTbl[RELEASE].limit = 63;
	StatTbl[RELEASE_MAX].limit = 63;

	StatTbl[SUSTAIN_MAX].and = 4097;
	StatTbl[SUSTAIN_MAX].cmp = 2048;
	StatTbl[SUSTAIN_MAX].add = 0;
	StatTbl[RELEASE_MAX].and = 4097;
	StatTbl[RELEASE_MAX].cmp = 2048;
	StatTbl[RELEASE_MAX].add = 0;

	Xr_stat = RELEASE_MAX;
	Xr_and = StatTbl[Xr_stat].and;
	Xr_cmp = StatTbl[Xr_stat].cmp;
	Xr_add = StatTbl[Xr_stat].add;
	Xr_limit = StatTbl[Xr_stat].limit;

	keyon = 0;
	csmkeyon = 0;

	CulcArStep();
	CulcD1rStep();
	CulcD2rStep();
	CulcRrStep();
	CulcPitch();
	CulcDt1Pitch();
}

inline void Op::InitSamprate() {
	LfoPitch = CULC_DELTA_T;

//	NoiseStep = (int64_t)(1<<26)*(int64_t)62500/Samprate;
	NoiseStep = (int64_t)(1<<26)*(int64_t)OpmRate/Samprate;
	CulcNoiseCycle();

	CulcArStep();
	CulcD1rStep();
	CulcD2rStep();
	CulcRrStep();
	CulcPitch();
	CulcDt1Pitch();
}

inline void	Op::CulcArStep() {
	if (Ar != 0) {
		int ks = (Ar<<1)+(Kc>>(5-Ks));
			StatTbl[ATACK].and = XRTBL[ks].and;
			StatTbl[ATACK].cmp = XRTBL[ks].and>>1;
		if (ks < 62) {
			StatTbl[ATACK].add = XRTBL[ks].add;
		} else {
			StatTbl[ATACK].add = 128;
		}
	} else {
		StatTbl[ATACK].and = 4097;
		StatTbl[ATACK].cmp = 2048;
		StatTbl[ATACK].add = 0;
	}
	if (Xr_stat == ATACK) {
		Xr_and = StatTbl[Xr_stat].and;
		Xr_cmp = StatTbl[Xr_stat].cmp;
		Xr_add = StatTbl[Xr_stat].add;
	}
};
inline void	Op::CulcD1rStep() {
	if (D1r != 0) {
		int ks = (D1r<<1)+(Kc>>(5-Ks));
		StatTbl[DECAY].and = XRTBL[ks].and;
		StatTbl[DECAY].cmp = XRTBL[ks].and>>1;
		StatTbl[DECAY].add = XRTBL[ks].add;
	} else {
		StatTbl[DECAY].and = 4097;
		StatTbl[DECAY].cmp = 2048;
		StatTbl[DECAY].add = 0;
	}
	if (Xr_stat == DECAY) {
		Xr_and = StatTbl[Xr_stat].and;
		Xr_cmp = StatTbl[Xr_stat].cmp;
		Xr_add = StatTbl[Xr_stat].add;
	}
};
inline void	Op::CulcD2rStep() {
	if (D2r != 0) {
		int ks = (D2r<<1)+(Kc>>(5-Ks));
		StatTbl[SUSTAIN].and = XRTBL[ks].and;
		StatTbl[SUSTAIN].cmp = XRTBL[ks].and>>1;
		StatTbl[SUSTAIN].add = XRTBL[ks].add;
	} else {
		StatTbl[SUSTAIN].and = 4097;
		StatTbl[SUSTAIN].cmp = 2048;
		StatTbl[SUSTAIN].add = 0;
	}
	if (Xr_stat == SUSTAIN) {
		Xr_and = StatTbl[Xr_stat].and;
		Xr_cmp = StatTbl[Xr_stat].cmp;
		Xr_add = StatTbl[Xr_stat].add;
	}
};
inline void	Op::CulcRrStep() {
	int ks = (Rr<<2)+2+(Kc>>(5-Ks));
	StatTbl[RELEASE].and = XRTBL[ks].and;
	StatTbl[RELEASE].cmp = XRTBL[ks].and>>1;
	StatTbl[RELEASE].add = XRTBL[ks].add;
	if (Xr_stat == RELEASE) {
		Xr_and = StatTbl[Xr_stat].and;
		Xr_cmp = StatTbl[Xr_stat].cmp;
		Xr_add = StatTbl[Xr_stat].add;
	}
};
inline void Op::CulcPitch() {
	Pitch = (Note<<6)+Kf+Dt2;
	if (Pitch>KC_KF_DT2_MAX){
		Pitch=KC_KF_DT2_MAX;
	}
}
inline void Op::CulcDt1Pitch() {
	Dt1Pitch = DT1TBL[(Kc&0xFC)+(Dt1&3)];
	if (Dt1&0x04) {
		Dt1Pitch = -Dt1Pitch;
	}
}

inline void Op::SetFL(int n) {
	n = (n>>3) & 7;
	if (n == 0) {
		Fl = 31;
		Fl_mask = 0;
	} else {
		Fl = (7-n+1+1);
		Fl_mask = -1;
	}
};

inline void Op::SetKC(int n) {
	Kc = n & 127;
	int note = Kc & 15;
	Note = ((Kc>>4)+1)*12+ note-(note>>2);
	++Kc;
	CulcPitch();
	CulcDt1Pitch();
	LfoPitch = CULC_DELTA_T;
	CulcArStep();
	CulcD1rStep();
	CulcD2rStep();
	CulcRrStep();
};

inline void Op::SetKF(int n) {
	Kf = (n&255)>>2;
	CulcPitch();
	LfoPitch = CULC_DELTA_T;
};

inline void Op::SetDT1MUL(int n) {
	Dt1 = (n>>4)&7;
	CulcDt1Pitch();
	Mul = (n&15)<<1;
	if (Mul == 0) {
		Mul = 1;
	}
	LfoPitch = CULC_DELTA_T;
};

inline void Op::SetTL(int n) {
	Tl = (128-(n&127))<<3;
	LfoLevel = CULC_ALPHA;
};

inline void Op::SetKSAR(int n) {
	Ks = (n&255)>>6;
	Ar = n & 31;
	CulcArStep();
	CulcD1rStep();
	CulcD2rStep();
	CulcRrStep();
};

inline void Op::SetAMED1R(int n) {
	D1r = n & 31;
	CulcD1rStep();
	Ame = 0;
	if (n & 0x80) {
		Ame = -1;
	}
};

inline void Op::SetDT2D2R(int n) {
	Dt2 = DT2TBL[(n&255)>>6];
	CulcPitch();
	LfoPitch = CULC_DELTA_T;
	D2r = n & 31;
	CulcD2rStep();
};

inline void Op::SetD1LRR(int n) {
	StatTbl[DECAY].limit = D1LTBL[(n&255)>>4];
	if (Xr_stat == DECAY) {
		Xr_limit = StatTbl[DECAY].limit;
	}

	Rr = n & 15;
	CulcRrStep();
};

inline void Op::KeyON(int csm) {
	if (keyon == 0) {
		if (Xr_stat >= RELEASE) {
			// KEYON
			T = 0;

			if (Xr_el == 0) {
				Xr_stat = DECAY;
				Xr_and = StatTbl[Xr_stat].and;
				Xr_cmp = StatTbl[Xr_stat].cmp;
				Xr_add = StatTbl[Xr_stat].add;
				Xr_limit = StatTbl[Xr_stat].limit;
				if ((Xr_el>>4) == Xr_limit) {
					Xr_stat = NEXTSTAT[Xr_stat];
					Xr_and = StatTbl[Xr_stat].and;
					Xr_cmp = StatTbl[Xr_stat].cmp;
					Xr_add = StatTbl[Xr_stat].add;
					Xr_limit = StatTbl[Xr_stat].limit;
				}
			} else {
				Xr_stat = ATACK;
				Xr_and = StatTbl[Xr_stat].and;
				Xr_cmp = StatTbl[Xr_stat].cmp;
				Xr_add = StatTbl[Xr_stat].add;
				Xr_limit = StatTbl[Xr_stat].limit;
			}
		}

		if (csm == 0) {
			keyon = 1;
			csmkeyon = 0;
		} else {
			csmkeyon = 1;
		}
	}
};
inline void Op::KeyOFF(int csm) {
	if (keyon > 0 || csmkeyon > 0) {

		if (csm == 0) {
			keyon = 0;
		} else {
			csmkeyon = 0;
		}

		if (keyon == 0 && csmkeyon == 0) {
			Xr_stat = RELEASE;
			Xr_and = StatTbl[Xr_stat].and;
			Xr_cmp = StatTbl[Xr_stat].cmp;
			Xr_add = StatTbl[Xr_stat].add;
			Xr_limit = StatTbl[Xr_stat].limit;
			if ((Xr_el>>4) >= 63 || csm > 0) {
				Xr_el = 1024;
				Xr_stat = MAXSTAT[Xr_stat];
				Xr_and = StatTbl[Xr_stat].and;
				Xr_cmp = StatTbl[Xr_stat].cmp;
				Xr_add = StatTbl[Xr_stat].add;
				Xr_limit = StatTbl[Xr_stat].limit;
			}
		}
	}
};

inline void Op::Envelope(int env_counter) {
	if ((env_counter&Xr_and) == Xr_cmp) {

		if (Xr_stat==ATACK) {
			// ATACK
			Xr_step += Xr_add;
			Xr_el += ((~Xr_el)*(Xr_step>>3)) >> 4;
			LfoLevel = CULC_ALPHA;
			Xr_step &= 7;

			if (Xr_el <= 0) {
				Xr_el = 0;
				Xr_stat = DECAY;
				Xr_and = StatTbl[Xr_stat].and;
				Xr_cmp = StatTbl[Xr_stat].cmp;
				Xr_add = StatTbl[Xr_stat].add;
				Xr_limit = StatTbl[Xr_stat].limit;
				if ((Xr_el>>4) == Xr_limit) {
					Xr_stat = NEXTSTAT[Xr_stat];
					Xr_and = StatTbl[Xr_stat].and;
					Xr_cmp = StatTbl[Xr_stat].cmp;
					Xr_add = StatTbl[Xr_stat].add;
					Xr_limit = StatTbl[Xr_stat].limit;
				}
			}
		} else {
			// DECAY, SUSTAIN, RELEASE
			Xr_step += Xr_add;
			Xr_el += Xr_step>>3;
			LfoLevel = CULC_ALPHA;
			Xr_step &= 7;

			int e = Xr_el>>4;
			if (e == 63) {
				Xr_el = 1024;
				Xr_stat = MAXSTAT[Xr_stat];
				Xr_and = StatTbl[Xr_stat].and;
				Xr_cmp = StatTbl[Xr_stat].cmp;
				Xr_add = StatTbl[Xr_stat].add;
				Xr_limit = StatTbl[Xr_stat].limit;
			} else if (e == Xr_limit) {
				Xr_stat = NEXTSTAT[Xr_stat];
				Xr_and = StatTbl[Xr_stat].and;
				Xr_cmp = StatTbl[Xr_stat].cmp;
				Xr_add = StatTbl[Xr_stat].add;
				Xr_limit = StatTbl[Xr_stat].limit;
			}
		}
	}
}

inline void Op::SetNFRQ(int nfrq) {
	if ((Nfrq ^ nfrq) & 0x80) {
		LfoLevel = CULC_ALPHA;
	}
	Nfrq = nfrq;
	CulcNoiseCycle();
}
inline void Op::CulcNoiseCycle() {
	if (Nfrq & 0x80) {
		NoiseCycle = (32-(Nfrq&31)) << 25;
		if (NoiseCycle < NoiseStep) {
			NoiseCycle = NoiseStep;
		}
		NoiseCounter = NoiseCycle;
	} else {
		NoiseCycle = 0;
	}
}


inline void Op::Output0(int lfopitch, int lfolevel) {
	if (LfoPitch != lfopitch) {
//		DeltaT = ((STEPTBL[Pitch+lfopitch]+Dt1Pitch)*Mul)>>1;
		DeltaT = ((STEPTBL[Pitch+lfopitch]+Dt1Pitch)*Mul)>>(6+1);
		LfoPitch = lfopitch;
	}
	T += DeltaT;

	int lfolevelame = lfolevel & Ame;
	if (LfoLevel != lfolevelame) {
		Alpha = (int)(ALPHATBL[ALPHAZERO+Tl-Xr_el-lfolevelame]);
		LfoLevel = lfolevelame;
	}
	int o = (Alpha)
		* (int)(SINTBL[(((T+Out2Fb)>>PRECISION_BITS))&(SIZESINTBL-1)]) ;

//	int o2 = (o+Inp_last) >> 1;
//	Out2Fb = (o+o) >> Fl;
	Out2Fb = ((o + Inp_last) & Fl_mask) >> Fl;
	Inp_last = o;

	*out = o;
	*out2 = o;	// alg=5�p
	*out3 = o; // alg=5�p
//	*out = o2;
//	*out2 = o2;	// alg=5�p
//	*out3 = o2; // alg=5�p
};

inline void Op::Output(int lfopitch, int lfolevel) {
	if (LfoPitch != lfopitch) {
//		DeltaT = ((STEPTBL[Pitch+lfopitch]+Dt1Pitch)*Mul)>>1;
		DeltaT = ((STEPTBL[Pitch+lfopitch]+Dt1Pitch)*Mul)>>(6+1);
		LfoPitch = lfopitch;
	}
	T += DeltaT;

	int lfolevelame = lfolevel & Ame;
	if (LfoLevel != lfolevelame) {
		Alpha = (int)(ALPHATBL[ALPHAZERO+Tl-Xr_el-lfolevelame]);
		LfoLevel = lfolevelame;
	}
	int o = (Alpha)
		* (int)(SINTBL[(((T+inp)>>PRECISION_BITS))&(SIZESINTBL-1)]) ;

	*out += o;
};

inline void Op::Output32(int lfopitch, int lfolevel) {
	if (LfoPitch != lfopitch) {
//		DeltaT = ((STEPTBL[Pitch+lfopitch]+Dt1Pitch)*Mul)>>1;
		DeltaT = ((STEPTBL[Pitch+lfopitch]+Dt1Pitch)*Mul)>>(6+1);
		LfoPitch = lfopitch;
	}
	T += DeltaT;

	int o;
	if (NoiseCycle == 0) {
		int lfolevelame = lfolevel & Ame;
		if (LfoLevel != lfolevelame) {
			Alpha = (int)(ALPHATBL[ALPHAZERO+Tl-Xr_el-lfolevelame]);
			LfoLevel = lfolevelame;
		}
		o = (Alpha)
			* (int)(SINTBL[(((T+inp)>>PRECISION_BITS))&(SIZESINTBL-1)]) ;
	} else {
		NoiseCounter -= NoiseStep;
		if (NoiseCounter <= 0) {
			NoiseValue = (int)((irnd()>>30)&2)-1;
			NoiseCounter += NoiseCycle;
		}

		int lfolevelame = lfolevel & Ame;
		if (LfoLevel != lfolevelame) {
			Alpha = (int)(NOISEALPHATBL[ALPHAZERO+Tl-Xr_el-lfolevelame]);
			LfoLevel = lfolevelame;
		}
		o = (Alpha)
			* NoiseValue * MAXSINVAL;
	}

	*out += o;
};

inline void Op::Output0_22(int lfopitch, int lfolevel) {
	if (LfoPitch != lfopitch) {
//		DeltaT = ((STEPTBL[Pitch+lfopitch]+Dt1Pitch)*Mul)>>1;
		DeltaT = ((STEPTBL[Pitch+lfopitch]+Dt1Pitch)*Mul)>>(6+1);
		LfoPitch = lfopitch;
	}
	T += DeltaT;

	int lfolevelame = lfolevel & Ame;
	if (LfoLevel != lfolevelame) {
		Alpha = (int)(ALPHATBL[ALPHAZERO+Tl-Xr_el-lfolevelame]);
		LfoLevel = lfolevelame;
	}
	int o = (Alpha)
		* (int)(SINTBL[(((T+Out2Fb)>>PRECISION_BITS))&(SIZESINTBL-1)]) ;

	Out2Fb = ((o + Inp_last) & Fl_mask) >> Fl;
	Inp_last = o;

//	*out += o;
//	*out2 += o;	// alg=5�p
//	*out3 += o; // alg=5�p
	*out = o;
	*out2 = o;	// alg=5�p
	*out3 = o; // alg=5�p
};

inline void Op::Output_22(int lfopitch, int lfolevel) {
	if (LfoPitch != lfopitch) {
//		DeltaT = ((STEPTBL[Pitch+lfopitch]+Dt1Pitch)*Mul)>>1;
		DeltaT = ((STEPTBL[Pitch+lfopitch]+Dt1Pitch)*Mul)>>(6+1);
		LfoPitch = lfopitch;
	}
	T += DeltaT;

	int lfolevelame = lfolevel & Ame;
	if (LfoLevel != lfolevelame) {
		Alpha = (int)(ALPHATBL[ALPHAZERO+Tl-Xr_el-lfolevelame]);
		LfoLevel = lfolevelame;
	}
	int o = (Alpha)
		* (int)(SINTBL[(((T+inp)>>PRECISION_BITS))&(SIZESINTBL-1)]) ;

	*out += o;
};

inline void Op::Output32_22(int lfopitch, int lfolevel) {
	if (LfoPitch != lfopitch) {
//		DeltaT = ((STEPTBL[Pitch+lfopitch]+Dt1Pitch)*Mul)>>1;
		DeltaT = ((STEPTBL[Pitch+lfopitch]+Dt1Pitch)*Mul)>>(6+1);
		LfoPitch = lfopitch;
	}
	T += DeltaT;

	int o;
	if (NoiseCycle == 0) {
		int lfolevelame = lfolevel & Ame;
		if (LfoLevel != lfolevelame) {
			Alpha = (int)(ALPHATBL[ALPHAZERO+Tl-Xr_el-lfolevelame]);
			LfoLevel = lfolevelame;
		}
		o = (Alpha)
			* (int)(SINTBL[(((T+inp)>>PRECISION_BITS))&(SIZESINTBL-1)]) ;
	} else {
		NoiseCounter -= NoiseStep;
		if (NoiseCounter <= 0) {
			NoiseValue = (int)((irnd()>>30)&2)-1;
			NoiseCounter += NoiseCycle;
		}

		int lfolevelame = lfolevel & Ame;
		if (LfoLevel != lfolevelame) {
			Alpha = (int)(NOISEALPHATBL[ALPHAZERO+Tl-Xr_el-lfolevelame]);
			LfoLevel = lfolevelame;
		}
		o = (Alpha)
			* NoiseValue * MAXSINVAL;
	}

	*out += o;
};

