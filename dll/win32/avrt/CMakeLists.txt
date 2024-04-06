remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

spec2def(avrt.dll avrt.spec ADD_IMPORTLIB)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)

list(APPEND SOURCE
    avrt.c
    ${CMAKE_CURRENT_BINARY_DIR}/avrt_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/avrt.def)

add_library(avrt MODULE ${SOURCE})
set_module_type(avrt win32dll ENTRYPOINT DllMain 12)
target_link_libraries(avrt crt wine ${PSEH_LIB})

add_importlibs(avrt kernel32 ntdll crtdll user32)
add_cd_file(TARGET avrt DESTINATION reactos/system32 FOR all)
