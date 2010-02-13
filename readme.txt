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
・動画の再生にはFFmpegのエンジン(libavcodec)を利用します。
・VSyncを基準クロックとしますが、単純に分周しているだけなので、動画ファイルは30fpsで
　準備してください。
・フレームを1枚1枚テクスチャとしてバッファするので、素材と素材の間も綺麗につなぐこと
　ができます。


■ 開発環境
OS: Windows7
IDE: Microsoft Visual C++ 2008 Express Edition
     FlashDevelop3.0.6RTM

■ 依存する外部ライブラリ
<ffmpeg>※MinGWやlibmp3lameなど含む
<POCO C++ Libraries>
<OpenSSL>※pocoにてSSL関連を扱う場合
<Microsoft DirectX 9.0 SDK (August 2009)>
<Windows Software Development Kit (SDK) for Windows 7 and .NET Framework 3.5 Service Pack 1>
<AdobeAIR1.5.3>※airSwitchの実行に必要
※DirectShowを使うためbaseclassesが必要
※コンパイルした環境によって、Microsoft Visual C++ 2008 再頒布可能パッケージ などが必要になります。
※UIの英文字フォントに http://rs125.org さんの defactica を使わせていただいています。

■ ビルド方法
のちほど。

■ リリースファイル一覧
[datas]             サンプルのワークスペース、コンテンツファイル
basic.fx            fxファイル
cursor.png          カーソルのイメージ
defactica.ttf       フォントファイル
switch.exe          実行ファイル
switch-config.dtd   基本設定ファイルのDTD
switch-config.xml   基本設定ファイル

airSvvitch.air      ユーザインターフェースのAIRアプリです。

■ 更新履歴
0.3
・試験的に音声トラックのサポート
・テロップ機能のサポート
・スケジュール機能のサポート
・USBストレージによるワークスペース更新機能

0.2
・AdobeAIR版オペレーション機能の追加
・送出画面側はUI機能の削除
0.1
・最初のリリース
