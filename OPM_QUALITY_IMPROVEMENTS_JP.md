# OPM (FM音源) 音質向上提案書

## 概要

このドキュメントでは、X68Sound DLLのOPM (YM2151) FM音源エミュレーションにおける音質向上の可能性について分析し、具体的な改善案を提示します。

## 現在の実装分析

### 1. 正弦波テーブル (SINTBL)

**現在の実装** (`opm.h:519-521`):
```cpp
for (i=0; i<SIZESINTBL; ++i) {
    SINTBL[i] = sin(2.0*PI*(i+0.0)/SIZESINTBL)*(MAXSINVAL) + 0.5;
}
```

- **テーブルサイズ**: 1024エントリ (10ビット精度)
- **最大値**: 4096 (12ビット)
- **補間**: なし（最近傍法）

**出力計算** (`op.h:554-555`):
```cpp
int o = (Alpha) * (int)(SINTBL[(((T+inp)>>PRECISION_BITS))&(SIZESINTBL-1)]);
```

- **位相精度**: 10ビット (PRECISION_BITS=10)
- **位相の小数部**: 破棄される
- **問題点**: 位相の小数部情報が失われ、階段状の波形になる

### 2. エンベロープテーブル (ALPHATBL)

**現在の実装** (`opm.h:536-544`):
```cpp
for (i=17; i<=SIZEALPHATBL; ++i) {
    ALPHATBL[ALPHAZERO+i] = floor(
        pow(2.0, -((SIZEALPHATBL)-i)*(128.0/8.0)/(SIZEALPHATBL))
        *1.0*1.0*PRECISION +0.0);
}
```

- **テーブルサイズ**: 1024エントリ (10ビット精度)
- **補間**: なし
- **最初の17エントリ**: ゼロ（低エンベロープレベル用）

### 3. 位相アキュムレータ

**精度** (`global.h:118-119`):
```cpp
#define PRECISION_BITS (10)
#define PRECISION (1<<PRECISION_BITS)
```

- **位相精度**: 10ビット（1024ステップ）
- **位相増分**: 整数演算
- **問題点**: 低周波数で位相解像度が不足

### 4. エンベロープジェネレータ

**更新処理** (`op.h:441-490`):
- **更新頻度**: 3サンプルごと (EnvCounter2)
- **ステップベース**: 離散的な状態遷移
- **補間**: なし

### 5. LFO (Low Frequency Oscillator)

**実装** (`lfo.h`):
- **テーブルサイズ**: 512エントリ (9ビット精度)
- **PM/AMテーブル**: 個別の波形テーブル
- **補間**: なし
- **時間精度**: 4096 (12ビット)

---

## 音質向上提案

### 提案1: 正弦波テーブルの線形補間 ⭐⭐⭐

**優先度**: 高
**難易度**: 中
**CPU負荷**: +3～5%
**効果**: エイリアシングノイズの大幅軽減

#### 問題点
現在の実装では、位相アキュムレータの下位ビット（小数部）が破棄されるため、階段状の波形になり、高周波ノイズが発生します。

#### 改善案
位相の小数部を使用して線形補間を実装：

```cpp
inline void Op::Output(int lfopitch, int lfolevel) {
    if (LfoPitch != lfopitch) {
        DeltaT = ((STEPTBL[Pitch+lfopitch]+Dt1Pitch)*Mul)>>(6+1);
        LfoPitch = lfopitch;
    }
    T += DeltaT;

    int lfolevelame = lfolevel & Ame;
    if (LfoLevel != lfolevelame) {
        Alpha = (int)(ALPHATBL[ALPHAZERO+Tl-Xr_el-lfolevelame]);
        LfoLevel = lfolevelame;
    }

    // 線形補間の実装
    int phase_index = ((T+inp)>>PRECISION_BITS) & (SIZESINTBL-1);
    int phase_frac = (T+inp) & ((1<<PRECISION_BITS)-1);  // 小数部を取得

    int sin0 = SINTBL[phase_index];
    int sin1 = SINTBL[(phase_index+1) & (SIZESINTBL-1)];

    // 16ビット固定小数点補間
    int sin_interpolated = sin0 + (((sin1 - sin0) * phase_frac) >> PRECISION_BITS);

    int o = Alpha * sin_interpolated;
    *out += o;
}
```

