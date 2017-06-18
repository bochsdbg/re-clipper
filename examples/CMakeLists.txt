CMAKE_MINIMUM_REQUIRED(VERSION 3.2)

find_package(Agg)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_SOURCE_DIR})

set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets)

add_executable(agg_example
  agg_main.cpp
  ../clipper.cpp
  make_arrows.cpp
  make_gb_poly.cpp
)

add_executable(console_example
  console_main.cpp
  ../clipper.cpp
  )

add_executable(qt_example
  qt_main.cpp
  ../clipper.cpp
  )

target_link_libraries(qt_example
  Qt5::Widgets
)

target_include_directories(agg_example PUBLIC
  ${AGG_INCLUDE_DIRS}
  ${AGG_INCLUDE_DIRS}/platform/
  ${AGG_INCLUDE_DIRS}/ctrl/
  )
  
target_link_libraries(agg_example
  ${AGG_LIBRARIES}
  )


