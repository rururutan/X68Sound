
# X68Sound2025

Sharp X68000のサウンドハードウェア（OPM/ADPCM/PCM8）をエミュレートするWindows用DLLライブラリです。

## 最新の変更履歴

### 2025/11/17 - v2.1 ハイレゾ対応アップデート

#### 新機能
* **ハイレゾ音源対応**: 96kHz/192kHz出力サンプリングレートをサポート
  - 環境変数 `X68SOUND_OUTPUT_RATE` で設定可能
  - 96kHz: CPU負荷2倍、高品質な音質
  - 192kHz: CPU負荷4倍、超高品質な音質
  - 線形補間機能との相乗効果でエイリアシングノイズをさらに軽減
  - 詳細は [HIRES_AUDIO_FEASIBILITY_JP.md](HIRES_AUDIO_FEASIBILITY_JP.md) を参照

### 2025/11/17 - v2.0 音質向上アップデート

#### 新機能
* **PCM8/ADPCM線形補間**: サンプルレート変換に線形補間を実装
  - エイリアシングノイズを大幅に軽減（50～70%）
  - 高周波成分の歪みを低減
  - CPU負荷: 約5%増加
  - 環境変数 `X68SOUND_LINEAR_INTERPOLATION` で制御可能（デフォルト: 有効）

* **OPM線形補間**: FM音源の正弦波テーブルに線形補間を実装
  - エイリアシングノイズを大幅に軽減（50～70%）
  - FM変調の滑らかさが向上
  - 位相精度の向上
  - CPU負荷: 約3～5%増加
  - 環境変数 `X68SOUND_OPM_SINE_INTERP` で制御可能（デフォルト: 有効）

* **ボリュームスムージング**: PCM8のクリックノイズ完全除去
  - ボリューム変更時に徐々に変化（約5ms）
  - CPU負荷: 1%未満
  - 環境変数 `X68SOUND_VOLUME_SMOOTHING` で制御可能（デフォルト: 有効）

* **環境変数によるオーディオ設定**: 再コンパイル不要の設定変更
  - バッファサイズ、レイテンシ、音質機能などを環境変数で制御
  - 詳細は [ENVIRONMENT_VARIABLES.md](ENVIRONMENT_VARIABLES.md) を参照

#### 改善
* コード品質の向上
  - マジックナンバーを定数化（HPF係数、パンニング定数など）
  - PCM8ミキシングロジックの可読性向上
  - 整数オーバーフロー対策（飽和演算）
  - 日本語コメントの整理と追加

#### ドキュメント
* [PCM8_QUALITY_IMPROVEMENTS_JP.md](PCM8_QUALITY_IMPROVEMENTS_JP.md) - PCM8音質向上の詳細解説
* [OPM_QUALITY_IMPROVEMENTS_JP.md](OPM_QUALITY_IMPROVEMENTS_JP.md) - OPM音質向上の詳細解説
* [ENVIRONMENT_VARIABLES.md](ENVIRONMENT_VARIABLES.md) - 環境変数設定ガイド

### 2025/01/17 - 初期リリース
* Shift-JISからUTF-8への変換
* `__int64` を `int64_t` に置換
* Visual Studio 2022対応のビルドファイル追加

---

## 過去の変更履歴

### 2016/09/12
* CSM音声合成モードの実装(GIMONS氏提供)

### 2013/08/03
* SRC/Low-passFilterを無効化する(LinearDrive氏提供)
* 44.1KHz/48KHzでは過去互換を取って従来通り
* 22KHzでチャンネルマスクが効いていなかったのを修正
* LFOの修正をVOPMから持ってきた

### 2013/02/12
* C86CTLへの対応を追加（honet氏提供）
* PCM8モード時のDMAチェイン動作を修正

### 2013/01/09
* x64バイナリのビルドを可能にした。pcidebug.dll自体がx64非対応の為、ROMEOサポートを外しています

### 2011/01/19
* チャンネルマスク機能(X68Sound_SetMask)を追加

### 2003/09/26
* 22/44.1/48KHz以外の出力許す様に修正
* ROMEOパッチ(GORRY氏)/ROMEO-Athronパッチ(うさ氏)のコードをマージ

---

## 使い方

### 基本的な使用方法

```cpp
#include "x68sound.h"

// 初期化
X68Sound_Start(
    44100,  // サンプルレート
    1,      // OPM有効
    1,      // ADPCM有効
    5,      // Between time (ms)
    5,      // PCMバッファサイズ
    200,    // レイテンシ (ms)
    1.0     // サンプルレート補正マージン
);

// OPMレジスタ操作
X68Sound_OpmReg(0x20);      // レジスタ選択
X68Sound_OpmPoke(0x80);     // データ書き込み

// 終了処理
X68Sound_Free();
```

### 環境変数による設定

音質や動作を環境変数でカスタマイズできます。詳細は [ENVIRONMENT_VARIABLES.md](ENVIRONMENT_VARIABLES.md) を参照してください。

#### 基本設定
```batch
REM バッファサイズとレイテンシ
set X68SOUND_PCM_BUFFER=5
set X68SOUND_LATE_TIME=200

REM デバッグログ有効化
set X68SOUND_DEBUG=1
```

