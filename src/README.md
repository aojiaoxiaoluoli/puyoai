src/
======

ぷよぷよAIのソースコード

# 各ディレクトリの説明

* base このプロジェクト専用ではないような関数の定義など。
* core 定数や、ぷよの色の定義など、全員が利用するべきもの。
 * core/algorithm AI を実装するときにあると便利なアルゴリズムたち。サーバー実装でも利用。
 * core/client クライアントが利用すると便利なもの。
   * core/client/ai AIのベース。ここにあるクラスを継承してthink()だけ実装すれば、とりあえず動く。
   * core/client/connector サーバーと接続するときに使うと便利なクラス。
 * core/field フィールドの実装。gui とかはこれを使っている。
 * core/server サーバー実装に必要なもの
   * core/srever/connector クライアントとの通信に使うと便利なクラス。
* capture キャプチャー関連。画面解析など。
* cpu みんなの AI 実装。ある意味一番重要なコード類。
* duel ローカルでの対戦サーバー
* gui GUI関連。対戦サーバーやwiiの実装で使う。
* solver AIを作成してテストするときに便利なもの。とこぷよルーチンなど。
* third_party 第三者ライブラリをそのまま持ってきたもの。
* wii Wii実機と接続して対戦するサーバー。
