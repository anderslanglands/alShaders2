include(CMakeParseArguments)

# define shader build function
function(build_shader)
    set(options)
    set(one_value_args NAME)
    set(multi_value_args SOURCES FLAGS)
    cmake_parse_arguments(
        shader
        "${options}"
        "${one_value_args}"
        "${multi_value_args}"
        "${ARGN}"
    )
    message("NAME: ${shader_NAME}")
    message("SOURCES: ${shader_SOURCES}")
    set(UI ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.ui)
    #set(MTD ${CMAKE_CURRENT_BINARY_DIR}/${SHADER}.mtd)
    #set(AE ${CMAKE_CURRENT_BINARY_DIR}/${SHADER}Template.py)
    #set(AEXML ${CMAKE_CURRENT_BINARY_DIR}/AE${SHADER}Template.xml)
    #set(NEXML ${CMAKE_CURRENT_BINARY_DIR}/NE${SHADER}Template.xml)
    #set(SPDL ${CMAKE_CURRENT_BINARY_DIR}/${SHADER}.spdl)
    #set(KARGS ${CMAKE_CURRENT_BINARY_DIR}/${SHADER}.args)
    #set(C4DRES ${CMAKE_CURRENT_BINARY_DIR}/C4DtoA)
    #set(HTML ${CMAKE_SOURCE_DIR}/docs/${SHADER}.html)

    add_library(${shader_NAME} SHARED ${shader_SOURCES})

    target_link_libraries(${shader_NAME} ai)
    set_target_properties(${shader_NAME} PROPERTIES PREFIX "")
    target_compile_options(${shader_NAME} PRIVATE "-Wno-unknown-attributes" "${shader_FLAGS}")

    #add_custom_command(OUTPUT ${MTD} COMMAND python ARGS ${CMAKE_SOURCE_DIR}/uigen.py ${UI} ${MTD} ${AE} ${AEXML} ${NEXML} ${SPDL} ${KARGS} ${CMAKE_CURRENT_BINARY_DIR} ${HTML} DEPENDS ${UI})
    #add_custom_target(${SHADER}UI ALL DEPENDS ${MTD})

    install(TARGETS ${shader_NAME} DESTINATION ${DSO_INSTALL_DIR})
    #install(FILES ${MTD} DESTINATION ${MTD_INSTALL_DIR})
    #install(FILES ${AE} DESTINATION ${AE_INSTALL_DIR})
    #install(FILES ${AEXML} DESTINATION ${AEXML_INSTALL_DIR})
    #install(FILES ${NEXML} DESTINATION ${NEXML_INSTALL_DIR})
    #install(FILES ${SPDL} DESTINATION ${SPDL_INSTALL_DIR})
    #install(FILES ${KARGS} DESTINATION ${ARGS_INSTALL_DIR})
    #install(FILES ${KARGS} DESTINATION ${ARGS_INSTALL_DIR})
    #install(DIRECTORY ${C4DRES} DESTINATION ${INSTALL_DIR})
endfunction(build_shader)

