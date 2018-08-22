set(PACKAGES
    tut01-deviceList
   )

foreach(pkg ${PACKAGES})
  add_subdirectory(${pkg})
endforeach()
