file(GLOB_RECURSE CRYSL_FC_SRC *.h *.cpp)

add_library(FormatConverter SHARED ${CRYSL_FC_SRC})