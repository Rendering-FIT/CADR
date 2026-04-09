# SPDX-FileCopyrightText: 2018-2025 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
#
# SPDX-License-Identifier: MIT

set(PACKAGES
	CadR
	CadPL
	)

foreach(pkg ${PACKAGES})
	add_subdirectory(${pkg})
endforeach()
