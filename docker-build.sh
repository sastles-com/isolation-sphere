#!/bin/bash

# Docker-based ESP-IDF build script for M5atomS3R
# Usage: ./docker-build.sh [build|clean|menuconfig|fullclean]

set -e

# Configuration
PROJECT_NAME="m5atoms3r_project"
DOCKER_SERVICE="esp-idf-build"
BUILD_LOG_DIR="./build-logs"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

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

# Function to check if Docker is running
check_docker() {
    if ! docker info >/dev/null 2>&1; then
        print_status $RED "Docker is not running. Please start Docker first."
        exit 1
    fi
}

# Function to create build log directory
prepare_logs() {
    mkdir -p "$BUILD_LOG_DIR"
    print_status $BLUE "Build logs will be saved to: $BUILD_LOG_DIR"
}

# Function to run command in Docker container
run_in_docker() {
    local cmd="$1"
    local log_file="$BUILD_LOG_DIR/build_${TIMESTAMP}.log"
    
    print_status $BLUE "Executing: $cmd"
    print_status $BLUE "Log file: $log_file"
    
    # Start container if not running
    if ! docker-compose ps | grep -q "Up"; then
        print_status $YELLOW "Starting Docker container..."
        docker-compose up -d
        sleep 2
    fi
    
    # Execute command and capture output
    if docker-compose exec -T $DOCKER_SERVICE bash -c "
        source /opt/esp/idf/export.sh > /dev/null 2>&1 &&
        echo 'ESP-IDF environment loaded' &&
        echo 'Target: esp32s3' &&
        echo 'Project: $PROJECT_NAME' &&
        echo 'Command: $cmd' &&
        echo '========================================' &&
        $cmd
    " 2>&1 | tee "$log_file"; then
        print_status $GREEN "Command completed successfully"
        echo "$(date '+%Y-%m-%d %H:%M:%S') SUCCESS: $cmd" >> "$BUILD_LOG_DIR/build_summary.log"
        return 0
    else
        local exit_code=$?
        print_status $RED "Command failed with exit code: $exit_code"
        echo "$(date '+%Y-%m-%d %H:%M:%S') ERROR: $cmd (exit code: $exit_code)" >> "$BUILD_LOG_DIR/build_summary.log"
        return $exit_code
    fi
}

# Function to stop Docker container
stop_docker() {
    if docker-compose ps | grep -q "Up"; then
        print_status $YELLOW "Stopping Docker container..."
        docker-compose down
    fi
}

# Main function
main() {
    local command="${1:-build}"
    
    print_status $BLUE "Starting Docker-based ESP-IDF build process"
    print_status $BLUE "Project: $PROJECT_NAME"
    print_status $BLUE "Command: $command"
    
    # Check prerequisites
    check_docker
    prepare_logs
    
    # Execute command
    case "$command" in
        build)
            run_in_docker "idf.py build"
            ;;
        clean)
            run_in_docker "idf.py clean"
            ;;
        fullclean)
            run_in_docker "idf.py fullclean"
            ;;
        menuconfig)
            print_status $YELLOW "Starting menuconfig in interactive mode..."
            docker-compose exec $DOCKER_SERVICE bash -c "
                source /opt/esp/idf/export.sh &&
                idf.py menuconfig
            "
            ;;
        size)
            run_in_docker "idf.py size"
            ;;
        size-components)
            run_in_docker "idf.py size-components"
            ;;
        size-files)
            run_in_docker "idf.py size-files"
            ;;
        shell)
            print_status $YELLOW "Starting interactive shell in Docker container..."
            docker-compose exec $DOCKER_SERVICE bash -c "
                source /opt/esp/idf/export.sh &&
                echo 'ESP-IDF environment loaded. You can now use idf.py commands.' &&
                bash
            "
            ;;
        stop)
            stop_docker
            ;;
        logs)
            print_status $BLUE "Recent build logs:"
            ls -la "$BUILD_LOG_DIR"
            ;;
        *)
            print_status $RED "Unknown command: $command"
            echo "Usage: $0 [build|clean|fullclean|menuconfig|size|size-components|size-files|shell|stop|logs]"
            echo ""
            echo "Commands:"
            echo "  build           - Build the project"
            echo "  clean           - Clean build files"
            echo "  fullclean       - Full clean including config"
            echo "  menuconfig      - Open configuration menu"
            echo "  size            - Show binary size information"
            echo "  size-components - Show component size information"
            echo "  size-files      - Show file size information"
            echo "  shell           - Open interactive shell in container"
            echo "  stop            - Stop Docker container"
            echo "  logs            - Show recent build logs"
            exit 1
            ;;
    esac
    
    print_status $GREEN "Docker build process completed"
}

# Handle script interruption
trap 'print_status $YELLOW "Script interrupted by user"; exit 130' INT

# Run main function
main "$@"