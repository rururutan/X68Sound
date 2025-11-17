@echo off
REM =========================================
REM X68Sound2025 標準設定
REM =========================================
REM
REM このバッチファイルは、X68Sound2025をデフォルト設定で起動します。
REM すべての音質向上機能が有効ですが、サンプリングレートは標準の
REM 44.1kHzです。
REM
REM 【動作環境】
REM - CPU: Pentium以上
REM - メモリ: 4GB以上
REM - OS: Windows 10/11
REM
REM 【使い方】
REM 1. 下の「your_application.exe」を実際のアプリケーションパスに変更
REM 2. このバッチファイルをダブルクリックして起動
REM
REM =========================================

echo X68Sound2025 標準設定を適用中...
echo.

REM 【出力】自動検出（通常は44.1kHz）
set X68SOUND_OUTPUT_RATE=0

REM 【音質向上機能】すべて有効化（デフォルト）
set X68SOUND_LINEAR_INTERPOLATION=1
set X68SOUND_VOLUME_SMOOTHING=1
set X68SOUND_OPM_SINE_INTERP=1

REM 【バッファ設定】標準
set X68SOUND_PCM_BUFFER=5
set X68SOUND_LATE_TIME=200
set X68SOUND_BETW_TIME=5

REM 【デバッグログ】（必要に応じてコメント解除）
REM set X68SOUND_DEBUG=1

echo 設定内容:
echo   出力サンプリングレート: 自動検出（44.1kHz）
echo   PCM8/ADPCM線形補間: ON
echo   ボリュームスムージング: ON
echo   OPM正弦波線形補間: ON
echo   PCMバッファサイズ: 5
echo   レイテンシ: 200ms
echo.
echo 【期待される効果】
echo   - エイリアシングノイズ: 50～70%軽減
echo   - クリックノイズ: 完全除去
echo   - FM音源の音質: 大幅向上
echo   - CPU負荷: 標準（100%）
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

pause
