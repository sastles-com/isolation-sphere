#!/bin/bash

# 自動再接続モニター for M5atomS3R
# リセット時にも継続してログを表示

PORT="/dev/cu.usbmodem1101"
BAUDRATE="115200"

echo "=== M5atomS3R 継続モニター ==="
echo "ポート: $PORT"
echo "ボーレート: $BAUDRATE"
echo "デバイスリセット時も自動で再接続します"
echo "終了するには Ctrl+C を押してください"
echo "================================"

while true; do
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] 接続試行中..."
    
    # pyserialを使用した継続モニター
    python3 -c "
import serial
import time
import sys

def monitor_serial():
    try:
        ser = serial.Serial('$PORT', $BAUDRATE, timeout=1)
        print(f'[{time.strftime(\"%Y-%m-%d %H:%M:%S\")}] 接続成功 - モニター開始')
        
        while True:
            try:
                if ser.in_waiting:
                    data = ser.readline().decode('utf-8', errors='ignore').strip()
                    if data:
                        print(data)
                else:
                    time.sleep(0.01)
            except (serial.SerialException, OSError) as e:
                print(f'[{time.strftime(\"%Y-%m-%d %H:%M:%S\")}] 接続エラー: {e}')
                break
        
        ser.close()
        
    except Exception as e:
        print(f'[{time.strftime(\"%Y-%m-%d %H:%M:%S\")}] 初期化エラー: {e}')

monitor_serial()
" 2>/dev/null

    echo "[$(date '+%Y-%m-%d %H:%M:%S')] 接続が切断されました。3秒後に再接続..."
    sleep 3
done