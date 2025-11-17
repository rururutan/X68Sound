# X68Sound 環境変数設定ガイド

## 概要

X68Sound DLLは、環境変数を使用してオーディオバッファとレイテンシ設定を調整できます。
これにより、音の途切れ（アンダーラン）を防ぎ、システム環境に応じた最適な設定が可能になります。

## 対応環境変数

### X68SOUND_PCM_BUFFER
PCMバッファサイズを設定します。

- **デフォルト値**: `5`
- **有効範囲**: `2` ～ `20`
- **説明**: バッファサイズが大きいほど音の途切れは少なくなりますが、レイテンシ（遅延）が増加します。
- **推奨設定**:
  - 低スペックPC: `8` ～ `12`
  - 標準PC: `5` ～ `7`
  - 高性能PC: `3` ～ `5`

**設定例**:
```batch
set X68SOUND_PCM_BUFFER=8
```

### X68SOUND_BETW_TIME
Between時間をミリ秒単位で設定します。

- **デフォルト値**: `5` ms
- **有効範囲**: `1` ～ `50` ms
- **説明**: タイマー割り込み間隔を制御します。値が小さいほど精度が上がりますが、CPU負荷が増加します。
- **推奨設定**: `5` ～ `10` ms

**設定例**:
```batch
set X68SOUND_BETW_TIME=10
```

### X68SOUND_LATE_TIME
レイテンシ時間をミリ秒単位で設定します。

- **デフォルト値**: `200` ms
- **有効範囲**: `50` ～ `1000` ms
- **説明**: オーディオ出力の遅延許容時間です。値が大きいほど音の途切れに強くなりますが、反応が遅くなります。
- **推奨設定**:
  - リアルタイム重視: `100` ～ `150` ms
  - 安定性重視: `200` ～ `400` ms

**設定例**:
```batch
set X68SOUND_LATE_TIME=300
```

### X68SOUND_REV_MARGIN
サンプルレート補正マージンを設定します。

- **デフォルト値**: `1.0`
- **有効範囲**: `0.1` ～ `10.0`
- **説明**: サンプルレートの微調整マージン。通常は変更不要です。
- **推奨設定**: `1.0` （デフォルトのまま）

**設定例**:
```batch
set X68SOUND_REV_MARGIN=1.5
```

### X68SOUND_BUF_MULTIPLIER
バッファサイズの乗数を設定します。

- **デフォルト値**: `1`
- **有効範囲**: `1` ～ `8`
- **説明**: PCM_BUFFERに乗算される値。大きな値を設定すると、より大きなバッファが確保されます。
- **推奨設定**:
  - 標準: `1`
  - 音の途切れが頻発する場合: `2` ～ `4`

**設定例**:
```batch
set X68SOUND_BUF_MULTIPLIER=2
```

### X68SOUND_DEBUG
デバッグログの有効/無効を設定します。

- **デフォルト値**: `0` （無効）
- **有効値**: `0` (無効) / `1` (有効)
- **説明**: 有効にすると、OutputDebugStringを通じて設定値とイベントログが出力されます。
- **用途**: トラブルシューティング、パラメータチューニング

**設定例**:
```batch
set X68SOUND_DEBUG=1
```

デバッグログの確認方法:
- **DebugView** (Sysinternals) を使用
- または、Visual Studioのデバッグ出力ウィンドウで確認

### X68SOUND_LINEAR_INTERPOLATION
線形補間による音質向上機能の有効/無効を設定します。

- **デフォルト値**: `1` （有効）
- **有効値**: `0` (無効) / `1` (有効)
- **説明**: サンプルレート変換時に線形補間を適用します。有効にすると、エイリアシングノイズが軽減され、滑らかな音質になります。
- **効果**:
  - エイリアシングノイズの大幅軽減
  - 高周波成分の歪みが減少
  - CPU負荷は約5%増加
- **推奨設定**: `1` （有効のまま）

**設定例**:
```batch
REM 線形補間を無効化（実機互換性重視の場合）
set X68SOUND_LINEAR_INTERPOLATION=0
```

### X68SOUND_VOLUME_SMOOTHING
ボリュームスムージング機能の有効/無効を設定します（PCM8のみ）。

- **デフォルト値**: `1` （有効）
- **有効値**: `0` (無効) / `1` (有効)
- **説明**: ボリューム変更時にクリックノイズを防ぐため、徐々にボリュームを変化させます。
- **効果**:
  - クリックノイズ完全除去
  - 滑らかなボリューム変化（約5ms）
  - CPU負荷は1%未満の増加
- **推奨設定**: `1` （有効のまま）

**設定例**:
```batch
REM ボリュームスムージングを無効化（即座のボリューム変更が必要な場合）
set X68SOUND_VOLUME_SMOOTHING=0
```

### X68SOUND_OPM_SINE_INTERP
OPM（FM音源）の正弦波テーブル線形補間機能の有効/無効を設定します。

- **デフォルト値**: `1` （有効）
- **有効値**: `0` (無効) / `1` (有効)
- **説明**: FM音源の正弦波生成時に線形補間を適用します。有効にすると、位相精度が向上し、エイリアシングノイズが軽減されます。
- **効果**:
  - エイリアシングノイズの大幅軽減（50～70%）
  - 高周波成分の歪みが減少
  - FM変調の滑らかさが向上
  - CPU負荷は約3～5%増加
