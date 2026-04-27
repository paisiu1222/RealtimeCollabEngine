cmake_minimum_required(VERSION 3.14)
project(RealtimeCollabEngine VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准为C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找依赖包
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(SQLite3 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Crow REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# 查找asio（Crow的依赖）
find_path(ASIO_INCLUDE_DIR asio.hpp)
if(NOT ASIO_INCLUDE_DIR)
    message(FATAL_ERROR "Asio not found. Please install libasio-dev")
endif()

# 包含头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
    ${ASIO_INCLUDE_DIR}
)

# 收集源文件
file(GLOB_RECURSE CORE_SOURCES "src/core/*.cpp")
file(GLOB_RECURSE NETWORK_SOURCES "src/network/*.cpp")
file(GLOB_RECURSE STORAGE_SOURCES "src/storage/*.cpp")
file(GLOB_RECURSE API_SOURCES "src/api/*.cpp")
file(GLOB_RECURSE UTILS_SOURCES "src/utils/*.cpp")

# 主程序源文件
set(MAIN_SOURCES src/main.cpp)

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${MAIN_SOURCES}
    ${CORE_SOURCES}
    ${NETWORK_SOURCES}
    ${STORAGE_SOURCES}
    ${API_SOURCES}
    ${UTILS_SOURCES}
)

# 查找UUID库
find_package(PkgConfig REQUIRED)
pkg_check_modules(UUID REQUIRED uuid)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::system
    Boost::thread
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
    Crow::Crow
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    ${UUID_LIBRARIES}
    z
)

# 添加包含目录
target_include_directories(${PROJECT_NAME} PRIVATE ${UUID_INCLUDE_DIRS})

# 添加预处理器定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    CROW_ENABLE_SSL
    CROW_ENABLE_COMPRESSION
)

# 测试支持（可选）
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    
    # Logger测试
    add_executable(test_logger 
        tests/unit/test_logger.cpp
        src/utils/Logger.cpp
    )
    target_link_libraries(test_logger PRIVATE
        Boost::system
        Boost::thread
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    target_include_directories(test_logger PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/unit
    )
    add_test(NAME test_logger COMMAND test_logger)
    
    # Config测试
    add_executable(test_config 
        tests/unit/test_config.cpp
        src/utils/Config.cpp
    )
    target_link_libraries(test_config PRIVATE
        Boost::system
        Boost::thread
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    target_include_directories(test_config PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/unit
    )
    add_test(NAME test_config COMMAND test_config)
    
    # Storage测试（数据库层）
    add_executable(test_storage 
        tests/unit/test_storage.cpp
        src/storage/Database.cpp
        src/storage/DocumentDAO.cpp
        src/storage/OperationDAO.cpp
    )
    target_link_libraries(test_storage PRIVATE
        Boost::system
        Boost::thread
        SQLite::SQLite3
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    target_include_directories(test_storage PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/unit
    )
    add_test(NAME test_storage COMMAND test_storage)
    
    # Network测试（网络层）
    add_executable(test_network 
        tests/unit/test_network.cpp
        src/network/MessageProtocol.cpp
        src/network/WebSocketServer.cpp
        src/utils/Logger.cpp
    )
    target_link_libraries(test_network PRIVATE
        Boost::system
        Boost::thread
        nlohmann_json::nlohmann_json
        Crow::Crow
        Threads::Threads
        ${UUID_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
        z
    )
    target_include_directories(test_network PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/unit
        ${UUID_INCLUDE_DIRS}
    )
    add_test(NAME test_network COMMAND test_network)
    
    # Core测试（OT算法核心）
    add_executable(test_ot_algorithm 
        tests/unit/test_ot_algorithm.cpp
        src/core/Operation.cpp
        src/core/DocumentState.cpp
        src/core/OTAlgorithm.cpp
        src/utils/Logger.cpp
    )
    target_link_libraries(test_ot_algorithm PRIVATE
        Boost::system
        Boost::thread
        nlohmann_json::nlohmann_json
        Threads::Threads
        ${UUID_LIBRARIES}
    )
    target_include_directories(test_ot_algorithm PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/unit
        ${UUID_INCLUDE_DIRS}
    )
    add_test(NAME test_ot_algorithm COMMAND test_ot_algorithm)
    
    message(STATUS "Tests enabled: test_logger, test_config, test_storage, test_network, test_ot_algorithm")
endif()

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/${PROJECT_NAME}
)

# 打印配置信息
message(STATUS "===========================================")
message(STATUS "RealtimeCollabEngine Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Boost Version: ${Boost_VERSION}")
message(STATUS "  SQLite3 Version: ${SQLite3_VERSION}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "===========================================")
