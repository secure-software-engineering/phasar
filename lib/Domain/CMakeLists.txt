file(GLOB_RECURSE DOMAIN_SRC *.h *.cpp)

add_phasar_library(phasar_domain
  ${DOMAIN_SRC}

  LINK_PRIVATE
    nlohmann_json::nlohmann_json

  MODULE_FILES
    Domain.cppm
)
