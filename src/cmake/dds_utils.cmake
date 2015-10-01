macro(COMPILE_IDL _idl_file)

    # Get only the file name, without prefix and suffix
    get_filename_component(_idl ${_idl_file} NAME_WE)
    get_filename_component(_dir ${_idl_file} DIRECTORY)

    message(${_dir})

    # First command compiles base IDL into xTypeSupport.idl
    set(_idl_src ${_dir}/${_idl}TypeSupportImpl.cpp ${_dir}/${_idl}TypeSupportImpl.h)
    set(_idl_idl ${_dir}/${_idl}TypeSupport.idl)
    add_custom_command(OUTPUT ${_idl_src} ${_idl_idl}
        COMMAND DDS_ROOT=${DDS_ROOT} LD_LIBRARY_PATH=${DDS_ROOT}/lib:${ACE_ROOT}/lib ${DDS_ROOT}/bin/opendds_idl ${_idl_file}
        WORKING_DIRECTORY ${_dir}
        DEPENDS ${_idl_file}
        COMMENT "Compiling ${_idl_file}" VERBATIM)

    # First command compiles base IDL
    set(_idl_imp_src ${_dir}/${_idl}TypeSupportC.cpp  ${_dir}/${_idl}TypeSupportC.h  ${_dir}/${_idl}TypeSupportS.cpp  ${_dir}/${_idl}TypeSupportS.h)
    add_custom_command(OUTPUT ${_idl_imp_src}
        COMMAND DDS_ROOT=${DDS_ROOT} TAO_ROOT=${ACE_ROOT} ACE_ROOT=${ACE_ROOT} LD_LIBRARY_PATH=${DDS_ROOT}/lib:${ACE_ROOT}/lib ${ACE_ROOT}/bin/tao_idl -I${DDS_ROOT} -I${TAO_ROOT}/orbsvcs ${_idl_idl}
        WORKING_DIRECTORY ${_dir}
        DEPENDS ${_idl_src}
        COMMENT "Compiling ${_idl_file}" VERBATIM)

    # Second command compiles TypeSupport.idl
    set(_idl_msg_src ${_dir}/${_idl}C.cpp ${_dir}/${_idl}C.h ${_dir}/${_idl}S.cpp ${_dir}/${_idl}S.h)
    add_custom_command(OUTPUT ${_idl_msg_src}
        COMMAND DDS_ROOT=${DDS_ROOT} TAO_ROOT=${ACE_ROOT} ACE_ROOT=${ACE_ROOT} LD_LIBRARY_PATH=${DDS_ROOT}/lib:${ACE_ROOT}/lib ${ACE_ROOT}/bin/tao_idl -I${DDS_ROOT} -I${TAO_ROOT}/orbsvcs ${_idl_file}
        WORKING_DIRECTORY ${_dir}
        DEPENDS ${_idl_src}
        COMMENT "Compiling ${_idl_file}" VERBATIM)

    # Concat all sources together
    set(ALL_IDL_SRCS ${_idl_src} ${_idl_imp_src} ${_idl_msg_src})

endmacro(COMPILE_IDL)

macro(COMPILE_IDL_FILES)
    set(idl_srcs)
    foreach(idl ${ARGN})
        COMPILE_IDL(${idl})
    endforeach(idl)
endmacro(COMPILE_IDL_FILES)
