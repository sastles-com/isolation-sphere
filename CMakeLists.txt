# The following lines of boilerplate have to be in your project's
# CMakeLists.txt file for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Set the project name
set(PROJECT_NAME "m5atoms3r_project")

# Include the ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Define the project
project(${PROJECT_NAME})