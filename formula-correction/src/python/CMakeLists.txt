
add_library(_orcus_ml_formula_correction MODULE python.cpp)

target_include_directories(_orcus_ml_formula_correction PUBLIC ${Python3_INCLUDE_DIRS})
set_target_properties(_orcus_ml_formula_correction PROPERTIES PREFIX "")

install(TARGETS _orcus_ml_formula_correction LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR})
