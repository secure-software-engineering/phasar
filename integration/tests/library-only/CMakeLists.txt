cmake_minimum_required(VERSION 3.21)
project(library_only C)
add_library(mylib STATIC alpha.c beta.c)
