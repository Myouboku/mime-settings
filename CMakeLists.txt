cmake_minimum_required(VERSION 3.16)

project(mime-settings VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)

add_executable(mime-settings
  src/main.cpp
  src/ui/MainWindow.cpp
  src/ui/MainWindow.h
  src/ui/DetailsPane.cpp
  src/ui/DetailsPane.h
  src/models/MimeTypeModel.cpp
  src/models/MimeTypeModel.h
  src/models/MimeTypeFilterProxy.cpp
  src/models/MimeTypeFilterProxy.h
  src/services/AppRegistry.cpp
  src/services/AppRegistry.h
  src/services/MimeDefaultsStore.cpp
  src/services/MimeDefaultsStore.h
  src/services/MimeAssociationService.cpp
  src/services/MimeAssociationService.h
  src/utils/XdgPaths.cpp
  src/utils/XdgPaths.h
)

target_link_libraries(mime-settings PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core)

target_include_directories(mime-settings PRIVATE src)