#### 音質設定
```batch
REM すべての音質向上機能（デフォルトで有効）
set X68SOUND_LINEAR_INTERPOLATION=1
set X68SOUND_VOLUME_SMOOTHING=1
set X68SOUND_OPM_SINE_INTERP=1

REM 実機完全互換モード（すべての音質機能を無効化）
set X68SOUND_LINEAR_INTERPOLATION=0
set X68SOUND_VOLUME_SMOOTHING=0
set X68SOUND_OPM_SINE_INTERP=0
```

#### ハイレゾ設定
```batch
REM 96kHzハイレゾ出力（推奨）
set X68SOUND_OUTPUT_RATE=96000
set X68SOUND_PCM_BUFFER=7
set X68SOUND_LATE_TIME=200

REM 192kHz超高品質出力（ハイスペックPC向け）
set X68SOUND_OUTPUT_RATE=192000
set X68SOUND_PCM_BUFFER=10
set X68SOUND_LATE_TIME=300
set X68SOUND_BUF_MULTIPLIER=2
```

### ビルド方法

#### Visual Studio 2022
```batch
REM x86版
build_VS2022_x86.bat

REM x64版
build_VS2022_x64.bat

REM ARM64版
build_VS2022_arm64.bat
```

#### Visual Studio 2019
```batch
REM x86版
build_VS2019_x86.bat

REM x64版
build_VS2019_x64.bat
```

ビルド後のDLLは `build/` ディレクトリに出力されます。

---

## 機能

### エミュレート対象
- **OPM (YM2151)**: FM音源8チャンネル
- **ADPCM**: ADPCMチャンネル
- **PCM8**: 8チャンネルPCM再生（ソフトウェアミキシング）

### 対応フォーマット
- **サンプルレート**: 22.05kHz / 44.1kHz / 48kHz / 96kHz / 192kHz (その他のレートも対応)
- **PCM形式**: 16bit PCM / 8bit PCM / ADPCM

### 音質向上機能（v2.0以降）
- **PCM8/ADPCM線形補間**: サンプルレート変換時のエイリアシングノイズを軽減（50～70%）
- **OPM線形補間**: FM音源の正弦波生成時のエイリアシングノイズを軽減（50～70%）
- **ボリュームスムージング**: PCM8ボリューム変更時のクリックノイズを除去
- **飽和演算**: PCM8の8チャンネル同時再生時のオーバーフロー対策
- **ハイレゾ音源対応**: 96kHz/192kHz出力サンプリングレート（v2.1以降）

---

## ドキュメント

- [ENVIRONMENT_VARIABLES.md](ENVIRONMENT_VARIABLES.md) - 環境変数設定ガイド
- [PCM8_QUALITY_IMPROVEMENTS_JP.md](PCM8_QUALITY_IMPROVEMENTS_JP.md) - PCM8音質向上の詳細解説
- [OPM_QUALITY_IMPROVEMENTS_JP.md](OPM_QUALITY_IMPROVEMENTS_JP.md) - OPM音質向上の詳細解説
- [HIRES_AUDIO_FEASIBILITY_JP.md](HIRES_AUDIO_FEASIBILITY_JP.md) - ハイレゾ音源対応の技術解説

---

## 動作環境

- **OS**: Windows 10/11 (x86/x64/ARM64)
- **CPU**: Pentium以上 (SIMD命令対応を推奨)
- **メモリ**: 16MB以上

---

## トラブルシューティング

### 音が途切れる
バッファサイズとレイテンシを増やしてください：
```batch
set X68SOUND_PCM_BUFFER=10
set X68SOUND_LATE_TIME=300
set X68SOUND_BUF_MULTIPLIER=2
```

### レイテンシが大きい
バッファサイズを減らしてください：
```batch
set X68SOUND_PCM_BUFFER=3
set X68SOUND_LATE_TIME=100
```

### 音質を実機に近づけたい
すべての音質向上機能を無効化してください：
```batch
set X68SOUND_LINEAR_INTERPOLATION=0
set X68SOUND_VOLUME_SMOOTHING=0
set X68SOUND_OPM_SINE_INTERP=0
```

---

## ライセンス

以下のオリジナルのライセンスに従います。

・本ソースの改変および改変物の公開、自作ソフト等への組み込みおよびそのソフトの
　配布は自由です。m_puusanへの報告は必要ありません。
・本ソフトウェアの使用または使用不能から生じるいかなる損害（利益やデータの損失、
　その他の金銭的損失など）に関してm_puusanは一切責任を負わないこととします。

---

## クレジット

### オリジナル作者
- m_puusan (X68Sound オリジナル開発者)

### コントリビューター
- GIMONS氏 (CSM音声合成モード実装)
- LinearDrive氏 (SRC/Low-passFilter無効化)
- honet氏 (C86CTL対応)
- GORRY氏 (ROMEOパッチ)
- うさ氏 (ROMEO-Athronパッチ)

### 2025年版改良
- UTF-8変換、Visual Studio 2022対応
- 環境変数サポート
- 音質向上機能（PCM8/ADPCM線形補間、OPM線形補間、ボリュームスムージング）
- ハイレゾ音源対応（96kHz/192kHz出力サンプリングレート）
- コード品質改善（飽和演算、定数化、コメント整理） 
