# PCM8音質向上提案書

## 概要

本ドキュメントでは、X68Sound2025ライブラリにおけるPCM8チャンネルの音質向上に関する改善提案をまとめています。

## 現状分析

### 現在の実装（pcm8.h）

1. **サンプルレート変換**: 最近傍補間（Nearest Neighbor）
   - `RateCounter`による単純なサンプル間引き/補間
   - サンプル間の補間処理なし

2. **ボリューム制御**: 即座の反映
   - `(((OutPcm*Volume)>>4)*TotalVolume)>>8`
   - ボリューム変更時にクリックノイズが発生する可能性

3. **HPFフィルター**: 1次IIRハイパスフィルター
   - DC成分除去用
   - 固定係数（HPF_COEFF_A1_22KHZ = 459）

4. **8チャンネルミキシング**: 飽和演算対応済み
   - `saturate_add_pcm()`により整数オーバーフロー対策済み

---

## 改善提案

### 【優先度：高】すぐに実装可能な改善

#### 1. 線形補間によるサンプルレート変換

**効果**: エイリアシングノイズの軽減、滑らかな音質

**実装方法**:
```cpp
class Pcm8 {
    int PrevInpPcm;      // 前回のサンプル値

    inline int GetPcm() {
        if (AdpcmReg & 0x80) return 0x80000000;

        RateCounter -= AdpcmRate;

        if (RateCounter < 0) {
            PrevInpPcm = InpPcm;  // 前回サンプル保存

            // 新しいサンプルを取得
            while (RateCounter < 0) {
                // ... 既存のサンプル取得処理 ...
                if (PcmKind == 5) {
                    int dataH = DmaGetByte();
                    if (dataH == 0x80000000) {
                        RateCounter = 0;
                        AdpcmReg = 0xC7;
                        return 0x80000000;
                    }
                    int dataL = DmaGetByte();
                    if (dataL == 0x80000000) {
                        RateCounter = 0;
                        AdpcmReg = 0xC7;
                        return 0x80000000;
                    }
                    pcm16_2pcm((int)(short)((dataH<<8)|dataL));
                } else if (PcmKind == 6) {
                    int data = DmaGetByte();
                    if (data == 0x80000000) {
                        RateCounter = 0;
                        AdpcmReg = 0xC7;
                        return 0x80000000;
                    }
                    pcm16_2pcm((int)(char)data);
                } else {
                    if (N1DataFlag == 0) {
                        int N10Data = DmaGetByte();
                        if (N10Data == 0x80000000) {
                            RateCounter = 0;
                            AdpcmReg = 0xC7;
                            return 0x80000000;
                        }
                        adpcm2pcm(N10Data & 0x0F);
                        N1Data = (N10Data >> 4) & 0x0F;
                        N1DataFlag = 1;
                    } else {
                        adpcm2pcm(N1Data);
                        N1DataFlag = 0;
                    }
                }

                RateCounter += 15625*12;
            }

            // 線形補間の適用
            // frac = RateCounter / (15625*12) の割合で補間
            int sampleInterval = 15625*12;
            int frac = (RateCounter << 16) / sampleInterval;  // 16bit固定小数点
            InpPcm = PrevInpPcm + (((InpPcm - PrevInpPcm) * frac) >> 16);
        }

        // 既存のHPF処理
        OutInpPcm = InpPcm;
        OutPcm = ((InpPcm << HPF_SHIFT) - (InpPcm_prev << HPF_SHIFT)
                  + HPF_COEFF_A1_22KHZ * OutPcm) >> HPF_SHIFT;
        InpPcm_prev = InpPcm;

        return (((OutPcm*Volume)>>4)*TotalVolume)>>8;
    }
};
```

**メリット**:
- エイリアシングノイズが大幅に軽減
- CPU負荷増加は約5%程度（乗算1回、加算2回のみ）
- 実装コストが低い

**デメリット**:
- メモリ使用量が微増（int型1個 × 8チャンネル）

---

#### 2. ボリュームスムージング（クリックノイズ対策）

**効果**: ボリューム変更時のクリックノイズ除去