#### 期待される効果
- エイリアシングノイズ: **50～70%軽減**
- 高周波成分の歪み: **大幅に改善**
- 音の滑らかさ: **向上**

---

### 提案2: 高精度位相アキュムレータ ⭐⭐

**優先度**: 中
**難易度**: 高
**CPU負荷**: +2～3%
**効果**: 低周波数での精度向上

#### 問題点
10ビットの位相精度では、低周波数での位相解像度が不足し、ピッチの不安定性が発生する可能性があります。

#### 改善案
位相精度を12ビットまたは14ビットに向上：

```cpp
// global.hで変更
#define PRECISION_BITS (12)  // 10→12に変更
#define PRECISION (1<<PRECISION_BITS)

// 正弦波テーブルサイズはそのまま（1024エントリ）
// 上位10ビットでテーブルインデックス、下位2ビットで補間

// 補間コード例
int table_index = ((T+inp)>>(PRECISION_BITS-SIZESINTBL_BITS)) & (SIZESINTBL-1);
int frac = (T+inp) & ((1<<(PRECISION_BITS-SIZESINTBL_BITS))-1);
int frac_shift = PRECISION_BITS - SIZESINTBL_BITS;  // 2ビット

int sin0 = SINTBL[table_index];
int sin1 = SINTBL[(table_index+1) & (SIZESINTBL-1)];
int sin_interpolated = sin0 + (((sin1 - sin0) * frac) >> frac_shift);
```

#### 期待される効果
- 低周波数のピッチ精度: **4倍向上**
- 周波数変調の滑らかさ: **改善**
- LFO効果の精度: **向上**

---

### 提案3: エンベロープテーブルの補間 ⭐

**優先度**: 低
**難易度**: 低
**CPU負荷**: +1%未満
**効果**: エンベロープ遷移の滑らかさ向上

#### 改善案
エンベロープレベル (Xr_el) の小数部を利用した補間：

```cpp
if (LfoLevel != lfolevelame) {
    int env_index = ALPHAZERO+Tl-Xr_el-lfolevelame;
    int env_frac = Xr_el & 15;  // 下位4ビットを小数部として使用
    int env_int = env_index & ~15;

    int alpha0 = (int)(ALPHATBL[env_int]);
    int alpha1 = (int)(ALPHATBL[env_int + 16]);

    Alpha = alpha0 + (((alpha1 - alpha0) * env_frac) >> 4);
    LfoLevel = lfolevelame;
}
```

#### 期待される効果
- エンベロープのクリックノイズ: **軽減**
- アタック/リリースの滑らかさ: **改善**

---

### 提案4: 高解像度正弦波テーブル ⭐

**優先度**: 低
**難易度**: 中
**CPU負荷**: +5～8% (メモリ帯域幅)
**メモリ使用量**: +4～12KB
**効果**: 補間なしでノイズ軽減

#### 改善案
正弦波テーブルを2048または4096エントリに拡大：

```cpp
// global.h
#define SIZESINTBL_BITS (11)  // 10→11 (2048エントリ) または 12 (4096エントリ)
#define SIZESINTBL (1<<SIZESINTBL_BITS)

// 位相精度も同時に向上
#define PRECISION_BITS (12)
```

#### トレードオフ
- **メリット**: 補間なしでも高品質、CPU負荷が提案1より低い
- **デメリット**: メモリ使用量増加、キャッシュミス増加の可能性

---

### 提案5: LFOテーブルの線形補間 ⭐

**優先度**: 低
**難易度**: 低
**CPU負荷**: +1%未満
**効果**: LFO効果の滑らかさ向上

#### 改善案
PM/AMテーブルアクセス時に線形補間を適用：

