add_executable(runner_miniaudio
    src/main.cpp
    src/App.cpp
)

target_include_directories(runner_miniaudio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniaudio
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(runner_miniaudio PRIVATE drumbox_core)
