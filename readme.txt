switch
================================================================================

■ 概要
「switch」は、リアルタイムに映像を使った演出を行うための送出アプリケーションです。
PC上で作成できる動画ファイル、静止画ファイルの送出の他、テキストファイルをテロップ
合成する機能、素材の切替でトランジションをかける機能を備えています。

放送用の機材は比較的高価で、素人が手が出せないものもあります。このプロジェクトでは、
イベント用途などで比較的簡単に扱えるPCベースのアプリケーションとして送出・編成がで
きるアプリケーションを目指します。

「switch」の特徴は次のとおりです。
・DirectXを使ったWindowsアプリケーションです。
・動画の再生にはffmpegのエンジン(libavcodec)を利用します。
・VSyncに同期させるために、動画ファイルについては30fpsである必要があります。
・フレームを1枚1枚テクスチャとしてバッファするので、素材と素材の間も綺麗につなぐこと
　ができます。


■ 開発環境
OS: Windows XP SP3
IDE: Microsoft Visual C++ 2008 Express Edition


■ 依存する外部ライブラリ
<ffmpeg>※MinGWやlibmp3lameなど含む
<POCO C++ Libraries>
<OpenSSL>※pocoにてSSL関連を扱う場合
<Microsoft DirectX 9.0 SDK (December 2004)>
<Windows Software Development Kit (SDK) for Windows Server 2008 and .NET Framework 3.5>
※DirectShowを使うためbaseclassesが必要
※コンパイルした環境によって、Microsoft Visual C++ 2008 再頒布可能パッケージ などが必要になります。

■ ビルド方法
のちほど。

■ リリースファイル一覧
[samples]           サンプルのコンテンツファイル
basic.fx            fxファイル
cursor.png          カーソルのイメージ
switch.exe          実行ファイル
switch-config.dtd   基本設定ファイルのDTD
switch-config.xml   基本設定ファイル
workspace.xml       コンテンツ定義ファイル
