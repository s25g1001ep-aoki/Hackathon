# Arduino Orchestra

複数のArduinoがSPI通信で連携し、「かえるの歌」を演奏するオーケストラシステムです。
カエル人形を圧力センサに乗せると楽器を操作でき、観客の拍手でBPMをリアルタイムに変化させることができます。

---

## システム概要

```
[楽器Arduino x4] ---SPI---> [サーバーArduino] ---USB Serial---> [PC (Processing)]
   圧力センサ                   マイク / スイッチ                      音声生成・出力
```

1. **サーバーArduino** が楽曲データを管理し、SPI通信でBPMクロックを全楽器Arduinoに配信
2. **楽器Arduino** は圧力センサの値をもとに音データをサーバーへ送信
3. **サーバーArduino** が受信した音データをUSBシリアル経由でPCへ転送
4. **PC (Processing)** が加算合成・エンベロープ・エフェクト処理を行い、音声として出力

---

## ハードウェア構成

| 種別 | 台数 | 役割 |
|------|------|------|
| サーバーArduino | 1台 | BPM管理・楽曲データ管理（PROGMEM）・SPI Master・シリアル通信・マイク入力 |
| 楽器Arduino（メロディ） | 3台 | 圧力センサ入力・SoundData送信（SPI Slave） |
| 楽器Arduino（リズム） | 1台 | 圧力センサ入力・SoundData送信（SPI Slave） |

### 主要センサ・デバイス

- **圧力センサ**（楽器Arduino）: カエル人形を乗せると音が鳴る
- **アナログマイクモジュール**（サーバーArduino）: 観客の拍手を検出してBPMを変更
- **スイッチ**（サーバーArduino）: システムの起動・停止

---

## 通信仕様

### SPI（サーバー ↔ 楽器Arduino）

- 1対多（サーバー1台 : 楽器Arduino4台）
- サーバー → 楽器: BPMクロック同期
- 楽器 → サーバー: 割り込み処理で音データ送信

### 音データ構造体

```c
struct SoundData {
    uint8_t instrument_id;  // 楽器番号
    uint8_t pitch;          // 音の高さ（MIDIノート番号）
    uint8_t duration;       // 音の長さ
    uint8_t velocity;       // 音の強さ
};
```

チェックサム方式でエラーを検出し、1楽器につき3回エラーが発生するとフェイルセーフで該当楽器を無音化します。

---

## 音声生成（Processing）

PC側のProcessingアプリが以下の順で音声を生成します。

1. シリアルポートからSoundDataを受信・パース
2. 楽器ごとの倍音構成テーブルを参照し、正弦波を**加算合成**
3. **エンベロープ（ADSR）** をサンプル単位で適用
4. **ビブラート**（音程変調）→ **リバーブ**（ディレイバッファ）の順でエフェクト付加
5. サウンドカードへ出力

---

## 機能一覧

| 機能 | 説明 |
|------|------|
| 自動演奏 | 起動後、「かえるの歌」をデフォルトBPMで自動演奏 |
| 手動演奏 | カエル人形を圧力センサに乗せると、そのパートを手動操作できる |
| 拍手テンポ同期 | マイクで観客の拍手を検出し、BPMをリアルタイム変更 |
| フェイルセーフ | 通信エラー3回で該当楽器を無音化、演奏は継続 |

---

## セットアップ

### 必要なもの

- Arduino x5（サーバー1台 + 楽器4台）
- 圧力センサ x4
- アナログマイクモジュール x1（例: MAX4466）
- Processing（PC側音声生成）
- SPIバス配線一式

### 起動手順

1. PC側でProcessingアプリを起動
2. サーバーArduinoのスイッチをON
3. 自動的に初期化・ハンドシェイクが行われ、演奏が開始される

---

## 開発フェーズ

| フェーズ | 内容 |
|---------|------|
| 設計 | 仕様確定・回路設計・ソフトウェア設計 |
| 実装 | サーバーArduino / 楽器Arduino / Processingアプリの実装 |
| テスト | 単体テスト → 結合テスト → システムテスト → デモ準備 |

---

## チーム

Hackathon Team 5

---

## ディレクトリ構成

```
Hackathon_team5/
├── README.md
├── .gitignore
├── arduino/
│   ├── server/                     # サーバーArduino（BPM管理・SPI Master・シリアル通信）
│   │   └── server.ino
│   ├── instrument_melody/          # メロディ楽器Arduino x3（圧力センサ・SPI Slave）
│   │   └── instrument_melody.ino
│   └── instrument_rhythm/          # リズム楽器Arduino x1（圧力センサ・SPI Slave）
│       └── instrument_rhythm.ino
├── processing/
│   └── orchestra/                  # PC側音声生成アプリ（加算合成・ADSR・エフェクト）
│       └── orchestra.pde
└── docs/
    ├── circuit/                    # 回路図・配線図
    └── design/                     # 設計資料
```

| パス | 説明 |
|------|------|
| `arduino/server/` | サーバーArduino のスケッチ。BPMクロック管理、楽曲データ（PROGMEM）、SPI Master、マイク入力による拍手検出、シリアル送信を担当 |
| `arduino/instrument_melody/` | メロディ楽器Arduino（3台共通）のスケッチ。圧力センサ入力・SoundData を SPI Slave として送信 |
| `arduino/instrument_rhythm/` | リズム楽器Arduino（1台）のスケッチ。圧力センサ入力・SoundData を SPI Slave として送信 |
| `processing/orchestra/` | PC側 Processing アプリ。シリアルポートから SoundData を受信し、加算合成・エンベロープ（ADSR）・ビブラート・リバーブを適用して音声出力 |
| `docs/circuit/` | 回路図・ピン配線図 |
| `docs/design/` | 設計資料・議事録・仕様書 |
