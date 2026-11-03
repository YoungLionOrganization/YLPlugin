# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

cmake_minimum_required(VERSION 3.26)
project(YLPluginConsumer LANGUAGES C CXX)
find_package(YLPlugin 0.1 CONFIG REQUIRED)
add_executable(read_c read.c)
target_link_libraries(read_c PRIVATE YLPlugin::ylplugin)
add_executable(read_cpp read.cpp)
target_compile_features(read_cpp PRIVATE cxx_std_20)
target_link_libraries(read_cpp PRIVATE YLPlugin::ylplugin)
