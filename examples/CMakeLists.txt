CMAKE_MINIMUM_REQUIRED(VERSION 3.2)

find_package(Agg)

add_executable(agg_example
  agg_main.cpp
  ../clipper.cpp
  make_arrows.cpp
  make_gb_poly.cpp
)

target_include_directories(agg_example PUBLIC
  ${AGG_INCLUDE_DIRS}
  ${AGG_INCLUDE_DIRS}/platform/
  ${AGG_INCLUDE_DIRS}/ctrl/
  )
  
target_link_libraries(agg_example
  ${AGG_LIBRARIES}
  )



