
add_executable(formula-data-parser formula_data_parser.cpp)

target_link_libraries(formula-data-parser ${Boost_LIBRARIES} ${LIBORCUS_LDFLAGS})

install(TARGETS formula-data-parser
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

