cmake_minimum_required(VERSION 3.16)
project(kobiQ VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(kobiQ
  src/main.cpp
  src/ClipboardMonitor.cpp
  src/HistoryStore.cpp
  src/MainWindow.cpp
  src/TrayController.cpp
  src/PasteHelper.cpp
  src/GlobalHotkey.cpp
  src/SingleInstance.cpp
  resources.qrc
)

target_include_directories(kobiQ PRIVATE src)
target_link_libraries(kobiQ PRIVATE Qt6::Widgets Qt6::Network)

install(TARGETS kobiQ RUNTIME DESTINATION bin)