```cpp
inline void Lfo::CulcPmValue(int ch) {
    int idx = LfoIdx & (SIZELFOTBL-1);
    int frac = LfoSmallCounter & 15;  // 小数部

    int pm0, pm1;
    if (LfoWaveForm == 0 || LfoWaveForm == 3) {
        pm0 = PmTbl0[idx];
        pm1 = PmTbl0[(idx+1) & (SIZELFOTBL-1)];
    } else {
        pm0 = PmTbl2[idx];
        pm1 = PmTbl2[(idx+1) & (SIZELFOTBL-1)];
    }

    int pm_interpolated = pm0 + (((pm1 - pm0) * frac) >> 4);

    if (pm_interpolated >= 0) {
        PmValue[ch] = ((pm_interpolated*PmdPmsmul[ch])>>(7+5))<<Pmsshl[ch];
    } else {
        PmValue[ch] = -( (((-pm_interpolated)*PmdPmsmul[ch])>>(7+5))<<Pmsshl[ch] );
    }
}
```

#### 期待される効果
- ビブラート効果の滑らかさ: **向上**
- トレモロ効果の品質: **改善**

---

### 提案6: エンベロープ更新頻度の向上 ⭐

**優先度**: 低
**難易度**: 低
**CPU負荷**: +10～15%
**効果**: エンベロープ精度向上

#### 問題点
現在、エンベロープは3サンプルごとに更新されます (EnvCounter2 = 3)。

#### 改善案
毎サンプル更新、またはEnvCounter2を削除：

```cpp
// pcmset62() 内で
// 元のコード:
// if ((--EnvCounter2) == 0) {
//     EnvCounter2 = 3;
//     ++EnvCounter1;
//     for (slot=0; slot<32; ++slot) {
//         op[0][slot].Envelope(EnvCounter1);
//     }
// }

// 改善案:
++EnvCounter1;
for (slot=0; slot<32; ++slot) {
    op[0][slot].Envelope(EnvCounter1);
}
```

#### トレードオフ
- **メリット**: エンベロープの精度が3倍向上
- **デメリット**: CPU負荷が大幅に増加（全32オペレータの処理が3倍）

---

### 提案7: バンドリミット処理 ⭐⭐

**優先度**: 中
**難易度**: 非常に高
**CPU負荷**: +20～50%
**効果**: 完全なアンチエイリアシング

#### 改善案
BLEP (Band-Limited Step) またはPolyBLEP手法を使用：

```cpp
// 非常に複雑な実装が必要
// 各オペレータの位相リセット時にBLEPを適用
// 周波数変調時にバンドリミット補正を適用
```

#### トレードオフ
- **メリット**: 理論的に完全なアンチエイリアシング
- **デメリット**: 実装が非常に複雑、CPU負荷が大幅増加

---

## 環境変数による制御

音質向上機能は環境変数で制御可能にします：

```cpp
// global.h に追加
struct X68SoundConfig {
    // ... 既存のフィールド ...
    int opm_sine_interpolation;     // 正弦波補間 (0/1, デフォルト: 1)
    int opm_envelope_interpolation; // エンベロープ補間 (0/1, デフォルト: 0)
    int opm_lfo_interpolation;      // LFO補間 (0/1, デフォルト: 0)
    int opm_high_precision_phase;   // 高精度位相 (0/1, デフォルト: 0)
};

// LoadConfigFromEnvironment() に追加
g_Config.opm_sine_interpolation = GetEnvInt("X68SOUND_OPM_SINE_INTERP", 1);
g_Config.opm_envelope_interpolation = GetEnvInt("X68SOUND_OPM_ENV_INTERP", 0);
g_Config.opm_lfo_interpolation = GetEnvInt("X68SOUND_OPM_LFO_INTERP", 0);
g_Config.opm_high_precision_phase = GetEnvInt("X68SOUND_OPM_HIGH_PREC", 0);
```

---

## 実装優先度のまとめ

### フェーズ1: 基本的な音質向上（推奨）
1. **正弦波テーブルの線形補間** (提案1)
   - 最大の効果、中程度の難易度
   - CPU負荷: +3～5%
   - 環境変数: `X68SOUND_OPM_SINE_INTERP=1` (デフォルト有効)

