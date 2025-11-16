# shared_components/CMakeLists.txt
add_library(shared_components STATIC
    src/protocol_parse.cpp
    src/equipment.cpp
    src/message_buffer.cpp
)
target_include_directories(shared_components PUBLIC include)