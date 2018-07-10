set(PACKAGES
    CADR
    )

foreach(pkg ${PACKAGES})
  add_subdirectory(${pkg})
endforeach()
