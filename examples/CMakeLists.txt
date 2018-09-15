set(PACKAGES
	tut01-deviceList
	tut04-swapChain
	)
if(UNIX)
	list(INSERT PACKAGES 1 tut03-helloWindowX11)
elseif(WIN32)
	list(INSERT PACKAGES 1 tut03-helloWindowWin)
endif()

foreach(pkg ${PACKAGES})
	add_subdirectory(${pkg})
endforeach()
