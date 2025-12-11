
# X68Sound

m_puusan氏作の[X68Sound](http://mpuusan.web.fc2.com/x68sound/x68soundbody.htm)をパッチの纏めと修正を行ったレポジトリです。


## Change History

2019/11/16

* KF,KC,DT2の音程に上限値を設定(TAPPY氏提供)

2016/09/12

* CSM音声合成モードの実装(GIMONS氏提供)

2013/08/03

* SRC/Low-passFilterを無効化する(LinearDrive氏提供)
* 44.1KHz/48KHzでは過去互換を取って従来通り
* 22KHzでチャンネルマスクが効いていなかったのを修正。
* LFOの修正をVOPMから持ってきた。

2013/02/12

* C86CTLへの対応を追加。(honet氏提供)
* PCM8モード時のDMAチェイン動作を修正。

2013/01/09

* x64バイナリのビルドを可能にした。pcidebug.dll自体がx64非対応の為、ROMEOサポートを外しています。

2011/01/19

* チャンネルマスク機能(X68Sound_SetMask)を追加。

2003/09/26

* 22/44.1/48KHz以外の出力許す様に修正。
* [ROMEOパッチ](https://gorry.haun.org/mx/) (GORRY氏)/[ROMEO-Athronパッチ](http://retropc.net/usalin/) (うさ氏)のコードをマージ


## Build

> cmake -S . -B build -A win32

or 

> cmake -S . -B build -A x64

> cmake --build build --config Release


## License

以下のオリジナルのライセンスに従います。

・本ソースの改変および改変物の公開、自作ソフト等への組み込みおよびそのソフトの
　配布は自由です。m_puusanへの報告は必要ありません。  
・本ソフトウェアの使用または使用不能から生じるいかなる損害（利益やデータの損失、
　その他の金銭的損失など）に関してm_puusanは一切責任を負わないこととします。 