**実装方法**:
```cpp
class Pcm8 {
    int CurrentVolume;    // 現在の実効ボリューム（内部で徐々に変化）
    int TargetVolume;     // 目標ボリューム

    // SetVol関数を呼ぶ際の変更
    inline void SetVol(int v) {
        TargetVolume = PCM8VOLTBL[v & 15];
        // Volume = PCM8VOLTBL[v & 15];  // この行を削除
        Mode = (Mode&0xFF00FFFF)|(v<<16);
    }

    // Init関数に追加
    inline void Init() {
        // ... 既存の初期化 ...
        CurrentVolume = 0;
        TargetVolume = 0;
    }

    inline int GetPcm() {
        // ... サンプル取得処理 ...

        // ボリュームを徐々に変化（256サンプルで目標値に到達）
        if (CurrentVolume != TargetVolume) {
            int diff = TargetVolume - CurrentVolume;
            if (diff > 0) {
                CurrentVolume += (diff >> 8) + 1;
                if (CurrentVolume > TargetVolume) CurrentVolume = TargetVolume;
            } else {
                CurrentVolume += (diff >> 8) - 1;
                if (CurrentVolume < TargetVolume) CurrentVolume = TargetVolume;
            }
        }

        // CurrentVolumeを使用して音量計算
        return (((OutPcm*CurrentVolume)>>4)*TotalVolume)>>8;
    }
};
```

**メリット**:
- クリックノイズが完全に除去される
- CPU負荷増加は1%未満
- 実装が非常に簡単

**デメリット**:
- ボリューム変化が若干遅延する（約5ms程度）

---

### 【優先度：中】品質重視の場合に実装

#### 3. ディザリング（量子化ノイズ対策）

**効果**: 低ビット深度環境での量子化ノイズ低減

**実装方法**:
```cpp
// global.hに追加
inline int ApplyTPDFDither(int sample) {
    // TPDF（三角確率密度関数）ディザー
    static unsigned int seed = 0x12345678;

    // 簡易乱数生成（xorshift）
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    int rand1 = (int)(seed & 0xFF);

    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    int rand2 = (int)(seed & 0xFF);

    // 三角分布ディザー（-255 ～ +255）
    int dither = rand1 - rand2;

    // ディザーを追加（最下位ビット付近）
    return sample + (dither >> 4);
}

// pcm8.h GetPcm()の最後に適用
inline int GetPcm() {
    // ... 既存処理 ...

    int output = (((OutPcm*Volume)>>4)*TotalVolume)>>8;
    return ApplyTPDFDither(output);
}
```

**メリット**:
- 量子化ノイズが耳障りでなくなる
- 低音量時の音質向上
- CPU負荷増加は2～3%程度

**デメリット**:
- わずかにノイズフロアが上がる

---

#### 4. 2次IIRハイパスフィルター

**効果**: DC成分除去性能の向上、より急峻な周波数特性

**実装方法**:
```cpp
// global.hに定数追加
// Butterworth 2次HPF @ 44.1kHz, fc=10Hz の係数例
#define HPF2_B0  1023      // b0 ≈ 0.9993
#define HPF2_B1 -2046      // b1 ≈ -1.9986
#define HPF2_B2  1023      // b2 ≈ 0.9993
#define HPF2_A1  2045      // a1 ≈ 1.9986
#define HPF2_A2 -1022      // a2 ≈ -0.9987
#define HPF2_SHIFT 10      // 係数のシフト量

class Pcm8 {
    int InpPcm_prev2;      // 2サンプル前の入力
    int OutPcm_prev;       // 1サンプル前の出力
    int OutPcm_prev2;      // 2サンプル前の出力

    inline int ApplyHPF2(int input) {
        // 2次IIRフィルター: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
        int output = (HPF2_B0 * input
                     + HPF2_B1 * InpPcm_prev
                     + HPF2_B2 * InpPcm_prev2
                     - HPF2_A1 * OutPcm_prev
                     - HPF2_A2 * OutPcm_prev2) >> HPF2_SHIFT;

        // 履歴更新
        InpPcm_prev2 = InpPcm_prev;
        InpPcm_prev = input;
        OutPcm_prev2 = OutPcm_prev;
        OutPcm_prev = output;

        return output;
    }
};
```

**メリット**:
- DC成分除去性能が大幅に向上
- 低周波のゴースト音が軽減

**デメリット**:
- CPU負荷が約5～10%増加
- メモリ使用量増加（int型4個 × 8チャンネル）

---

### 【優先度：低】将来的な検討項目

#### 5. 高品質補間（Sinc補間）

**効果**: プロフェッショナル品質のサンプルレート変換

**概要**:
- Windowed Sincフィルター（Lanczos窓など）を使用
- 複数サンプルを参照して補間
- libsamplerate (Secret Rabbit Code) のような既存ライブラリ使用を推奨

**メリット**:
- 業務用レベルの高音質
- エイリアシングがほぼ完全に除去

