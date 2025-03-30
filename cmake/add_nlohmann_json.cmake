
function(add_nlohmann_json)
  if (PHASAR_USE_CONAN)
    find_package(nlohmann_json REQUIRED)
  else()
    set(JSON_BuildTests OFF)
    set(JSON_Install OFF)

    if (PHASAR_IN_TREE)
      set_property(GLOBAL APPEND PROPERTY LLVM_EXPORTS nlohmann_json)
    endif()

    add_subdirectory(external/json EXCLUDE_FROM_ALL)
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
  endif()
endfunction()

function(add_json_schema_validator)
  if (PHASAR_USE_CONAN)
    find_package(nlohmann_json_schema_validator REQUIRED)
  else()
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

    set(BUILD_SHARED_LIBS_SAVE ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)

    add_subdirectory(external/json-schema-validator EXCLUDE_FROM_ALL)

    set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_SAVE})

    set_property(TARGET nlohmann_json_schema_validator APPEND PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES $<INSTALL_INTERFACE:${PHASAR_DEPS_INSTALL_DESTINATION}/include>
    )

    # Silence warning that we do not install the PUBLIC_HEADER target property.
    # We can't, since it contains a relative path located from deep inside the schema validator tree
    set_target_properties(nlohmann_json_schema_validator PROPERTIES PUBLIC_HEADER "")

    install(TARGETS nlohmann_json_schema_validator
      EXPORT ${PHASAR_DEPS_EXPORT_SET}
      LIBRARY DESTINATION ${PHASAR_DEPS_INSTALL_DESTINATION}/lib
      ARCHIVE DESTINATION ${PHASAR_DEPS_INSTALL_DESTINATION}/lib
      RUNTIME DESTINATION ${PHASAR_DEPS_INSTALL_DESTINATION}/bin
      # PUBLIC_HEADER DESTINATION ${PHASAR_DEPS_INSTALL_DESTINATION}/include/nlohmann
    )
    install(FILES external/json-schema-validator/src/nlohmann/json-schema.hpp
      DESTINATION ${PHASAR_DEPS_INSTALL_DESTINATION}/include/nlohmann
    )
  endif()
endfunction()
