cmake_minimum_required(VERSION 3.11)
project(Hierarchy-Clear)

# Ceres
find_package(Ceres CONFIG REQUIRED)
if(NOT Ceres_FOUND)
    message(FATAL_ERROR "Ceres not found")
endif()

# PCL  
find_package(PCL CONFIG REQUIRED)  
find_package(PCL CONFIG REQUIRED COMPONENTS common io)  

#OpenMP
find_package(OpenMP)

set(CMAKE_CXX_STANDARD 17)

# Class files (shared by both executables)
file(GLOB CLASS_SRC "class/*.cpp" "class/*.h")

# Original executable with visualization
add_executable(Hierarchy-Clear main.cpp ${CLASS_SRC})

# Headless executable without visualization
add_executable(Hierarchy-Clear-headless main_headless.cpp ${CLASS_SRC})

if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
  add_definitions(-D _CRT_SECURE_NO_WARNINGS)
endif()

if(OpenMP_CXX_FOUND)
    target_link_libraries(Hierarchy-Clear
        PUBLIC
            OpenMP::OpenMP_CXX
    )
    target_link_libraries(Hierarchy-Clear-headless
        PUBLIC
            OpenMP::OpenMP_CXX
    )
endif()

target_link_libraries(Hierarchy-Clear
    PRIVATE
        ${CERES_LIBRARIES}
        ${PCL_LIBRARIES}
)
target_include_directories(Hierarchy-Clear
    PUBLIC
        ${CMAKE_SOURCE_DIR}/class
)

target_link_libraries(Hierarchy-Clear-headless
    PRIVATE
        ${CERES_LIBRARIES}
        ${PCL_LIBRARIES}
        stdc++fs
)
target_include_directories(Hierarchy-Clear-headless
    PUBLIC
        ${CMAKE_SOURCE_DIR}/class
)
