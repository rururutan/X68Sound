# X68Sound ハイレゾ音源化の実現可能性分析

## 概要

X68Sound DLLをハイレゾリューション（高解像度）オーディオに対応させる可能性について技術的な分析を行いました。

---

## ハイレゾ音源の定義

一般的なハイレゾ音源の規格：

| 項目 | CD品質 | ハイレゾ（最小） | ハイレゾ（高品質） |
|------|--------|----------------|-------------------|
| **サンプリングレート** | 44.1kHz | 96kHz | 192kHz / 384kHz |
| **ビット深度** | 16bit | 24bit | 32bit (float) |
| **データレート** | 1.4Mbps | 4.6Mbps | 12.3Mbps |

---

## 現在のX68Sound実装

### サンプリングレート対応状況

**現在対応しているレート** (`opm.h:1504-1511`):
```cpp
if (samprate == 44100) {
    Samprate = samprate;
} else if (samprate == 48000) {
    Samprate = samprate;
} else {
    Samprate = samprate;  // その他のレートも受け入れ
}
WaveOutSamp = samprate;
```

- **公式対応**: 22.05kHz, 44.1kHz, 48kHz
- **実装上**: 任意のサンプリングレートを受け入れ可能
- **内部レート**:
  - OPM: 62.5kHz（実機のYM2151内部レート）
  - PCM8/ADPCM: 187.5kHz（15625×12Hz）

### ビット深度

**現在の実装**:
```cpp
short (*PcmBuf)[2];  // 16bit ステレオバッファ
```

- **内部演算**: 32bit整数演算（int型）
- **出力フォーマット**: 16bit PCM (short型)
- **ダイナミックレンジ**: 約96dB (16bit理論値)

---

## ハイレゾ化の技術的可能性

### ✅ 可能な部分

#### 1. サンプリングレート向上（96kHz / 192kHz）

**実現可能性**: **高い（80%）**

**理由**:
- 現在の実装は既に任意のサンプリングレートを受け入れ可能
- 内部処理は187.5kHzで動作しており、ダウンサンプリングで対応
- レート変換ロジックは既に実装済み

**必要な変更**:
```cpp
// opm.h の Start() 関数
inline int Opm::Start(int samprate, int opmflag, int adpcmflag, ...) {
    // 96kHz, 192kHz対応を追加
    if (samprate == 96000) {
        Samprate = samprate;
        // FIRフィルタ係数を96kHz用に最適化
    } else if (samprate == 192000) {
        Samprate = samprate;
        // FIRフィルタ係数を192kHz用に最適化
    }
    WaveOutSamp = samprate;

    // MakeTable()で周波数テーブルを再計算
    MakeTable();
}
```

**メリット**:
- より滑らかな周波数応答
- エイリアシングのさらなる軽減
- 既存の線形補間との相乗効果

**デメリット**:
- CPU負荷が2倍（96kHz）～4倍（192kHz）に増加
- メモリ使用量の増加
- 実機のサウンドを超えた「エミュレーションを超える音質」になる

#### 2. 内部精度向上（疑似24bit/32bit処理）

**実現可能性**: **中程度（60%）**

**アプローチ**:
現在の32bit整数演算を活用して、より高精度な内部処理を実装：

```cpp
// 現在: 16bit出力 (short)
short PcmBuf[...][2];

// 改善案: 32bit整数出力（24bit相当の精度）
int PcmBuf32[...][2];  // 内部は32bit

// 最終出力時にディザリングして16bit/24bitに変換
inline short Dither24to16(int sample32) {
    // TPDF (Triangular Probability Density Function) ディザリング
    int dither = (rand() & 0x1FF) - (rand() & 0x1FF);  // -512～+512
    int sample24 = (sample32 + dither) >> 8;  // 24bit相当に丸め
    return (short)(sample24 >> 8);  // 16bitに変換
}
```

**メリット**:
- 内部演算の精度向上
- 量子化ノイズの軽減
- 微細な音色変化の再現性向上

**デメリット**:
- 出力フォーマットは16bitのまま（真のハイレゾではない）
- CPU負荷の増加

---

### ⚠️ 困難な部分

#### 3. 真の24bit/32bit出力

**実現可能性**: **低い（30%）**

**課題**:
1. **出力フォーマットの変更が必要**:
```cpp
// 現在
short (*PcmBuf)[2];  // 16bit

// 変更後（24bit packed）
struct Sample24 {
    unsigned char b0, b1, b2;  // リトルエンディアン
};
Sample24 (*PcmBuf)[2];

// または（32bit float）
float (*PcmBuf)[2];
```

2. **Windows Audio APIの変更**:
   - 現在: `waveOut` API（16bit PCMを想定）
   - 必要: WASAPI（Windows Audio Session API）への移行
   - または: ASIO対応

3. **大規模なリファクタリングが必要**:
   - すべてのバッファ処理の変更
   - 出力関数の完全な書き換え
   - Windows APIレイヤーの再実装

