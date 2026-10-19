cmake_minimum_required(VERSION 3.28)
project(sunbreak VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SUNBREAK_BUILD_GODOT_BRIDGE "Build the Godot 4.7.2 GDExtension shared library" ON)
option(SUNBREAK_BUILD_TESTS "Build C++23 contract tests" ON)

add_library(sunbreak_core STATIC
  src/match_engine.cpp
)
target_include_directories(sunbreak_core PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_options(sunbreak_core PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>
)

if(SUNBREAK_BUILD_TESTS)
  enable_testing()
  add_executable(sunbreak_match_test
    test/match_engine_test.cpp
  )
  target_link_libraries(sunbreak_match_test PRIVATE sunbreak_core)
  target_compile_options(sunbreak_match_test PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Werror>
  )
  add_test(NAME sunbreak-match-engine-contract COMMAND sunbreak_match_test)
endif()

if(SUNBREAK_BUILD_GODOT_BRIDGE)
  include(FetchContent)
  set(GODOTCPP_BUILD_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/binding-profile.json"
      CACHE PATH "Godot minimal binding profile" FORCE)
  set(GODOTCPP_DISABLE_EXCEPTIONS OFF CACHE BOOL "Allow C++ boundary error handling" FORCE)

  set(SUNBREAK_GODOT_CPP_SOURCE_DIR "" CACHE PATH
      "Optional local checkout of godot-cpp to avoid network fetch")
  if(NOT "${SUNBREAK_GODOT_CPP_SOURCE_DIR}" STREQUAL "" AND
     EXISTS "${SUNBREAK_GODOT_CPP_SOURCE_DIR}/CMakeLists.txt")
    FetchContent_Declare(godot_cpp
      SOURCE_DIR "${SUNBREAK_GODOT_CPP_SOURCE_DIR}"
      EXCLUDE_FROM_ALL
    )
  else()
    FetchContent_Declare(godot_cpp
      URL https://github.com/godotengine/godot-cpp/archive/e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77.tar.gz
      URL_HASH SHA256=d4c03daf46c8bef4544614182b0762c3e32789151c7f8f44e8eb2053d5cddeba
      EXCLUDE_FROM_ALL
    )
  endif()
  FetchContent_MakeAvailable(godot_cpp)
  set_property(TARGET godot-cpp PROPERTY SYSTEM TRUE)

  add_library(sunbreak_godot SHARED
    src/godot_bridge.cpp
  )
  target_link_libraries(sunbreak_godot PRIVATE
    sunbreak_core
    godot::cpp
  )
  target_compile_options(sunbreak_godot PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
  )
  set_target_properties(sunbreak_godot PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
  )
  add_custom_command(TARGET sunbreak_godot POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/godot/bin"
    COMMAND "${CMAKE_COMMAND}" -E copy
      "$<TARGET_FILE:sunbreak_godot>"
      "${CMAKE_CURRENT_SOURCE_DIR}/godot/bin/$<TARGET_FILE_NAME:sunbreak_godot>"
  )
endif()
