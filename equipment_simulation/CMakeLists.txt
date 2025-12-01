cmake_minimum_required(VERSION 3.10)
project(EquipmentStimulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------- 第一步：正确定义测试程序（程序名+源文件列表） --------------------------
# 格式：每个程序名后面紧跟其依赖的源文件，用分号分隔（换行仅为美观）
set(TEST_TARGETS

    # 第一个测试程序
    test_simulation_manager
    src/test_simulation_manager.cpp
    src/simulation_manager.cpp
    src/simulator_connections.cpp
    src/simulator_database_reader.cpp

    # 第二个测试程序
    test_integration
    src/test_integration.cpp
    src/simulation_manager.cpp
    src/simulator_connections.cpp
    src/simulator_database_reader.cpp

    # 第三个测试程序（修复拼写+路径）
    test_performance
    src/test_performance.cpp
    src/simulation_manager.cpp
    src/simulator_connections.cpp
    src/simulator_database_reader.cpp

    # 第四个测试程序（修复拼写：simualtor→simulator）
    test_simulator_connections
    src/test_simulator_connections.cpp
    src/simulation_manager.cpp
    src/simulator_connections.cpp
    src/simulator_database_reader.cpp
)

# -------------------------- 第二步：公共配置（不变，减少冗余） --------------------------
set(PUBLIC_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/shared_components/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 依赖库（MySQL + pthread + 共享组件）
find_library(MYSQL_LIB mysqlclient)

if(NOT MYSQL_LIB)
    message(FATAL_ERROR "MySQL client library not found")
endif()

find_package(Threads REQUIRED)
set(PUBLIC_LIBS
    shared_components
    ${MYSQL_LIB}
    Threads::Threads
)

# 公共编译选项
set(PUBLIC_COMPILE_OPTIONS
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

# -------------------------- 第三步：用 while 循环正确生成测试程序 --------------------------
# 初始化索引（从0开始遍历 TEST_TARGETS）
set(CURRENT_INDEX 0)

# 获取 TEST_TARGETS 的总元素个数
list(LENGTH TEST_TARGETS TARGET_COUNT)

# 循环：直到索引超过列表长度
while(${CURRENT_INDEX} LESS ${TARGET_COUNT})
    # 1. 提取当前索引对应的「程序名」（第一个元素是程序名）
    list(GET TEST_TARGETS ${CURRENT_INDEX} TEST_NAME)

    # 索引+1（准备提取源文件）
    math(EXPR CURRENT_INDEX "${CURRENT_INDEX} + 1")

    # 2. 提取「源文件列表」：从当前索引开始，直到下一个程序名（或列表结束）
    set(SRC_FILES "")

    # 循环：只要当前索引没到末尾，且下一个元素不是程序名（程序名不含 src/）
    while(${CURRENT_INDEX} LESS ${TARGET_COUNT})
        list(GET TEST_TARGETS ${CURRENT_INDEX} CURRENT_ITEM)

        # 判断当前元素是否是源文件（含 src/ 则为源文件）
        if(CURRENT_ITEM MATCHES "src/")
            list(APPEND SRC_FILES ${CURRENT_ITEM})
            math(EXPR CURRENT_INDEX "${CURRENT_INDEX} + 1")
        else()
            # 遇到下一个程序名，退出源文件提取
            break()
        endif()
    endwhile()

    # 3. 生成可执行程序（核心：程序名是纯字符串，源文件是列表）
    add_executable(${TEST_NAME} ${SRC_FILES})

    # 4. 配置头文件、链接库、编译选项
    target_include_directories(${TEST_NAME} PRIVATE ${PUBLIC_INCLUDE_DIRS})
    target_link_libraries(${TEST_NAME} PRIVATE ${PUBLIC_LIBS})
    target_compile_options(${TEST_NAME} PRIVATE ${PUBLIC_COMPILE_OPTIONS})
endwhile()

# 1. 主程序可执行文件
add_executable(equipment_simulation
    src/main.cpp # 入口
    src/simulation_manager.cpp # 以下与测试程序共用的实现
    src/simulator_connections.cpp
    src/simulator_database_reader.cpp
)

# 2. 主程序用同样的头文件/库/编译选项
target_include_directories(equipment_simulation PRIVATE ${PUBLIC_INCLUDE_DIRS})
target_link_libraries(equipment_simulation PRIVATE ${PUBLIC_LIBS})
target_compile_options(equipment_simulation PRIVATE ${PUBLIC_COMPILE_OPTIONS})

# -------------------------- （可选优化）公共代码提取为静态库（推荐） --------------------------
# 若编译慢，可启用此优化，替换上面的源文件配置：
# add_library(sim_core STATIC
# src/simulation_manager.cpp
# src/simulator_connections.cpp
# src/simulator_database_reader.cpp
# )
# target_include_directories(sim_core PUBLIC ${PUBLIC_INCLUDE_DIRS})
# target_link_libraries(sim_core PUBLIC ${PUBLIC_LIBS})
#
# # 测试程序仅需自己的测试文件
# set(TEST_TARGETS
# test_simulation_manager src/test_simulation_manager.cpp
# test_integration src/test_integration.cpp
# test_performance src/test_performance.cpp
# test_simulator_connections src/test_simulator_connections.cpp
# )