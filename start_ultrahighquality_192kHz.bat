@echo off
REM =========================================
REM X68Sound2025 超高音質設定（192kHz）
REM =========================================
REM
REM このバッチファイルは、X68Sound2025を超高音質で起動するための
REM 環境変数を設定します。
REM
REM 【注意】192kHzは非常に高いCPU負荷がかかります！
REM
REM 【動作環境】
REM - CPU: Intel Core i7以上 / AMD Ryzen 7以上（高性能必須）
REM - メモリ: 16GB以上
REM - OS: Windows 10/11（最新アップデート推奨）
REM - オーディオ: 192kHz対応ハイエンドインターフェース
REM
REM 【使い方】
REM 1. 下の「your_application.exe」を実際のアプリケーションパスに変更
REM 2. このバッチファイルをダブルクリックして起動
REM
REM =========================================

echo X68Sound2025 超高音質設定（192kHz）を適用中...
echo.
echo 【警告】192kHzモードはCPU負荷が非常に高くなります！
echo         音が途切れる場合は96kHz版の使用を推奨します。
echo.

REM 【ハイレゾ出力】192kHz
set X68SOUND_OUTPUT_RATE=192000

REM 【音質向上機能】すべて有効化
set X68SOUND_LINEAR_INTERPOLATION=1
set X68SOUND_VOLUME_SMOOTHING=1
set X68SOUND_OPM_SINE_INTERP=1

REM 【バッファ設定】安定性最優先
set X68SOUND_PCM_BUFFER=10
set X68SOUND_LATE_TIME=300
set X68SOUND_BETW_TIME=5
set X68SOUND_BUF_MULTIPLIER=2

REM 【デバッグログ】（必要に応じてコメント解除）
REM set X68SOUND_DEBUG=1

echo 設定内容:
echo   出力サンプリングレート: 192kHz（最高品質）
echo   PCM8/ADPCM線形補間: ON
echo   ボリュームスムージング: ON
echo   OPM正弦波線形補間: ON
echo   PCMバッファサイズ: 10
echo   レイテンシ: 300ms
echo   バッファ乗数: 2
echo.
echo 【期待される効果】
echo   - エイリアシングノイズ: 最大限軽減
echo   - クリックノイズ: 完全除去
echo   - FM音源の音質: 最高品質
echo   - サンプリングレート: 4倍（44.1kHz→192kHz）
echo   - CPU負荷: 約4倍（400%）
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

REM もし音が途切れる場合は、96kHz版の使用を推奨します：
REM start_highquality_96kHz.bat

pause
