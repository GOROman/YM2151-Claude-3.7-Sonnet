cmake_minimum_required(VERSION 3.10)
project(YM2151Emulator VERSION 1.0.0 LANGUAGES CXX)

# C++17を使用
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ソースファイル
set(SOURCES
    src/ym2151.cpp
)

# ヘッダーファイル
set(HEADERS
    include/ym2151/ym2151.h
)

# ライブラリの作成
add_library(ym2151 STATIC ${SOURCES} ${HEADERS})
target_include_directories(ym2151 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# サンプルプログラム
add_executable(simple_tone examples/simple_tone.cpp)
target_link_libraries(simple_tone PRIVATE ym2151)

# インストール設定
install(TARGETS ym2151 DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