**デメリット**:
- CPU負荷が30～50%増加
- 実装が複雑
- メモリ使用量が大幅に増加

---

#### 6. アンチエイリアシングフィルター

**効果**: ダウンサンプリング時の折り返しノイズ除去

**概要**:
- FIRローパスフィルターをサンプルレート変換前に適用
- カットオフ周波数 = 出力サンプルレート / 2

**メリット**:
- 折り返しノイズが完全に除去
- 非常にクリアな音質

**デメリット**:
- CPU負荷が20～40%増加
- 大幅な実装変更が必要
- レイテンシが増加

---

#### 7. SIMD最適化（並列処理）

**効果**: CPU負荷削減（音質向上ではない）

**概要**:
- SSE/AVX命令を使用して8チャンネルを並列処理
- 特にミキシング処理で効果的

**実装例**:
```cpp
#ifdef _M_IX86
#include <emmintrin.h>  // SSE2

// 4チャンネルを同時処理
__m128i ch0_1 = _mm_loadu_si128((__m128i*)&pcm_data[0]);
__m128i ch2_3 = _mm_loadu_si128((__m128i*)&pcm_data[2]);
__m128i vol0_1 = _mm_loadu_si128((__m128i*)&volume[0]);
__m128i vol2_3 = _mm_loadu_si128((__m128i*)&volume[2]);

__m128i result0_1 = _mm_mullo_epi32(ch0_1, vol0_1);
__m128i result2_3 = _mm_mullo_epi32(ch2_3, vol2_3);
#endif
```

**メリット**:
- CPU負荷を30～50%削減可能
- 他の改善と組み合わせて全体の負荷を相殺できる

**デメリット**:
- プラットフォーム依存コードが増える
- ARM64対応が別途必要（NEON命令）
- 実装・テストコストが高い

---

## 推奨実装ロードマップ

### フェーズ1: クイックウィン（1～2週間）
1. **ボリュームスムージング** の実装
   - クリックノイズ問題を即座に解決
   - リスク低、効果大

2. **線形補間** の実装
   - 音質が明確に向上
   - CPU負荷増加が少ない

### フェーズ2: 品質強化（2～4週間）
3. **ディザリング** の実装（オプション機能として）
   - 環境変数でON/OFF切り替え可能に
   - 低音量再生時の品質向上

4. **2次HPFフィルター** の実装（オプション）
   - 既存の1次フィルターと切り替え可能に
   - 環境変数で選択可能

### フェーズ3: 最適化（将来的）
5. **SIMD最適化** の検討
   - フェーズ1～2の負荷増加を相殺
   - プラットフォームごとに段階的実装

6. **高品質補間** の検討
   - 上級ユーザー向けオプション機能として
   - libsamplerate等の外部ライブラリ統合を検討

---

## 環境変数による制御（提案）

新しい改善機能を環境変数で制御可能にすることを推奨：

```batch
REM 線形補間の有効化/無効化
set X68SOUND_LINEAR_INTERPOLATION=1

REM ボリュームスムージングの有効化/無効化
set X68SOUND_VOLUME_SMOOTHING=1

REM ディザリングの有効化/無効化
set X68SOUND_DITHERING=1

REM HPFフィルター種別（1:1次IIR, 2:2次IIR）
set X68SOUND_HPF_ORDER=1
```

---

## 参考資料

### サンプルレート変換
- Smith, J.O. "Digital Audio Resampling Home Page"
  http://ccrma.stanford.edu/~jos/resample/

- Secret Rabbit Code (libsamplerate)
  http://www.mega-nerd.com/SRC/

### ディザリング
- Wannamaker, R.A., et al. "A Theory of Nonsubtractive Dither" (1999)
- Lipshitz, S.P., et al. "Quantization and Dither: A Theoretical Survey" (1992)

### デジタルフィルター
- Butterworth Filter Design
  https://en.wikipedia.org/wiki/Butterworth_filter

---

## まとめ

PCM8チャンネルの音質向上には、以下の順序で実装することを推奨します：

1. **ボリュームスムージング** （即効性大、実装容易）
2. **線形補間** （音質向上明確、CPU負荷小）
3. **ディザリング** （低音量時の品質向上）
4. **2次HPFフィルター** （DC成分除去強化）

これらの改善により、X68000実機を超える高品質なPCM再生が実現できます。

---

**作成日**: 2025-11-17
**対象バージョン**: X68Sound2025
**作成者**: Claude (AI Assistant)