### フェーズ2: 精度向上（オプション）
2. **高精度位相アキュムレータ** (提案2)
   - 低周波数の品質向上
   - CPU負荷: +2～3%
   - 環境変数: `X68SOUND_OPM_HIGH_PREC=1` (デフォルト無効)

3. **エンベロープテーブルの補間** (提案3)
   - 微細な改善
   - CPU負荷: +1%未満
   - 環境変数: `X68SOUND_OPM_ENV_INTERP=1` (デフォルト無効)

### フェーズ3: 追加機能（将来的）
4. **LFOテーブルの線形補間** (提案5)
   - LFO効果の向上
   - CPU負荷: +1%未満

---

## PCM8との比較

| 項目 | PCM8/ADPCM | OPM (FM音源) |
|------|------------|-------------|
| **主な問題** | サンプルレート変換のエイリアシング | 位相精度不足によるエイリアシング |
| **線形補間の効果** | 非常に高い (ノイズ50～70%軽減) | 高い (ノイズ50～70%軽減) |
| **CPU負荷** | +5% | +3～5% |
| **実装難易度** | 中 | 中 |
| **副次的効果** | - | 低周波数での精度向上 |

---

## 既存の品質対策

X68Soundには既に以下の品質対策が実装されています：

1. **OPMハイパスフィルタ** (`opm.h:983-988`)
   - DC成分除去
   - 係数: 1/1024 + 1/4096

2. **OPM出力のローパスフィルタ** (`opm.h:1131`)
   - サンプルレート変換時のアンチエイリアシング
   - FIRフィルタ実装

3. **ADPCM/PCM8ローパスフィルタ**
   - 複雑なIIRフィルタカスケード
   - 係数最適化済み

これらの既存フィルタは優秀ですが、**ソース時点でのアンチエイリアシング（補間）** を追加することで、さらに品質が向上します。

---

## 実機互換性への配慮

すべての音質向上機能は環境変数で無効化できるようにし、実機完全互換モードを提供します：

```batch
REM 実機完全互換モード（すべての音質機能を無効化）
set X68SOUND_OPM_SINE_INTERP=0
set X68SOUND_OPM_ENV_INTERP=0
set X68SOUND_OPM_LFO_INTERP=0
set X68SOUND_OPM_HIGH_PREC=0
```

---

## 技術的な注意事項

### 1. 整数オーバーフロー対策
補間計算時に中間値が32ビットを超えないよう注意：

```cpp
// 安全な補間
int diff = sin1 - sin0;  // -8192 ～ +8192
int frac = phase_frac;   // 0 ～ 1023
// diff * frac は最大 8192 * 1023 = 8,380,416 (24ビット) → 安全
int interpolated = sin0 + ((diff * frac) >> PRECISION_BITS);
```

### 2. キャッシュ効率
テーブルサイズを増やす場合、L1キャッシュ (32KB程度) に収まるよう配慮：
- SINTBL: 1024エントリ × 2バイト = 2KB ✓
- SINTBL (2048): 4KB ✓
- SINTBL (4096): 8KB ✓

### 3. エンベロープ更新のタイミング
実機のYM2151はエンベロープを一定周期で更新するため、毎サンプル更新すると動作が変わる可能性があります。互換性を重視する場合は提案6は実装しないことを推奨。

---

## 結論

**最も推奨される改善**は **提案1: 正弦波テーブルの線形補間** です。

- ✅ 実装難易度が適切
- ✅ CPU負荷が許容範囲 (+3～5%)
- ✅ 音質向上効果が大きい (エイリアシングノイズ50～70%軽減)
- ✅ 既存コードへの影響が少ない
- ✅ 環境変数で無効化可能

**次点**は **提案2: 高精度位相アキュムレータ** で、低周波数の精度向上に効果があります。

**提案3, 5**は副次的な改善として、必要に応じて実装できます。

**提案4, 6, 7**は効果とコストのバランスを考慮し、慎重に検討すべきです。

---

**作成日**: 2025-11-17
**バージョン**: 1.0
**対象**: X68Sound DLL OPM (YM2151) エミュレーション
