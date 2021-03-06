##############################################################################
# OpenSplice_IDLGEN(idlfilename)
#
# Macro to generate OpenSplice DDS sources from a given idl file with the 
# data structures.
# You must include the extension .idl in the name of the data file.
#
##############################################################################
# Courtersy of Ivan Galvez Junquera <ivgalvez@gmail.com>
##############################################################################


# Macro to create a list with all the generated source files for a given .idl filename
MACRO (DEFINE_OpenSplice_SOURCES idlfilename)
    GET_FILENAME_COMPONENT(dir ${idlfilename} DIRECTORY)
    GET_FILENAME_COMPONENT(nfile ${idlfilename} NAME_WE)
    SET(outsources
      ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${nfile}.cpp 
      ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${nfile}.h
      ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${nfile}Dcps.cpp 
      ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${nfile}Dcps.h
      ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${nfile}Dcps_impl.cpp 
      ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${nfile}Dcps_impl.h
      ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${nfile}SplDcps.cpp 
      ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${nfile}SplDcps.h
      ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${nfile}_DCPS.hpp
    )
ENDMACRO(DEFINE_OpenSplice_SOURCES)

MACRO (OpenSplice_IDLGEN idlfilename)
    GET_FILENAME_COMPONENT(it ${idlfilename} ABSOLUTE)
    GET_FILENAME_COMPONENT(idlfilename ${idlfilename} NAME)
    DEFINE_OpenSplice_SOURCES(${idlfilename})
    ADD_CUSTOM_COMMAND (
        OUTPUT ${outsources}
        COMMAND LD_LIBRARY_PATH=$ENV{OSPL_BASE}/lib PATH=$ENV{OSPL_BASE}/bin OSPL_TMPL_PATH=$ENV{OSPL_BASE}/etc/idlpp ${OpenSplice_IDLGEN_BINARY} 
        ARGS -l isocpp -d ${CMAKE_CURRENT_BINARY_DIR}/${dir} ${it}
        DEPENDS ${idlfilename}
    )
ENDMACRO (OpenSplice_IDLGEN)