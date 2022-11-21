cmake_minimum_required(VERSION 3.21)
project(mmn15_defense)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main.cpp
        TcpClientChannel.cpp
        TcpClientChannel.h
        FileHandler.cpp
        FileHandler.h
        ServerIcd.h
        ClientLogic.cpp
        ClientLogic.h
        RSAWrapper.cpp
        RSAWrapper.h
        ConfigManager.cpp
        ConfigManager.h
        ClientManager.cpp
        ClientManager.h
        StringWrapper.cpp
        StringWrapper.h
        AESWrapper.cpp
        AESWrapper.h
        )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ")

# Set Boost library
set(BOOST_ROOT "/opt/homebrew/Cellar/boost/boost")
set(BOOST_INCLUDE_DIR "/opt/homebrew/Cellar/boost/boost/include")
set(BOOST_LIBRARY_DIR "/opt/homebrew/Cellar/boost/boost/lib")
# Set CryptoPP library
set(CRYPTOPP_INCLUDE_DIR "/Users/milytopal/CLionProjects/mmn15_defense/libs/cryptopp/include/cryptopp")
set(CRYPTOPP_LIBRARY_DIR "/Users/milytopal/CLionProjects/mmn15_defense/libs/cryptopp")

if(APPLE)
    set(PROJECT_ROOT "/Users/milytopal/CLionProjects/mmn15_defense")
elseif(UNIX)
    set(PROJECT_ROOT "/home/milytopal/CLionProjects/mmn15_defense")
elseif(WIN32)
    set(PROJECT_ROOT "D:\\mmn15_defense")
endif()
find_package(Boost 1.79.0)
find_package(CryptoPP REQUIRED)
include_directories(${BOOST_INCLUDE_DIR})
include_directories(/Users/milytopal/CLionProjects/mmn15_defense/libs)

include_directories(${CRYPTOPP_INCLUDE_DIR})
include_directories(${CRYPTOPP_LIBRARY_DIR})

if(NOT BOOST_FOUND)
    message(FATAL_ERROR "Could not find boost!")
endif()
if(NOT CryptoPP_FOUND)
    message(FATAL_ERROR "Could not find cryptopp!")
endif()

link_libraries(${CRYPTOPP_LIBRARY_DIR}/libcryptopp.a)
link_directories(${CRYPTOPP_LIBRARY_DIR})
link_directories(${CRYPTOPP_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC ${CRYPTOPP_LIBRARY_DIR}/libcryptopp.a)
target_link_directories(${PROJECT_NAME} PUBLIC ${CRYPTOPP_INCLUDE_DIR})
target_link_directories(${PROJECT_NAME} PUBLIC ${BOOST_INCLUDE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})


