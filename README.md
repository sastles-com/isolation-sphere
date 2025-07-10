# M5atomS3R ESP-IDF プロジェクト

M5atomS3R（ESP32-S3）用の統合開発環境です。Dockerコンパイル環境とローカルフラッシュ/モニタ機能を組み合わせています。

## ハードウェア仕様

**M5atomS3R:**
- MCU: ESP32-S3FH4R2  
- Flash: 4MB
- PSRAM: 2MB
- 機能: RGB LED、ボタン、IMU、WiFi/Bluetooth

## クイックスタート

```bash
# 完全なワークフロー：ビルド、フラッシュ、モニタ
./dev.sh

# または段階的に：
./dev.sh build-flash-monitor
```

## 基本的な開発ワークフロー

### 1. コンパイル（Docker使用）

```bash
# プロジェクトをビルド
./docker-build.sh build

# クリーンビルド
./docker-build.sh clean

# 設定メニュー
./docker-build.sh menuconfig
```

### 2. フラッシュ/モニタ（ローカル）

```bash
# フラッシュとモニタ
./flash-monitor.sh flash-monitor

# フラッシュのみ
./flash-monitor.sh flash

# モニタのみ  
./flash-monitor.sh monitor

# ポート検出
./flash-monitor.sh detect
```

### 3. 統合ワークフロー

```bash
# ビルド＋フラッシュ＋モニタ
./dev.sh build-flash-monitor

# フラッシュのみ（リビルドなし）
./dev.sh quick-flash

# プロジェクト状態確認
./dev.sh status
```

## 動作確認方法

### Hello Worldアプリケーションの動作確認

1. **シリアルモニタでの出力確認**
   ```bash
   ./flash-monitor.sh monitor
   ```

2. **期待される出力**
   ```
   === M5atomS3R Hello World ===
   ESP-IDF Version: v5.0.x
   Chip Model: esp32s3
   Free Heap: xxxxx bytes
   Hello World from M5atomS3R! Counter: 1
   Free heap: xxxxx bytes
   Up time: 2 seconds
   ...
   --- System Status Report ---
   Device: M5atomS3R (ESP32-S3)
   Runtime: 10 seconds
   Status: Running OK
   ----------------------------
   ```

3. **ボタン動作確認**
   - M5atomS3Rのボタン（GPIO 41）を押すと：
     ```
     Button pressed!
     Button released!
     ```

4. **正常動作の証拠**
   - 2秒ごとにカウンターが増加
   - 10秒ごとにシステムステータスレポート
   - ボタン押下/離上の検出
   - メモリ情報の更新

### シリアルモニタの使用方法

```bash
# 方法1: スクリプト使用
./flash-monitor.sh monitor

# 方法2: 直接screen使用
screen /dev/cu.usbmodem1101 115200

# 方法3: 統合ワークフローでモニタ開始
./dev.sh build-flash-monitor
```

**モニタ終了方法:**
- `Ctrl+C` (スクリプト使用時)
- `Ctrl+A, K` (screen直接使用時)

### トラブルシューティング

1. **デバイスが見つからない場合**
   ```bash
   # 利用可能なポート確認
   ./flash-monitor.sh detect
   
   # macOSの場合
   ls /dev/cu.*
   ```

2. **ビルドエラーの場合**
   ```bash
   # ログ確認
   ls -la build-logs/
   
   # フルクリーン
   ./docker-build.sh fullclean
   ```

3. **フラッシュエラーの場合**
   ```bash
   # ポート権限確認
   ls -la /dev/cu.*
   
   # 別のポートを試す
   ./flash-monitor.sh flash /dev/cu.usbmodem1101
   ```

## GPIO定義

```c
#define LED_GPIO    GPIO_NUM_35      // RGB LED (WS2812)
#define BUTTON_GPIO GPIO_NUM_41      // Button
#define SDA_GPIO    GPIO_NUM_2       // I2C SDA
#define SCL_GPIO    GPIO_NUM_1       // I2C SCL
```

## 開発のヒント

1. **効率的な開発**
   - `./dev.sh` を使用して完全ワークフローを実行
   - `./dev.sh quick-flash` でリビルドなしフラッシュ
   - `./dev.sh status` で定期的な状態確認

2. **デバッグ**
   - `ESP_LOGI(TAG, "message")` でログ出力
   - シリアルモニタで115200ボーレート確認
   - `build-logs/` ディレクトリでビルドログ確認

3. **コード開発**
   - `main/main.c` でアプリケーションロジック編集
   - 既存のコード規約とパターンに従う
   - 変更後は `./dev.sh build-flash-monitor` で完全テスト

## プロジェクト構成

```
isolation-sphere/
├── main/
│   ├── CMakeLists.txt
│   └── main.c                 # メインアプリケーションコード
├── build-logs/                # ビルドログ（自動作成）
├── build/                     # ビルド成果物（自動作成）
├── CMakeLists.txt             # プロジェクト設定
├── sdkconfig.defaults         # デフォルトESP-IDF設定
├── docker-compose.yml         # Dockerビルド環境
├── docker-build.sh           # Dockerビルドラッパー
├── flash-monitor.sh          # ローカルフラッシュ/モニタツール
├── dev.sh                    # 統合開発ワークフロー
├── CLAUDE.md                 # Claude Code用ガイド
└── README.md                 # このファイル
```

このセットアップにより、プロフェッショナルな開発環境が提供され、ビルドフェーズとデプロイフェーズが明確に分離され、包括的なログ記録と統合ワークフロー自動化が実現されます。