**実装例（WASAPIへの移行）**:
```cpp
// 現在の実装（waveOut API）
HWAVEOUT hwo;
waveOutOpen(&hwo, ...);
waveOutWrite(hwo, ...);

// 必要な実装（WASAPI）
IAudioClient *pAudioClient;
IAudioRenderClient *pRenderClient;
WAVEFORMATEXTENSIBLE wfx;
wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
wfx.Format.nSamplesPerSec = 192000;
wfx.Format.wBitsPerSample = 24;
wfx.Format.nChannels = 2;
// ... 複雑な初期化処理
```

---

## 実現可能な改善案（優先度順）

### 提案1: 96kHz/192kHz対応（高優先度）⭐⭐⭐

**実装難易度**: 中
**効果**: 大
**CPU負荷**: +100%（96kHz）/ +300%（192kHz）

#### 実装内容

1. **サンプリングレート検出の拡張**:
```cpp
inline int Opm::Start(int samprate, ...) {
    // 高サンプリングレート対応
    if (samprate == 96000 || samprate == 192000) {
        Samprate = samprate;
        WaveOutSamp = samprate;
        // 高品質FIRフィルタの選択
        useHighQualityFilter = true;
    }
    MakeTable();
}
```

2. **FIRフィルタ係数の最適化**:
```cpp
// 96kHz用フィルタ係数（より長いタップ数）
const short OPMLOWPASS_96K[OPMLPF_ROW][OPMLPF_COL] = {
    // 96kHzに最適化された係数
    // Nyquist周波数48kHzまで平坦な特性
};
```

3. **環境変数による制御**:
```cpp
// global.h
struct X68SoundConfig {
    // ... 既存フィールド ...
    int output_sample_rate;  // 出力サンプリングレート (44100/96000/192000)
};

// LoadConfigFromEnvironment()
g_Config.output_sample_rate = GetEnvInt("X68SOUND_OUTPUT_RATE", 44100);
```

#### 使用例
```batch
REM 96kHz出力モード
set X68SOUND_OUTPUT_RATE=96000
set X68SOUND_LINEAR_INTERPOLATION=1
set X68SOUND_OPM_SINE_INTERP=1

your_application.exe
```

---

### 提案2: 内部精度向上（疑似ハイレゾ）⭐⭐

**実装難易度**: 中
**効果**: 中
**CPU負荷**: +5～10%

#### 実装内容

1. **内部バッファ精度の向上**:
```cpp
// 内部演算を64bit整数に拡張
int64_t InternalPcmBuffer[PCMBUFSIZE][2];

// 最終出力時に高品質ディザリングを適用
inline short OutputWithDither(int64_t sample) {
    // TPDF ディザリング
    int dither = triangular_dither();
    int64_t dithered = sample + dither;

    // 飽和処理
    if (dithered > 32767LL << 16) dithered = 32767LL << 16;
    if (dithered < -32768LL << 16) dithered = -32768LL << 16;

    return (short)(dithered >> 16);
}
```

2. **量子化ノイズシェーピング**:
```cpp
// ノイズシェーピングフィルタ
class NoiseShaper {
    int error_history[4];
public:
    short Shape(int64_t sample) {
        // 2次ノイズシェーピング
        int64_t shaped = sample
            + error_history[0] * 2
            - error_history[1];

        short output = (short)(shaped >> 16);
        int error = (int)((shaped >> 16) - output);

        error_history[1] = error_history[0];
        error_history[0] = error;

        return output;
    }
};
```

---

### 提案3: WASAPI対応（真のハイレゾ）⭐

**実装難易度**: 非常に高
**効果**: 非常に大
**CPU負荷**: 変わらず（APIの違いのみ）

#### 実装内容

これは**大規模なリファクタリング**が必要で、別プロジェクトとして扱うべきです：

1. **新しいバックエンドレイヤーの作成**:
```cpp
// audio_backend.h
class IAudioBackend {
public:
    virtual int Init(int samprate, int bits, int channels) = 0;
    virtual int Write(void *buffer, int samples) = 0;
    virtual void Close() = 0;
};

// wasapi_backend.h
class WASAPIBackend : public IAudioBackend {
    IAudioClient *pAudioClient;
    IAudioRenderClient *pRenderClient;
    // ... WASAPI実装
};

// legacy_waveout_backend.h
class WaveOutBackend : public IAudioBackend {
    HWAVEOUT hwo;
    // ... 既存の実装を移植
};
```

2. **ビット深度対応**:
```cpp
template<typename SampleType>
class AudioBuffer {
    SampleType *data;
    int size;
public:
    // 16bit, 24bit, 32bit, floatに対応
};
```

---

## CPU負荷の試算

### 現在の実装（44.1kHz, 16bit）

- **ベースライン**: 100%
- **線形補間ON**: 約110%
- **OPM補間ON**: 約115%
- **両方ON**: 約120%

### 96kHz対応時

| 構成 | CPU負荷 | 音質向上 |
|------|---------|---------|
| 96kHz + 補間OFF | 約200% | 中 |
| 96kHz + PCM8補間ON | 約210% | 高 |
| 96kHz + すべてON | 約220% | 非常に高 |

### 192kHz対応時

| 構成 | CPU負荷 | 音質向上 |
|------|---------|---------|
| 192kHz + 補間OFF | 約400% | 高 |
| 192kHz + すべてON | 約440% | 最高 |

