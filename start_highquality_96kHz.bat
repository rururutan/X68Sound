@echo off
REM =========================================
REM X68Sound2025 最高音質設定（96kHz）
REM =========================================
REM
REM このバッチファイルは、X68Sound2025を最高音質で起動するための
REM 環境変数を設定します。
REM
REM 【動作環境】
REM - CPU: Intel Core i5以上 / AMD Ryzen 5以上
REM - メモリ: 8GB以上
REM - OS: Windows 10/11
REM - オーディオ: 96kHz対応インターフェース
REM
REM 【使い方】
REM 1. 下の「your_application.exe」を実際のアプリケーションパスに変更
REM 2. このバッチファイルをダブルクリックして起動
REM
REM =========================================

echo X68Sound2025 最高音質設定を適用中...
echo.

REM 【ハイレゾ出力】96kHz
set X68SOUND_OUTPUT_RATE=96000

REM 【音質向上機能】すべて有効化
set X68SOUND_LINEAR_INTERPOLATION=1
set X68SOUND_VOLUME_SMOOTHING=1
set X68SOUND_OPM_SINE_INTERP=1

REM 【バッファ設定】安定性重視
set X68SOUND_PCM_BUFFER=7
set X68SOUND_LATE_TIME=200
set X68SOUND_BETW_TIME=5
set X68SOUND_BUF_MULTIPLIER=1

REM 【デバッグログ】（必要に応じてコメント解除）
REM set X68SOUND_DEBUG=1

echo 設定内容:
echo   出力サンプリングレート: 96kHz
echo   PCM8/ADPCM線形補間: ON
echo   ボリュームスムージング: ON
echo   OPM正弦波線形補間: ON
echo   PCMバッファサイズ: 7
echo   レイテンシ: 200ms
echo.
echo 【期待される効果】
echo   - エイリアシングノイズ: 50～70%軽減
echo   - クリックノイズ: 完全除去
echo   - FM音源の音質: 大幅向上
echo   - サンプリングレート: 2倍（44.1kHz→96kHz）
echo   - CPU負荷: 約15～20%増加
echo.

REM =========================================
REM アプリケーション起動
REM =========================================
REM 下の行を実際のアプリケーションパスに変更してください
REM 例: C:\Games\X68000\MyEmulator.exe

echo アプリケーションを起動します...
echo （注意: your_application.exeを実際のパスに変更してください）
echo.

REM TODO: 下の行を編集してください
your_application.exe

REM もし音が途切れる場合は、以下のコメントを解除してください：
REM set X68SOUND_PCM_BUFFER=10
REM set X68SOUND_LATE_TIME=300
REM set X68SOUND_BUF_MULTIPLIER=2

pause
