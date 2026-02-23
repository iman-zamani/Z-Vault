cmake_minimum_required(VERSION 3.10...3.28)

project(Z-Vault)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(APPLE)
    execute_process(COMMAND brew --prefix qt@5
                    OUTPUT_VARIABLE QT_HOMEBREW_PREFIX
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)
    if(QT_HOMEBREW_PREFIX)
        list(APPEND CMAKE_PREFIX_PATH "${QT_HOMEBREW_PREFIX}")
    endif()
    
    execute_process(COMMAND brew --prefix cryptopp
                    OUTPUT_VARIABLE CRYPTOPP_HOMEBREW_PREFIX
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)
    if(CRYPTOPP_HOMEBREW_PREFIX)
        list(APPEND CMAKE_PREFIX_PATH "${CRYPTOPP_HOMEBREW_PREFIX}")
    endif()
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(CRYPTO_PP QUIET IMPORTED_TARGET libcryptopp)
endif()

if(CRYPTO_PP_FOUND)
    message(STATUS "Using Crypto++ from pkg-config")
    set(CRYPTO_PP_TARGET PkgConfig::CRYPTO_PP)
else()
    find_path(CRYPTOPP_INCLUDE_DIR NAMES cryptopp/cryptlib.h)
    find_library(CRYPTOPP_LIBRARY NAMES cryptopp libcryptopp cryptlib)

    if(CRYPTOPP_INCLUDE_DIR AND CRYPTOPP_LIBRARY)
        message(STATUS "Found Crypto++ fallback: ${CRYPTOPP_LIBRARY}")
        add_library(CryptoPP::CryptoPP UNKNOWN IMPORTED)
        set_target_properties(CryptoPP::CryptoPP PROPERTIES
            IMPORTED_LOCATION "${CRYPTOPP_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${CRYPTOPP_INCLUDE_DIR}"
        )
        set(CRYPTO_PP_TARGET CryptoPP::CryptoPP)
    else()
        message(FATAL_ERROR "Crypto++ not found. Please ensure it is installed.")
    endif()
endif()

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SOURCE_FILES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/mainwindow.cpp
    ${SRC_DIR}/mainwindow.h
    ${SRC_DIR}/encrypt.cpp
    ${SRC_DIR}/encrypt.h
)

set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.icns")
set(APP_ICON_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.rc")

if(APPLE)
    list(APPEND SOURCE_FILES ${APP_ICON_MACOS})
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
elseif(WIN32)
    list(APPEND SOURCE_FILES ${APP_ICON_WINDOWS})
endif()

add_executable(${PROJECT_NAME} MACOSX_BUNDLE WIN32 ${SOURCE_FILES})

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_ICON_FILE icon.icns
        MACOSX_BUNDLE_GUI_IDENTIFIER com.zvault.app
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets ${CRYPTO_PP_TARGET})