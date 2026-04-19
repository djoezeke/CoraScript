# ===================================================================
# Copyright © 2025 Sackey Ezekiel Etrue
# Distributed under the terms of the MIT license.
# ===================================================================

set(MODULES_SOURCES 

    "${MODULES_DIR}/io.cpp"
)

#--------------------------------------------------------------------
# Build Modules Library.
#--------------------------------------------------------------------

add_library(Modules STATIC ${MODULES_SOURCES})

add_library(Cora::Modules ALIAS Modules)

target_include_directories(Modules
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include
)

if(TARGET Cora::Compiler)
	target_link_libraries(Modules
		PUBLIC
			Cora::Compiler
	)
endif()

# ===================================================================