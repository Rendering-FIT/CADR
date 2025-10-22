set(PACKAGES
    CadR
    CadPL
   )

foreach(pkg ${PACKAGES})
	add_subdirectory(${pkg})
endforeach()
