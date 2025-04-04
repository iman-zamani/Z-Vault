cmake_minimum_required(VERSION 3.5)

project(Z-Vault)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt5Widgets
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# Attempt to find Crypto++ using pkg-config first
find_package(PkgConfig QUIET)
pkg_check_modules(CRYPTO_PP QUIET libcryptopp)

if(CRYPTO_PP_FOUND)
    message(STATUS "Using Crypto++ from pkg-config")
else()
    # If pkg-config failed, try the conventional way
    find_path(CRYPTOPP_INCLUDE_DIR NAMES cryptopp/cryptlib.h)
    find_library(CRYPTOPP_LIBRARY NAMES cryptopp libcryptopp)

    if(CRYPTOPP_INCLUDE_DIR AND CRYPTOPP_LIBRARY)
        set(CRYPTO_PP_INCLUDE_DIRS ${CRYPTOPP_INCLUDE_DIR})
        set(CRYPTO_PP_LIBRARIES ${CRYPTOPP_LIBRARY})
        message(STATUS "Found Crypto++: ${CRYPTOPP_LIBRARY}")
    else()
        message(FATAL_ERROR "Crypto++ not found")
    endif()
endif()

# Define the source directory
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Define the source files
set(SOURCE_FILES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/mainwindow.cpp
    ${SRC_DIR}/mainwindow.h
    ${SRC_DIR}/encrypt.cpp
    ${SRC_DIR}/encrypt.h
)

# Add executable based on the source files
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Include directories for Crypto++ and our source files
include_directories(${CRYPTO_PP_INCLUDE_DIRS} ${SRC_DIR})

# Linking against Qt5 and Crypto++
target_link_libraries(${PROJECT_NAME} Qt5::Widgets ${CRYPTO_PP_LIBRARIES})
