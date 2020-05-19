
add_executable(formula-data-parser formula_data_parser.cpp)

add_executable(collect-tokens collect_tokens.cpp)

target_link_libraries(formula-data-parser ${Boost_LIBRARIES} ${LIBORCUS_LDFLAGS} ${LIBIXION_LDFLAGS})
target_link_libraries(collect-tokens ${Boost_LIBRARIES} ${LIBORCUS_LDFLAGS})

install(TARGETS formula-data-parser collect-tokens
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

