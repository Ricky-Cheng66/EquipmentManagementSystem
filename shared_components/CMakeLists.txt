# shared_components/CMakeLists.txt
add_library(shared_components STATIC
    src/protocol_parse.cpp
    src/equipment.cpp
    src/message_buffer.cpp
    src/epoll.cpp
    src/socket.cpp
)
target_include_directories(shared_components PUBLIC include)