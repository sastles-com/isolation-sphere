#!/bin/bash

# Local flash and monitor script for M5atomS3R
# Usage: ./flash-monitor.sh [flash|monitor|flash-monitor|erase|detect]

set -e

# Configuration
PROJECT_NAME="m5atoms3r_project"
DEFAULT_PORT="/dev/ttyUSB0"
BAUDRATE="115200"
FLASH_BAUDRATE="921600"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}[$(date '+%Y-%m-%d %H:%M:%S')] ${message}${NC}"
}

# Function to detect available ports
detect_ports() {
    print_status $BLUE "Detecting available serial ports..."
    
    # Check for common ESP32 ports
    local ports=()
    
    # macOS
    if [[ "$OSTYPE" == "darwin"* ]]; then
        ports=($(ls /dev/cu.* 2>/dev/null | grep -E "(usbserial|SLAB|usbmodem)" || true))
    # Linux
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        ports=($(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || true))
    fi
    
    if [ ${#ports[@]} -eq 0 ]; then
        print_status $YELLOW "No serial ports detected. Please check your device connection."
        return 1
    fi
    
    print_status $GREEN "Found ${#ports[@]} serial port(s):"
    for i in "${!ports[@]}"; do
        echo "  $((i+1)). ${ports[$i]}"
    done
    
    return 0
}

# Function to select port
select_port() {
    local selected_port=""
    
    # If port is provided as argument, use it
    if [ -n "$1" ]; then
        selected_port="$1"
        print_status $BLUE "Using specified port: $selected_port"
    else
        # Auto-detect ports
        if detect_ports; then
            local ports=()
            
            # macOS
            if [[ "$OSTYPE" == "darwin"* ]]; then
                ports=($(ls /dev/cu.* 2>/dev/null | grep -E "(usbserial|SLAB|usbmodem)" || true))
            # Linux
            elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
                ports=($(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || true))
            fi
            
            if [ ${#ports[@]} -eq 1 ]; then
                selected_port="${ports[0]}"
                print_status $GREEN "Auto-selected port: $selected_port"
            else
                print_status $YELLOW "Multiple ports found. Please specify which port to use:"
                read -p "Enter port number (1-${#ports[@]}) or full path: " choice
                
                if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le "${#ports[@]}" ]; then
                    selected_port="${ports[$((choice-1))]}"
                else
                    selected_port="$choice"
                fi
            fi
        else
            selected_port="$DEFAULT_PORT"
            print_status $YELLOW "Using default port: $selected_port"
        fi
    fi
    
    echo "$selected_port"
}

# Function to check if binary exists
check_binary() {
    local binary_path="build/$PROJECT_NAME.bin"
    
    if [ ! -f "$binary_path" ]; then
        print_status $RED "Binary not found: $binary_path"
        print_status $YELLOW "Please build the project first using: ./docker-build.sh build"
        return 1
    fi
    
    print_status $GREEN "Binary found: $binary_path"
    return 0
}

# Function to check if esptool is available
check_esptool() {
    if ! command -v esptool.py &> /dev/null; then
        print_status $YELLOW "esptool.py not found. Attempting to install..."
        
        # Try to install esptool
        if command -v pip3 &> /dev/null; then
            pip3 install esptool
        elif command -v pip &> /dev/null; then
            pip install esptool
        else
            print_status $RED "pip not found. Please install esptool manually:"
            print_status $YELLOW "pip install esptool"
            return 1
        fi
    fi
    
    print_status $GREEN "esptool.py is available"
    return 0
}

# Function to flash firmware
flash_firmware() {
    local port="$1"
    
    print_status $BLUE "Flashing firmware to $port..."
    
    # Check if binary exists
    if ! check_binary; then
        return 1
    fi
    
    # Check if esptool is available
    if ! check_esptool; then
        return 1
    fi
    
    # Flash the firmware
    print_status $BLUE "Starting flash process..."
    
    esptool.py --chip esp32s3 \
        --port "$port" \
        --baud "$FLASH_BAUDRATE" \
        --before default_reset \
        --after hard_reset \
        write_flash \
        --flash_mode dio \
        --flash_freq 80m \
        --flash_size 4MB \
        0x0 build/bootloader/bootloader.bin \
        0x10000 build/$PROJECT_NAME.bin \
        0x8000 build/partition_table/partition-table.bin
    
    print_status $GREEN "Flash completed successfully"
}

# Function to monitor serial output
monitor_serial() {
    local port="$1"
    
    print_status $BLUE "Starting serial monitor on $port (baudrate: $BAUDRATE)"
    print_status $YELLOW "Press Ctrl+C to exit monitor"
    
    # Check if screen is available (macOS/Linux)
    if command -v screen &> /dev/null; then
        screen "$port" "$BAUDRATE"
    # Check if minicom is available (Linux)
    elif command -v minicom &> /dev/null; then
        minicom -D "$port" -b "$BAUDRATE"
    # Check if picocom is available
    elif command -v picocom &> /dev/null; then
        picocom -b "$BAUDRATE" "$port"
    else
        print_status $RED "No terminal program found. Please install one of: screen, minicom, or picocom"
        return 1
    fi
}

# Function to erase flash
erase_flash() {
    local port="$1"
    
    print_status $BLUE "Erasing flash on $port..."
    
    # Check if esptool is available
    if ! check_esptool; then
        return 1
    fi
    
    esptool.py --chip esp32s3 --port "$port" erase_flash
    
    print_status $GREEN "Flash erased successfully"
}

# Main function
main() {
    local command="${1:-flash-monitor}"
    local port="${2:-}"
    
    print_status $BLUE "Starting local flash/monitor process"
    print_status $BLUE "Project: $PROJECT_NAME"
    print_status $BLUE "Command: $command"
    
    # Select port
    local selected_port
    if [ -n "$port" ]; then
        selected_port="$port"
        print_status $BLUE "Using specified port: $selected_port"
    else
        # Auto-detect ports
        local ports=()
        
        # macOS
        if [[ "$OSTYPE" == "darwin"* ]]; then
            ports=($(ls /dev/cu.* 2>/dev/null | grep -E "(usbserial|SLAB|usbmodem)" || true))
        # Linux
        elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
            ports=($(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || true))
        fi
        
        if [ ${#ports[@]} -eq 0 ]; then
            print_status $YELLOW "No serial ports detected. Please check your device connection."
            return 1
        elif [ ${#ports[@]} -eq 1 ]; then
            selected_port="${ports[0]}"
            print_status $GREEN "Auto-selected port: $selected_port"
        else
            print_status $YELLOW "Multiple ports found:"
            for i in "${!ports[@]}"; do
                echo "  $((i+1)). ${ports[$i]}"
            done
            read -p "Enter port number (1-${#ports[@]}): " choice
            
            if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le "${#ports[@]}" ]; then
                selected_port="${ports[$((choice-1))]}"
            else
                print_status $RED "Invalid selection"
                return 1
            fi
        fi
    fi
    
    if [ -z "$selected_port" ]; then
        print_status $RED "No port selected. Exiting."
        return 1
    fi
    
    # Execute command
    case "$command" in
        flash)
            flash_firmware "$selected_port"
            ;;
        monitor)
            monitor_serial "$selected_port"
            ;;
        flash-monitor)
            if flash_firmware "$selected_port"; then
                print_status $BLUE "Waiting 2 seconds before starting monitor..."
                sleep 2
                monitor_serial "$selected_port"
            fi
            ;;
        erase)
            erase_flash "$selected_port"
            ;;
        detect)
            detect_ports
            ;;
        *)
            print_status $RED "Unknown command: $command"
            echo "Usage: $0 [flash|monitor|flash-monitor|erase|detect] [port]"
            echo ""
            echo "Commands:"
            echo "  flash          - Flash firmware to device"
            echo "  monitor        - Start serial monitor"
            echo "  flash-monitor  - Flash firmware and start monitor"
            echo "  erase          - Erase flash memory"
            echo "  detect         - Detect available serial ports"
            echo ""
            echo "Examples:"
            echo "  $0 flash-monitor"
            echo "  $0 flash /dev/ttyUSB0"
            echo "  $0 monitor /dev/cu.usbserial-0001"
            exit 1
            ;;
    esac
    
    print_status $GREEN "Local flash/monitor process completed"
}

# Handle script interruption
trap 'print_status $YELLOW "Script interrupted by user"; exit 130' INT

# Run main function
main "$@"