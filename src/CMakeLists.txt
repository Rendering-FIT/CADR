set(PACKAGES
    CadR
   )

foreach(pkg ${PACKAGES})
  add_subdirectory(${pkg})
endforeach()
