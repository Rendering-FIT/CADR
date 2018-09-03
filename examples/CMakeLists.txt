set(PACKAGES
	tut01-deviceList
	)
if(UNIX)
	set(PACKAGES ${PACKAGES} tut03-helloWindowX11)
elseif(WIN32)
	set(PACKAGES ${PACKAGES} tut03-helloWindowWin)
endif()

foreach(pkg ${PACKAGES})
	add_subdirectory(${pkg})
endforeach()
