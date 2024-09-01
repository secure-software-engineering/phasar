file(GLOB_RECURSE CONTROLLER_SRC *.h *.cpp)

target_sources(phasar-cli PRIVATE ${CONTROLLER_SRC})
