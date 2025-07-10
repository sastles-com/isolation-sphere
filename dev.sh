#!/bin/bash

# M5atomS3R Development Workflow Script
# Integrated Docker build + Local flash/monitor workflow
# Usage: ./dev.sh [build-flash|build-monitor|build-flash-monitor|quick-flash|status|clean|help]

set -e

# Configuration
PROJECT_NAME="m5atoms3r_project"
LOG_DIR="./build-logs"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}[$(date '+%Y-%m-%d %H:%M:%S')] ${message}${NC}"
}

# Function to print banner
print_banner() {
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║                    M5atomS3R Development Tool                   ║${NC}"
    echo -e "${CYAN}║                Docker Build + Local Flash/Monitor             ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════════╝${NC}"
}

# Function to check prerequisites
check_prerequisites() {
    print_status $BLUE "Checking prerequisites..."
    
    local missing_tools=()
    
    # Check if Docker is installed and running
    if ! command -v docker &> /dev/null; then
        missing_tools+=("docker")
    elif ! docker info >/dev/null 2>&1; then
        print_status $RED "Docker is not running. Please start Docker first."
        return 1
    fi
    
    # Check if docker-compose is available
    if ! command -v docker-compose &> /dev/null; then
        missing_tools+=("docker-compose")
    fi
    
    # Check if our scripts exist
    if [ ! -f "./docker-build.sh" ]; then
        print_status $RED "docker-build.sh not found in current directory"
        return 1
    fi
    
    if [ ! -f "./flash-monitor.sh" ]; then
        print_status $RED "flash-monitor.sh not found in current directory"
        return 1
    fi
    
    # Report missing tools
    if [ ${#missing_tools[@]} -gt 0 ]; then
        print_status $RED "Missing required tools: ${missing_tools[*]}"
        print_status $YELLOW "Please install the missing tools and try again."
        return 1
    fi
    
    print_status $GREEN "All prerequisites satisfied"
    return 0
}

# Function to show project status
show_status() {
    print_banner
    print_status $BLUE "Project Status for: $PROJECT_NAME"
    echo ""
    
    # Check if build directory exists
    if [ -d "build" ]; then
        print_status $GREEN "Build directory: EXISTS"
        
        # Check if binary exists
        if [ -f "build/$PROJECT_NAME.bin" ]; then
            local binary_size=$(ls -lh "build/$PROJECT_NAME.bin" | awk '{print $5}')
            local binary_date=$(ls -l "build/$PROJECT_NAME.bin" | awk '{print $6, $7, $8}')
            print_status $GREEN "Binary: EXISTS (${binary_size}, ${binary_date})"
        else
            print_status $YELLOW "Binary: NOT FOUND"
        fi
    else
        print_status $YELLOW "Build directory: NOT FOUND"
    fi
    
    # Check Docker container status
    if docker-compose ps 2>/dev/null | grep -q "Up"; then
        print_status $GREEN "Docker container: RUNNING"
    else
        print_status $YELLOW "Docker container: NOT RUNNING"
    fi
    
    # Check recent logs
    if [ -d "$LOG_DIR" ]; then
        local log_count=$(ls -1 "$LOG_DIR"/*.log 2>/dev/null | wc -l)
        print_status $BLUE "Build logs: $log_count files in $LOG_DIR"
        
        if [ $log_count -gt 0 ]; then
            print_status $BLUE "Recent logs:"
            ls -lt "$LOG_DIR"/*.log 2>/dev/null | head -3 | while read -r line; do
                echo "  $line"
            done
        fi
    else
        print_status $YELLOW "Build logs: Directory not found"
    fi
    
    echo ""
}

# Function to build project
build_project() {
    print_status $BLUE "Building project with Docker..."
    
    if ./docker-build.sh build; then
        print_status $GREEN "Build completed successfully"
        return 0
    else
        print_status $RED "Build failed"
        return 1
    fi
}

# Function to flash firmware
flash_firmware() {
    local port="$1"
    
    print_status $BLUE "Flashing firmware..."
    
    if [ -n "$port" ]; then
        ./flash-monitor.sh flash "$port"
    else
        ./flash-monitor.sh flash
    fi
}

# Function to monitor serial
monitor_serial() {
    local port="$1"
    
    print_status $BLUE "Starting serial monitor..."
    
    if [ -n "$port" ]; then
        ./flash-monitor.sh monitor "$port"
    else
        ./flash-monitor.sh monitor
    fi
}

# Function to clean project
clean_project() {
    print_status $BLUE "Cleaning project..."
    
    # Clean build files
    if ./docker-build.sh clean; then
        print_status $GREEN "Clean completed successfully"
    else
        print_status $YELLOW "Clean encountered issues"
    fi
    
    # Optionally clean logs
    read -p "Do you want to clean build logs as well? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        if [ -d "$LOG_DIR" ]; then
            rm -rf "$LOG_DIR"/*
            print_status $GREEN "Build logs cleaned"
        fi
    fi
}

# Function to show help
show_help() {
    print_banner
    echo "Usage: $0 [COMMAND] [OPTIONS]"
    echo ""
    echo "COMMANDS:"
    echo "  build-flash        Build project and flash to device"
    echo "  build-monitor      Build project and start serial monitor"
    echo "  build-flash-monitor Build, flash, and start monitor (default)"
    echo "  quick-flash        Flash existing binary without rebuilding"
    echo "  status             Show project status"
    echo "  clean              Clean build files"
    echo "  help               Show this help message"
    echo ""
    echo "OPTIONS:"
    echo "  -p, --port PORT    Specify serial port (e.g., /dev/ttyUSB0)"
    echo ""
    echo "EXAMPLES:"
    echo "  $0                              # Build, flash, and monitor"
    echo "  $0 build-flash-monitor          # Same as above"
    echo "  $0 build-flash -p /dev/ttyUSB0  # Build and flash to specific port"
    echo "  $0 quick-flash                  # Flash without rebuilding"
    echo "  $0 status                       # Show project status"
    echo "  $0 clean                        # Clean build files"
    echo ""
    echo "DEVELOPMENT WORKFLOW:"
    echo "  1. Edit your code in main/main.c"
    echo "  2. Run: $0 build-flash-monitor"
    echo "  3. View output in serial monitor"
    echo "  4. Press Ctrl+C to exit monitor"
    echo "  5. Repeat from step 1"
    echo ""
}

# Main function
main() {
    local command="${1:-build-flash-monitor}"
    local port=""
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -p|--port)
                port="$2"
                shift 2
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                command="$1"
                shift
                ;;
        esac
    done
    
    # Check prerequisites
    if ! check_prerequisites; then
        exit 1
    fi
    
    # Execute command
    case "$command" in
        build-flash)
            print_banner
            print_status $BLUE "Starting build-flash workflow..."
            
            if build_project; then
                flash_firmware "$port"
            else
                print_status $RED "Build failed, skipping flash"
                exit 1
            fi
            ;;
        build-monitor)
            print_banner
            print_status $BLUE "Starting build-monitor workflow..."
            
            if build_project; then
                monitor_serial "$port"
            else
                print_status $RED "Build failed, skipping monitor"
                exit 1
            fi
            ;;
        build-flash-monitor)
            print_banner
            print_status $BLUE "Starting complete build-flash-monitor workflow..."
            
            if build_project; then
                flash_firmware "$port"
                if [ $? -eq 0 ]; then
                    print_status $BLUE "Waiting 2 seconds before starting monitor..."
                    sleep 2
                    monitor_serial "$port"
                else
                    print_status $RED "Flash failed, skipping monitor"
                    exit 1
                fi
            else
                print_status $RED "Build failed, skipping flash and monitor"
                exit 1
            fi
            ;;
        quick-flash)
            print_banner
            print_status $BLUE "Starting quick-flash (no rebuild)..."
            flash_firmware "$port"
            ;;
        status)
            show_status
            ;;
        clean)
            print_banner
            clean_project
            ;;
        help)
            show_help
            ;;
        *)
            print_status $RED "Unknown command: $command"
            echo "Use '$0 help' for usage information."
            exit 1
            ;;
    esac
    
    print_status $GREEN "Workflow completed successfully"
}

# Handle script interruption
trap 'print_status $YELLOW "Workflow interrupted by user"; exit 130' INT

# Run main function
main "$@"