switch
================================================================================

■ 概要
「switch」は、リアルタイムに映像を使った演出を行うための送出アプリケーションです。
PC上で作成できる動画ファイル、静止画ファイルの送出の他、テキストをテロップ合成す
る機能、素材の切替で簡単なトランジションをかける機能を備えています。

「switch」の特徴は次のとおりです。

・DirectX9を使ったWindowsアプリケーションです。
・動画の再生にはffmpegのエンジン(libavcodec)又はDirectShowを利用します。
・AdobeAIR製の運営アプリケーション(サンプル)がありますので、セットアップできれば
  一応すぐ運営ができます。
・ビデオキャプチャのデバイスがあればライブ入力することも可能です。
・送出内容を折り返して表示したりすることができ、長尺のLEDなどへの送出素材として
  利用できます。 


■ 開発環境
OS: WindowsXP SP3/Windows7 Professional 
VIDEO: ATI Radeon HD 4670/ATI Radeon HD5750 
IDE: Microsoft Visual C++ 2008 Professional Edition


■ 依存する外部ライブラリ
<ffmpeg>※MinGWやlibmp3lameなど含む
<POCO C++ Libraries>
<OpenSSL>※pocoにてSSL関連を扱う場合
<Microsoft DirectX 9.0 SDK (December 2004)>
<Windows Software Development Kit (SDK) for Windows Server 2008 and .NET Framework 3.5>
※DirectShowを使うためbaseclassesが必要
※コンパイルした環境によって、Microsoft Visual C++ 2008 再頒布可能パッケージ などが必要になります。

■ リリースファイル一覧
[datas]                サンプルワークスペースデータ
[fx]                   エフェクトファイル(conversion_yuv2rgb.fx)
[images]               外部参照イメージファイル
[fonts]                フォントファイル(defactica.ttf copyright(c)2004 RS125)
airSvvitchSetup.air    AdobeAIR2.7用運営アプリケーションインストーラ
switch.exe             実行ファイル
switch-config.xml      基本設定ファイル
workspace.xml          コンテンツ定義ファイル


■ サンプル実行方法
リリースパッケージを解凍してできたディレクトリの中にある、switch.exeを実行してください。
左上に黒いウィンドウが表示され、やがて動画が表示されればテストOKです。
表示されずにすぐプロセスが終了してしまったり、ダイアログが表示される場合は、
Visual C++ 2008再配布パッケージ等をインストールする必要があります。

最新の詳細情報は以下を参照してください。

http://sourceforge.jp/projects/switch/wiki/FrontPage


■ 更新履歴
1.10
・avcodecをFFmpeg-b616600516a0b46c365ee4fbd65167d6d276a9ad(11/10/24あたり)にアップデート
・音声付動画を再生時に途中で切替えると音声が残ってしまうのを修正
・コーデックが選択できたにも関わらずpix_formatが設定できない場合に落ちてしまうのを修正

1.01
・TextContentの不具合の修正
・24p/25p/60pの実験的なサポート
・その他、細かいバグフィックス。

1.0
・スケジュール 'playlist' 'next'コマンドの追加
・Dissolve時などに斜めに掛かってしまっていたのを修正
・静止画にJPEG形式も使用できるように修正
・ファイルコピーのWebAPIを修正し、ストックしてから一気に入替えるコマンドを追加したので
  古いairSwitchとの互換性が無くなりました
・倍精度の精度による時計ずれの修正
・airSwitchのメディア編集にて入力チェックを追加
・60p素材の実験的なサポート
・その他、細かいバグフィックス。

0.9f
・Flash(10.1)の対応
・airSwitchを複数個所で起動していた場合の更新通知機能を追加
・その他、airSvvitchにて細かいバグフィックス。

0.9d
・同期時に親ディレクトリが無い場合に同期できないのを修正

0.9c
・ディスプレイの状態だけを取得するWebAPIを追加
・ディスプレイ選択の表にディスプレイとの接続状況を表示する欄を追加
・プレイリストとメディアリストに追加したときに最後に追加されフォーカスが移動するように修正
・その他、airSvvitchにて細かいバグフィックス。

0.9b
・airSvvitchのsync機能のステータス表示を追加

0.9a
・airSvvitchのプラットフォームをAIR2.0に更新
・sync機能が動作しないのを修正
・ショートカット時にプレイリストの内容が更新されないのを修正
・追加したタイムラインのタグ名が誤っていたのを修正

0.9
・airSvvitchにて細かいバグフィックス。
・フォントファイルの管理ができるように修正。
・フォントファイルをfontsフォルダに設置するよう変更。

0.8
・airSvvitchにタイムライン機能追加
・送出の情報表示のレイアウトを変更

0.7
・airSvvitchにて、細かいバグフィックス。
・送出の方に同期処理と遅延更新ファイルの状況を表示するように追加


0.6
・細かいバグフィックス
・使用中のファイルを上書きできるようになってからコピーする機能追加
・airSvvitchのUIレイアウト変更
・リモート同期機能の追加

0.5
・細かいバグフィックス
・プレビュー画像のメモリ化(ファイルを作らなくした)
・動画の音声の再生タイミングの変更
・キャプチャ機能にステージをキャプチャする機能を追加
・エフェクトファイルをfxフォルダに移動
・ログファイルを出力しないモードを追加(log.fileを<file />にする)
・素材無しメディアが再生されても落ちないように修正
・USBメモリなどのデバイスの取り外しを検出するように追加
・airSwitchでは、IPアドレスを127.0.0.xにするとローカルモードで動作します

0.4c
・スケジュールによるコンテンツ切替時のリーク分を対処
・プレビュー用の画像サイズを縮小化
・airSwitchのAdobeAIRを2.0beta→1.5に戻しました

0.4b
・AMDのデュアルコアCPUで時計がずれるのを修正
・FFmpeg再生時のスレッド構成を3スレッド->2スレッドに修正
・音声再生処理でリングバッファを使っていますが、循環して音が再生されてしまうのをなんとなく修正

0.4a
・リムーバブルディスクを接続したときに45秒待つように修正
・AMDのデュアルコアCPUで時間測定系の数値がおかしいのを修正
・0.4にて、新しいDirectX SDKでコンパイルされていたので、今回はDecember 2004でコンパイルしています。

0.4
・コンテンツの終了待ち処理に不具合がありフリッカ状態になってしまうのを修正
・リムーバブルディスクによるインポート時に落ちることがあるのを修正
・シェーダの関数名をそれっぽく修正0.3
・試験的に音声トラックのサポート
・テロップ機能のサポート
・スケジュール機能のサポート
・USBストレージによるワークスペース更新機能

0.2
・AdobeAIR版オペレーション機能の追加
・送出画面側はUI機能の削除
0.1
・最初のリリース
