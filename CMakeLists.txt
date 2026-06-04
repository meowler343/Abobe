cmake_minimum_required (VERSION 3.10)
# Добавляем WIN32 для чистоты без консоли
add_executable (AbobaFotohob WIN32 "AbobaFotohob.cpp" "AbobaFotohob.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET AbobaFotohob PROPERTY CXX_STANDARD 20)
endif()
