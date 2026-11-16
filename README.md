# C++17 カメラ制御アーキテクチャ・ダミーサンプル

通信部と実SDKの実装を除いた、**クラスの役割とスレッドの所有関係を読むためのサンプル**です。実運用向けではなく、停止処理などは意図的に省略しています。

## ファイル構成

- `include/Types.hpp` — 要求・イベント・アクションの型
- `include/ThreadQueue.hpp` — スレッド間キュー
- `include/CommandHandlers.hpp`, `src/CommandHandlers.cpp` — A/B/C/Dの共通ワークフロー
- `include/Coordinator.hpp`, `src/Coordinator.cpp` — Handler生成、イベント配信、応答ルート
- `include/CameraScheduler.hpp` — 制御/受信ワーカーへの振り分け
- `include/Workers.hpp`, `src/Workers.cpp` — カメラごとに制御スレッドと受信スレッドを各1本
- `include/DummySdk.hpp` — 制御用・受信用のダミーSDKハンドル
- `include/CameraModel.hpp`, `src/CameraModel.cpp` — 機種別StrategyとRegisterMap
- `src/main.cpp` — 通信の代わりに要求投入

## スレッドとSDKハンドル

```text
通信A/B相当 → Coordinator（イベント処理・Handlerの状態変更はここだけ）
                    ├─ Camera 1 ─ ControlWorker thread ─ ControlSdkHandle ─ ModelAStrategy
                    │           └ ReceiveWorker thread ─ ReceiveSdkHandle
                    └─ Camera 2 ─ ControlWorker thread ─ ControlSdkHandle ─ ModelBStrategy
                                └ ReceiveWorker thread ─ ReceiveSdkHandle
```

制御SDKハンドルは**制御スレッド内で生成して使用**、受信SDKハンドルは**受信スレッド内で生成して使用**。HandlerはSDKを直接呼びません。Strategyは制御用SDKの操作内容（機種ごとのAPI選択、レジスタ、呼び出し順）を決めます。

## コマンドの流れ

- **A**: Handler → ApplyConfig → ControlWorker → ControlDone → 応答
- **B**: ApplyConfig → StartCapture → StartReceive → ImageArrived → 応答。受信待ち中にControlWorkerをブロックしない
- **C**: 進行中のBをCancelledとして応答 → ReceiveWorkerへStopReceive → ReceiveStoppedでCに応答
- **D**: 開始時はWriteAcceptedで即応答し、書き込み状態はワーカーに残る。後のD照会はWriteStatusで応答

## 意図的な簡略化・製品化時に必要なこと

1. **重要: スレッド終了処理は未実装**。現在のWorkerデストラクタの`detach()`は説明用の仮置きで、実行終了時の安全性を保証しません。実装では終了メッセージ、`join()`、SDKハンドル破棄順を必ず実装してください。
2. **同一カメラのコマンド直列化は未実装**。Schedulerにカメラ単位の保留キューと許可条件を設けます。ただしCはB受信待ちに割り込めるようにします。
3. **画像受信SDKのコールバック連携は未実装**。実際には受信SDKのコールバックで受信スレッドへ通知し、そこからCoordinatorへ`ImageArrived`を送ります。`postImage()`は疑似イベント入口です。
4. **受信開始・停止の競合**、Bの遅延イベント、C実行時の停止完了前に次のBを開始しない制御が必要です。
5. **Dの進捗**はダミーでWritingのままです。実際にはSDKの書き込み状態照会結果で更新し、WriteIdなどで対象書き込みを識別します。開始/照会は本来リクエストの明示的なフィールドで区別します（ここではrequestId偶奇で仮置き）。
6. タイムアウト、SDK失敗、通信切断、レスポンスの送信スレッド、画像バッファ所有権、SDKのスレッドセーフ性は省略しています。
7. Model Strategyの`Recipe`は**ログ文字列のみ**。実SDKでは制御スレッド上で共通SDK Driverに対してAPIを呼び分けます。

**注意:** 構成把握のための擬似的なコードです。ビルド・動作確認済みの完成品として扱わないでください。