- **推奨設定**: `1` （有効のまま）

**設定例**:
```batch
REM OPM線形補間を無効化（実機完全互換性重視の場合）
set X68SOUND_OPM_SINE_INTERP=0
```

---

## 使用例

### シナリオ1: 音が途切れる場合
```batch
REM バッファを増やして安定性を向上
set X68SOUND_PCM_BUFFER=10
set X68SOUND_LATE_TIME=300
set X68SOUND_BUF_MULTIPLIER=2
set X68SOUND_DEBUG=1

your_application.exe
```

### シナリオ2: レイテンシを最小化したい場合
```batch
REM 低レイテンシ設定（高性能PC向け）
set X68SOUND_PCM_BUFFER=3
set X68SOUND_LATE_TIME=100
set X68SOUND_BETW_TIME=5

your_application.exe
```

### シナリオ3: システムリソースが限られている場合
```batch
REM 低スペックPC向け設定
set X68SOUND_PCM_BUFFER=12
set X68SOUND_LATE_TIME=400
set X68SOUND_BETW_TIME=10
set X68SOUND_BUF_MULTIPLIER=3

your_application.exe
```

### シナリオ4: 最高音質設定（高性能PC向け）
```batch
REM すべての音質向上機能を有効化（デフォルト）
set X68SOUND_LINEAR_INTERPOLATION=1
set X68SOUND_VOLUME_SMOOTHING=1
set X68SOUND_OPM_SINE_INTERP=1
set X68SOUND_PCM_BUFFER=5
set X68SOUND_LATE_TIME=200

your_application.exe
```

### シナリオ5: 実機完全互換モード
```batch
REM すべての音質向上機能を無効化して実機と同じ動作に
set X68SOUND_LINEAR_INTERPOLATION=0
set X68SOUND_VOLUME_SMOOTHING=0
set X68SOUND_OPM_SINE_INTERP=0

your_application.exe
```

---

## トラブルシューティング

### 音が途切れる（アンダーラン）
**原因**: バッファサイズが小さすぎる、またはCPU負荷が高い

**対策**:
1. `X68SOUND_PCM_BUFFER` を増やす（例: `10`）
2. `X68SOUND_LATE_TIME` を増やす（例: `300` ～ `400`）
3. `X68SOUND_BUF_MULTIPLIER` を `2` 以上に設定
4. デバッグログを有効にして実際の設定値を確認

### レイテンシが大きすぎる
**原因**: バッファサイズが大きすぎる

**対策**:
1. `X68SOUND_PCM_BUFFER` を減らす（例: `3` ～ `5`）
2. `X68SOUND_LATE_TIME` を減らす（例: `100` ～ `150`）
3. 音の途切れが発生しないか確認しながら調整

### 設定が反映されない
**対策**:
1. 環境変数がアプリケーション起動前に設定されているか確認
2. `X68SOUND_DEBUG=1` を設定し、ログで実際の値を確認
3. アプリケーションを再起動

---

## 技術詳細

### 動作原理
1. DLLロード時（`DllMain`）に環境変数を読み取り
2. グローバルコンフィグ構造体（`g_Config`）に保存
3. `X68Sound_Start()` / `X68Sound_StartPcm()` 呼び出し時に設定を適用
4. 引数が明示的に指定された場合は引数を優先、デフォルト値の場合は環境変数を使用

### デバッグログの出力例
```
[X68Sound] Config loaded:
  PCM_BUFFER=8
  BETW_TIME=10 ms
  LATE_TIME=300 ms
  REV_MARGIN=1.00
  BUF_MULTIPLIER=2
  LINEAR_INTERPOLATION=1
  VOLUME_SMOOTHING=1
  OPM_SINE_INTERPOLATION=1
[X68Sound] DLL loaded successfully
[X68Sound] Start: samprate=44100, betw=10, pcmbuf=16, late=300, rev=1.00
```

---

## 注意事項

- 環境変数はDLLロード時に一度だけ読み込まれます
- 実行中に環境変数を変更しても反映されません（再起動が必要）
- 無効な値を設定した場合は自動的にデフォルト値またはクランプされた値が使用されます
- システム環境変数またはユーザー環境変数として設定することも可能です

---

## リファレンス

| 環境変数 | デフォルト | 範囲 | 用途 |
|---------|-----------|------|------|
| `X68SOUND_PCM_BUFFER` | 5 | 2-20 | PCMバッファサイズ |
| `X68SOUND_BETW_TIME` | 5 | 1-50 | Between時間（ms） |
| `X68SOUND_LATE_TIME` | 200 | 50-1000 | レイテンシ（ms） |
| `X68SOUND_REV_MARGIN` | 1.0 | 0.1-10.0 | サンプルレート補正 |
| `X68SOUND_BUF_MULTIPLIER` | 1 | 1-8 | バッファ乗数 |
| `X68SOUND_DEBUG` | 0 | 0/1 | デバッグログ |
| `X68SOUND_LINEAR_INTERPOLATION` | 1 | 0/1 | PCM8/ADPCM線形補間 |
| `X68SOUND_VOLUME_SMOOTHING` | 1 | 0/1 | PCM8ボリュームスムージング |
| `X68SOUND_OPM_SINE_INTERP` | 1 | 0/1 | OPM正弦波線形補間 |

---

**作成日**: 2025-01-17
**最終更新日**: 2025-11-17
**バージョン**: 2.0
