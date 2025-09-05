set(PACKAGES
    CadR
    CadText
   )

foreach(pkg ${PACKAGES})
  add_subdirectory(${pkg})
endforeach()
