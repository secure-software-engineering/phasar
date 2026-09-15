cmake_minimum_required(VERSION 3.10)
project(mylib C)
add_library(mylib STATIC mylib.c)   # builds libmylib.a (no executable)
