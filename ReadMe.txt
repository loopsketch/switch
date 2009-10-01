casterTest
================================================================================

■概要
casterTestは、C++のライブラリであるPocoとffmpegの実験のために作られた動画ファイル
プレイヤです。一応、VJプレイに耐えうるような動画プレイヤを目指してはいますので、
将来的にはエフェクト効果なども実装していきたいと思います。

次のような特徴があります。

・Direct3D上で再生するのでDirectX9が必要です。d3dx9_35.dllを参照しているようです。
・ffmpeg(LGPL分のみ)によるデコードに対応。現在音声は未対応です。
・Vsyncに同期し、更に強制的に30fpsだと思って再生しますので、
  ソースが30fpsで作られていないとずれが生じますので、予めご了承ください。
・30フレーム程バッファリングして再生しますので、プレイリストに組んだ複数のコンテ
  ンツを綺麗につなぐことができます。


■基本設定
casterTest.exeと同じディレクトリにあるconfig.xmlを参照します。XMLファイルは
encoding="UTF-8"で作成してください。

<config>タグ
  ルートタグです。

<display>タグ
  ディスプレイ関連の設定を行います。次の属性が設定できます。
  width		ウィンドウ又は全画面時の横幅
  height	ウィンドウ又は全画面時の高さ
  fullscreen	全画面モードで実行するかどうかをtrue/falseで指定します。

<workspace>タグ
  テキストノードにワークスペースファイル名を記述します。

■ワークスペース設定
プレイリストを構成するための設定ファイルです。config.xmlで指定されたXMLファイルを
参照します。config.xmlと同様にencoding="UTF-8"で作成してください。

<workspace>
  ルートタグです。

<playlist>
  プレイリストを構成します。次の属性が設定可能です。
  name		プレイリスト名
  loops		プレイリストのループ数。-1を指定すると永久ループです。

<content>
  <playlist>タグの下に記述し、プレイリストのコンテンツを構成します。ファイル名を
テキストノードで指定します。次の属性が設定可能です。
  loops		コンテンツのループ数。-1を指定すると永久ループとなり、
		以降のcontentは無視されます。


■操作
[↑][↓]	プレイリストを変更します。
[SPACE]		プレイリストを再生します。
マウス		再生中の動画をドラックできます。動画以外の場所でも相対的に移動します。
		右クリックすると動画が0,0の位置に戻ります。

■依存ライブラリ
次のライブラリをリンクしています。

DirectX9.0c用ライブラリ"Luna"
http://www.twin-tail.jp/
※但し、サポート終了していますので組み込んでいます。
配布しているライブラリはDirectX SDK(August2007)にてリンクしたものですので、他のSDKを使う場合は
リビルドする必要があるようです。

POCO C++ Libraries
http://pocoproject.org

DirectX SDK
Lunaのインクルードファイルの関係からAugust2007を使用してください。

FFMPEG
http://www.ffmpeg.org/

■ビルド方法
1)OpenSSL
POCO C++ LibrariesのNetSSLで必要になります。

2)POCO C++ Librariesをビルド
Foundation、XML、Net、NetSSL、Utilのrelease_staticをビルドします。
その際、コード生成のランタイムライブラリにて、マルチスレッドDLL(/MD)となっているのを、
マルチスレッド(/MT)に変更すると、あとでリンクするときにうまくいくみたいです。

3)ffmpeg
MMX/SSE最適化無し
$ ./configure --enable-static --disable-shared --disable-mmx --disable-sse --enable-libmp3lame --enable-avisynth --enable-w32threads
 --extra-cflags="-I/local/include" --extra-ldflags="-static -L/local/lib" --disable-debug

MMX/SSE最適化有りっぽい
$ ./configure --enable-static --disable-shared --enable-memalign-hack --enable-libmp3lame --enable-avisynth --enable-w32threads
 --extra-cflags="-I/local/include -fno-tree-ch" --extra-ldflags="-static -L/local/lib" --disable-debug

■更新履歴
v0.2
ffmpegによるデコードに対応。

v0.1
とりあえずリリース。
