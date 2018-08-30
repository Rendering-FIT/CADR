set(PACKAGES
    tut01-deviceList
    tut03-helloWindowX11
   )

foreach(pkg ${PACKAGES})
  add_subdirectory(${pkg})
endforeach()
