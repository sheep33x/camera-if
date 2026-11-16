# C++17 複数カメラ制御サンプル

## フォルダ構成

```text
camera-if/
├── CMakeLists.txt
├── README.md
├── include/                ヘッダー
├── src/                    実装とサンプルのmain.cpp
├── tests/                  camera_tests.cpp
└── archives/               既存の設計サンプルZIP
```

## 構成と責務

Coordinatorを実行する1スレッドが、すべてのCameraActorをイベント駆動で実行する。
Actor自身にはスレッドを持たせない。addCamera / runOnceは同じスレッドから呼ぶ。
通信側はpostRequest、Worker側はpostEventでスレッドセーフなキューへ投入する。
Request.idは遅延イベントとの混同を避けるため、プロセス内で再利用しないこと。

- SystemWorker: 全体に1本。システムSDKハンドルを生成・使用・破棄する。
- DeviceWorker: カメラごとに1本。デバイスSDKとModelStrategyを使用する。
- StreamWorker: カメラごとに1本。受信SDKを使用する。
- CameraActor: カメラ状態、現在のProcedure、要求待ち行列、WriteSessionを所有する。
- CameraActorはSystemWorker・DeviceWorker・StreamWorkerへの参照を持ち、各Workerのpost()を直接呼ぶ。

ProcedureはActorが所有する進行状態。状態遷移はsrc/CameraActor.cppにまとめ、各段階で担当WorkerのキューへActionを投入する。
Workerには手続き判断やクライアント要求をまたぐ業務状態を持たせない。
SDKハンドルはWorkerのローカル変数とし、生成から破棄まで所有スレッドで実行する。
DummySdkはスレッド所有をassertで確認する。Workerはキューを閉じ、joinして終了する。
CoordinatorはActorを破棄してからWorkerを破棄する。受信キューはすべてのWorkerより長く生存する。

## 手続き

- A: System準備 → 設定 → 応答。
- B: System準備 → 設定 → 撮影開始 → 受信開始 → 画像 → 受信停止完了 → 応答。
- C: 実行中Bにcancelledを応答し、受信停止完了を待って応答。B以外の実行中要求には割り込まず待機する。
- D Start: 明示的なsessionIdで開始し、受付完了に応答。SessionはActorに残る。
- D Query: 同じsessionIdで状態を照会。要求IDや応答先は開始要求と異なってよい。

通常要求はカメラごとに直列化する。異なるカメラは独立して進む。
Cで中断されたBの遅延イベントは要求IDで除外する。
停止完了より前に次のProcedureを開始しない。
書き込み中のSessionがある場合、新規D Startを拒否する。
サンプルではカメラごとに最新のSessionを1つ保持し、完了後の次回開始で置き換える。

## ビルドと検証

CMakeとC++17コンパイラが必要。

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

camera_sampleは2カメラに5要求を投入し、5応答を受けて終了する。
camera_testsでは実Workerの完了イベントを順にActorへ渡し、手続きの直列化、C割り込み、停止待ち、遅延イベント、カメラの取り違え、
Sessionの開始・照会・競合、失敗応答、複数カメラ、Worker破棄を確認する。

## 実SDK統合時に必要な作業

現在は構成を検証するダミーであり、実SDKや通信処理は含まない。
System準備はダミー操作で、実SDKの列挙・初期化・終了の具体的な寿命設計は別途必要。
受信は非ブロッキングのダミーで、開始直後に画像イベントを1件発行する。
ブロッキング受信待ちと別スレッドからの待ち解除はまだ実装していない。
実SDKでは待ち解除専用の経路だけを例外として公開し、通常のSDK操作はStreamWorkerで行うこと。
Dは次の照会でCompleteを返すダミーで、実際の進捗はSDK結果をEventに変換する。
タイムアウト、SDK例外変換、失敗時の撮影・受信の復旧、終了時の未完了要求への応答、
画像バッファ所有権、Sessionの永続化は未実装。
stopはイベント受付を閉じるための終了APIであり、すべての手続きの完了を保証するAPIではない。
