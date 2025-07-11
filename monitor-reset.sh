#!/bin/bash

# M5atomS3R リセット対応モニター
# リセット後のログを確実に取得

PORT="/dev/cu.usbmodem1101"
BAUDRATE="115200"

echo "=== M5atomS3R リセット対応モニター ==="
echo "1. このスクリプトを実行"
echo "2. デバイスのリセットボタンを押す"
echo "3. 起動ログが表示される"
echo "4. 終了するには Ctrl+A → K → Y"
echo "================================="

# screenを使用した安定モニター
exec screen "$PORT" "$BAUDRATE"