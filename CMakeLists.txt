cmake_minimum_required(VERSION 3.10)

project(heist)

add_executable(webserver
	server/main.cpp
)

set_property(TARGET webserver PROPERTY OUTPUT_NAME "webserver")
set_property(TARGET webserver PROPERTY CXX_STANDARD 20)
