# instrument 側 SPI 修正対応まとめ

`instrument_spi_fix_notes.md` の指摘事項と、サーバー側 (`server.ino` / `spi_master.ino`)
が実際に採用した「ack_ok と checksum のみ delay(1) を挟んで再要求する」プロトコル変更を
踏まえ、楽器Arduino側 (`spi_slave.ino`, `instrument.ino`) に加えた修正の意図をまとめる。

使用マイコンは サーバー側・楽器側ともに **Arduino UNO R4 WiFi (Renesas RA4M1, Arm
Cortex-M4)** であり、Arduino核心の `SPI` ライブラリはマスター動作のみをサポートする
（ハードウェアSPIスレーブは提供されない）。そのため、楽器Arduino側は
`digitalRead()`/`digitalWrite()` によるソフトウェアSPIスレーブを実装している。

## 前提として確定したプロトコル仕様

`server.ino` の `spi_send()` は、1回のCS LOW区間内で次の順に合計7バイトのクロックを出す
（CS回数は1回のまま、通信回数自体は増やさない設計）。

```
1バイト目: ControlCommand.command_type 送信  ←→ InstrumentStatus.instrument_id 受信
2バイト目: ControlCommand.payload(下位)送信   ←→ InstrumentStatus.frog_state   受信
3バイト目: ControlCommand.payload(上位)送信   ←→ InstrumentStatus.sequence_ack 受信
4バイト目: ControlCommand.sequence    送信   ←→ InstrumentStatus.ack_ok(暫定) 受信
5バイト目: ControlCommand.checksum    送信   ←→ InstrumentStatus.checksum(暫定)受信
--- delay(1) ここでマスターはクロックを止める ---
6バイト目: ダミー(DUMMY_ACK)送信              ←→ InstrumentStatus.ack_ok(確定) 受信
7バイト目: ダミー(DUMMY_SUM)送信              ←→ InstrumentStatus.checksum(確定)受信
```

### sequence_ack が「今回の sequence」を返せない理由

`InstrumentStatus.sequence_ack` は3バイト目の交換タイミングで送信される。この時点で
マスターから送られてくるのは `ControlCommand.payload` の上位バイト（3バイト目）であり、
`ControlCommand.sequence`（4バイト目）はまだ受信できていない。つまり **同一トランザクション
内で「今回受信する sequence」をそのまま sequence_ack に載せることは、バイト順の制約上
不可能** である。

一方 `ack_ok` と `checksum` は5バイト目（checksum）受信後の `delay(1)` の間に検証を完了
させ、6,7バイト目で確定値を返すことができる。これがサーバー側が「ack_ok と checksum のみ
再要求する」設計を採った理由である。

この制約を受けて、楽器Arduino側は次の方針で実装した。

- `sequence_ack` は **直前に正常受信できた ControlCommand の sequence**（前回値）を返す。
  `server.ino` の `verification_status()` 側の比較ロジック（`status.sequence_ack !=
  cmd.sequence` で比較）は変更しないため、エラー検出が1トランザクション遅れる。
  3回連続エラーでフェイルセーフに入る設計のため、実害は小さいと判断した。
- `ack_ok` と `checksum` は、`delay(1)` の間に確定したチェックサム検証結果をそのまま
  6,7バイト目で返す。これにより「今回送られてきたコマンドが正しく検証できたか」は
  同一トランザクション内で正確に伝わる。

## ファイルごとの修正意図

### spi_slave.ino

| 指摘 (instrument_spi_fix_notes.md) | 対応 | 実装箇所 |
|---|---|---|
| 1. MISOを非選択時ハイインピーダンス化 | `miso_drive_enable()`/`miso_drive_disable()` で通信中のみ`OUTPUT`にし、開始前・終了後・タイムアウト時は必ず`INPUT`に戻す | `on_cs_falling()` の先頭と全ての`return`直前、`spi_setup()` |
| 2. ACKが1トランザクション遅れる問題 | 上記「前提プロトコル」の通り、`sequence_ack`は前回値を返す設計に統一。`ack_ok`/`checksum`は`delay(1)`後の追加2バイトで今回の検証結果を確定して返す | `prepare_tx_buffer_head()`, `finalize_tx_buffer_tail()`, `on_cs_falling()`内の7バイト処理 |
| 3. 1MHzソフトウェアSPIの見直し | ハードウェアSPI Slave化はUNO R4 WiFiでは不可能なため対象外。代わりにバイト単位のタイムアウト(`SCK_EDGE_TIMEOUT_US`)を設け、クロック停止時にハングしないようにした。クロックを下げて検証する余地をコメントで明記 | `spi_transfer_byte_slave()`, 定数`SCK_EDGE_TIMEOUT_US` |
| 4. 割り込み内の重い処理を減らす | `score_init()`/`score_stop_all()`などの実処理を呼ばず、`pending_command_type`等のフラグに記録するだけにとどめる。実処理は`process_pending_command()`として`loop()`側に切り出した | `finalize_tx_buffer_tail()`(フラグ立てのみ), `process_pending_command()`(実処理) |
| 5. instrument_idとCS配線の確認 | コード修正ではなく配線・EEPROM書き込みの確認作業のため、本ファイルでは対応不要（`instrument_id.ino`側で既存実装） | - |

追加で、当初の実装案にあった「1バイト目をダミー(0x00)で返す」バグを修正し、
1バイト目交換時から`spi_tx_buffer[0]`(`instrument_id`)を正しく返すようにした。

### instrument.ino

- `spi_slave.ino`に切り出した`process_pending_command()`を`loop()`の先頭で呼び出すよう
  追加した。これにより、SPI割り込み(`on_cs_falling`)で記録された保留コマンドが、毎周期
  確実に実行される。
- それ以外の既存ロジック（圧力センサ読み取り、`score_step`、`score_loop_check`の呼び出し）
  はアップロードされた最新版から変更していない。

### sync_isr.ino

- 変更なし。`on_sync_tick()`は既に`local_tick++`のみを行う軽量な実装であり、設計書
  3.3.3節の「割り込み内ではlocal_tickの更新のみを行う」という方針と一致しているため、
  指摘4の観点からも修正の必要はないと判断した。

## 残課題・実機確認が必要な点

- **SPIクロック1MHzでの安定性**: ソフトウェアSPIで1Mbpsに正確に追従できるかは実機での
  ロジックアナライザ確認が必要。不安定な場合は`server.ino`の`SPI_CONFIG`と
  `spi_slave.ino`の`SPI_CLOCK_HZ`を揃えて下げること（コメントに明記済み）。
- **sequence_ackの1トランザクション遅延によるフェイルセーフ挙動**: 3回連続エラーで
  フェイルセーフに入る設計のため通常は問題ないが、結合テスト(WBS:321 SPI結合テスト)で
  実際の遅延量とエラーカウントの挙動を確認すること。
- **MISOバス共有時の衝突確認**: `miso_drive_enable/disable`によるハイインピーダンス化が
  実際に複数台接続時のバス衝突を防げているか、4台接続状態でのロジックアナライザ確認が
  望ましい。
