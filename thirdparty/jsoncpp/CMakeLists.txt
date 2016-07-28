cmake_minimum_required(VERSION 2.8.11)

set(PUBLIC_HEADERS
    json/config.h
    json/forwards.h
    json/features.h
    json/value.h
    json/reader.h
    json/writer.h
    json/assertions.h
    json/version.h
    )

set(jsoncpp_sources
    json_tool.h
    json_reader.cpp
    json_valueiterator.inl
    json_value.cpp
    json_writer.cpp
    )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(jsoncpp STATIC ${PUBLIC_HEADERS} ${jsoncpp_sources})
