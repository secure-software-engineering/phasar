file(GLOB_RECURSE CRYSL_PARSER_SRC *.h *.cpp)

add_library(Parser SHARED ${CRYSL_PARSER_SRC})
target_link_libraries(Parser
    antlr4-runtime
)

#target_compile_features(Parser PRIVATE cxx_std_17)
#set_target_properties(Parser
#	PROPERTIES
#	LINKER_LANGUAGE CXX
#	PREFIX "lib"
#)
