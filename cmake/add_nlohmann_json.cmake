
function(add_nlohmann_json)
  set(JSON_BuildTests OFF)
  set(JSON_Install OFF)

  add_subdirectory(external/json)
  set_property(TARGET nlohmann_json APPEND PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES $<INSTALL_INTERFACE:${PHASAR_DEPS_INSTALL_DESTINATION}/include>
  )

  install(TARGETS nlohmann_json
    EXPORT ${PHASAR_DEPS_EXPORT_SET}
    LIBRARY DESTINATION ${PHASAR_DEPS_INSTALL_DESTINATION}/lib
    ARCHIVE DESTINATION ${PHASAR_DEPS_INSTALL_DESTINATION}/lib
    RUNTIME DESTINATION ${PHASAR_DEPS_INSTALL_DESTINATION}/bin
  )
  install(DIRECTORY external/json/include/
    DESTINATION ${PHASAR_DEPS_INSTALL_DESTINATION}/include
  )
endfunction()

function(add_json_schema_validator)
  # We need to work around the behavior of nlohmann_json_schema_validator and nlohmann_json here
  # The validator needs the json part, but if you include it, the library of nlohmann_json_schema_validator
  # is not installed, leading to linker error. But just including nlohmann_json is not sufficient, as
  # in the installed state the nlohmann_json_schema_validator needs the nlohmann_json package which needs
  # to be installed.
  # The following workaround may collapse or become unnecessary once the issue is
  # changed or fixed in nlohmann_json_schema_validator.
  if (PHASAR_IN_TREE)
    set_property(GLOBAL APPEND PROPERTY LLVM_EXPORTS nlohmann_json_schema_validator)
  endif()

  set(JSON_VALIDATOR_INSTALL OFF)

  add_subdirectory(external/json-schema-validator)
  set_property(TARGET nlohmann_json_schema_validator APPEND PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES $<INSTALL_INTERFACE:${PHASAR_DEPS_INSTALL_DESTINATION}/include>
  )

  install(TARGETS nlohmann_json_schema_validator
    EXPORT ${PHASAR_DEPS_EXPORT_SET}
    LIBRARY DESTINATION ${PHASAR_DEPS_INSTALL_DESTINATION}/lib
    ARCHIVE DESTINATION ${PHASAR_DEPS_INSTALL_DESTINATION}/lib
    RUNTIME DESTINATION ${PHASAR_DEPS_INSTALL_DESTINATION}/bin
  )
  install(FILES external/json-schema-validator/src/nlohmann/json-schema.hpp
    DESTINATION ${PHASAR_DEPS_INSTALL_DESTINATION}/include/nlohmann
  )
endfunction()