---

## メリット・デメリット

### 96kHz/192kHz対応のメリット

✅ **音質向上**:
- 超高周波成分の再現性向上（20kHz以上）
- エイリアシングの完全除去
- 位相特性の改善
- より滑らかなトランジェント応答

✅ **既存機能との相乗効果**:
- 線形補間との相乗効果でさらに滑らかに
- ローパスフィルタの効果がより明確に

✅ **実装の容易さ**:
- 既存コードの大部分は変更不要
- サンプリングレート関連の変更のみ

### デメリット

❌ **CPU負荷の大幅増加**:
- 96kHz: 2倍
- 192kHz: 4倍
- 低スペックPCでは実用困難

❌ **メモリ使用量の増加**:
- バッファサイズが2～4倍に

❌ **実機との乖離**:
- X68000実機は44.1kHz相当
- エミュレーションを超えた「創造的な音質向上」になる

❌ **レイテンシの増加**:
- バッファサイズ増大により遅延が増える可能性

---

## 推奨される実装順序

### フェーズ1: 96kHz対応（推奨）⭐⭐⭐

**期間**: 2～3週間
**難易度**: 中

1. サンプリングレート検出の拡張
2. 96kHz用FIRフィルタ係数の設計・実装
3. 環境変数による制御
4. テスト・デバッグ

### フェーズ2: 内部精度向上（オプション）⭐⭐

**期間**: 1～2週間
**難易度**: 中

1. 内部バッファの64bit化
2. ディザリング実装
3. ノイズシェーピング実装
4. テスト・デバッグ

### フェーズ3: 192kHz対応（将来的）⭐

**期間**: 1週間
**難易度**: 低（96kHz実装後）

1. 192kHz用フィルタ係数の設計
2. パフォーマンス最適化
3. テスト・デバッグ

### フェーズ4: WASAPI対応（長期目標）

**期間**: 2～3ヶ月
**難易度**: 非常に高

1. 設計・アーキテクチャ策定
2. バックエンドレイヤーの実装
3. 24bit/32bit対応
4. 広範なテスト

---

## 結論

### ハイレゾ化は可能か？

**答え**: **部分的に可能**

| 項目 | 実現可能性 | 推奨度 |
|------|-----------|--------|
| **96kHz対応** | ⭐⭐⭐⭐⭐ 非常に高い | ⭐⭐⭐ 推奨 |
| **192kHz対応** | ⭐⭐⭐⭐ 高い | ⭐⭐ 条件付き推奨 |
| **内部精度向上** | ⭐⭐⭐ 中程度 | ⭐⭐ オプション |
| **真の24bit出力** | ⭐⭐ 低い | ⭐ 将来的な目標 |

### 最も現実的なアプローチ

**「96kHz/16bit ハイサンプリング対応」**:
- 現在の16bit出力を維持
- サンプリングレートを96kHzに向上
- 既存の線形補間との相乗効果
- 実装が比較的容易
- CPU負荷は2倍だが、現代のPCなら十分実用的

### 実装すべきか？

**推奨**: **イエス（条件付き）**

**条件**:
1. 環境変数で96kHz出力を選択可能にする
2. デフォルトは44.1kHzのまま（互換性維持）
3. CPU負荷の警告を文書化する
4. 「エミュレーション精度」より「音質」を重視するユーザー向け

**環境変数の例**:
```batch
REM 最高音質モード（96kHz + すべての音質機能）
set X68SOUND_OUTPUT_RATE=96000
set X68SOUND_LINEAR_INTERPOLATION=1
set X68SOUND_VOLUME_SMOOTHING=1
set X68SOUND_OPM_SINE_INTERP=1
set X68SOUND_PCM_BUFFER=10
set X68SOUND_LATE_TIME=300

REM 実機互換モード（44.1kHz + 音質機能OFF）
set X68SOUND_OUTPUT_RATE=44100
set X68SOUND_LINEAR_INTERPOLATION=0
set X68SOUND_VOLUME_SMOOTHING=0
set X68SOUND_OPM_SINE_INTERP=0
```

---

## 参考情報

### 実機のサンプリングレート

- **X68000 PCM8**: 15.625kHz ～ 62.5kHz（可変）
- **X68000 ADPCM**: 3.9kHz ～ 15.625kHz（可変）
- **YM2151 (OPM)**: 内部62.5kHz相当の動作

### ハイレゾ音源の一般的な用途

- **96kHz/24bit**: スタジオマスタリング、ハイエンドオーディオ
- **192kHz/24bit**: 超ハイエンド、DSD変換のソース
- **44.1kHz/16bit**: CD、一般的なリスニング

### X68Soundの場合

実機のサウンドハードウェアは44.1kHz以下で動作しているため、**96kHz出力は「超解像」処理**になります。これは：
- ✅ 音質向上としては意味がある
- ❌ エミュレーション精度としては過剰
- ✅ 現代のオーディオ環境への最適化として有効

---

**作成日**: 2025-11-17
**バージョン**: 1.0
**対象**: X68Sound DLL ハイレゾ対応の実現可能性分析
