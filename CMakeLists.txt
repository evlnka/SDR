cmake_minimum_required(VERSION 3.16)
project(PlutoSDR CXX) 

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# Прямой поиск библиотек
find_path(SOAPYSDR_INCLUDE_DIR SoapySDR/Device.h
    PATHS /usr/include /usr/local/include)

find_library(SOAPYSDR_LIBRARY SoapySDR
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)

if(NOT SOAPYSDR_INCLUDE_DIR OR NOT SOAPYSDR_LIBRARY)
    message(FATAL_ERROR "SoapySDR не найден")
else()
    message(STATUS "Found SoapySDR: ${SOAPYSDR_LIBRARY}")
    message(STATUS "Include dir: ${SOAPYSDR_INCLUDE_DIR}")
endif()

# Поиск математической библиотеки
find_library(M_LIB m)

add_executable(main main.cpp)  # Оставляем .cpp расширение

target_include_directories(main PRIVATE ${SOAPYSDR_INCLUDE_DIR})
target_link_libraries(main ${SOAPYSDR_LIBRARY} ${M_LIB})
target_compile_options(main PRIVATE -g -Wall)

# Для C++ может потребоваться явно указать использование C++11
target_compile_features(main PRIVATE cxx_std_11)