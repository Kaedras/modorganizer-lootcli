cmake_minimum_required(VERSION 3.16)

# Qt is not required, this allows us to skip building the executable and only create
# the single-header interface when using this as a VCPKG dependency
# CHANGED: use a feature in vcpkg portfile for header-only, Qt6_FOUND is always true on some linux distros
if (NOT HEADER_ONLY)
find_package(Qt6 CONFIG COMPONENTS Core)

message(STATUS "Qt6 found, building lootcli executable")

if (UNIX)
	find_package(ICU REQUIRED COMPONENTS data uc)
	find_package(TBB REQUIRED)
	find_package(spdlog CONFIG REQUIRED)
	find_package(yaml-cpp CONFIG REQUIRED)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(tomlplusplus REQUIRED IMPORTED_TARGET tomlplusplus)
find_package(libloot CONFIG REQUIRED)
find_package(unofficial-curlpp CONFIG REQUIRED)
find_package(Boost REQUIRED CONFIG COMPONENTS locale)

# avoid CMake error/warning
set_target_properties(libloot::loot PROPERTIES
  MAP_IMPORTED_CONFIG_RELEASE RelWithDebInfo
  MAP_IMPORTED_CONFIG_MINSIZEREL RelWithDebInfo
)

if (WIN32)
	set(OS_SPECIFIC_DIR win32)
else()
	set(OS_SPECIFIC_DIR linux)
endif()

add_executable(lootcli WIN32)
set_target_properties(lootcli PROPERTIES
	CXX_STANDARD 20
	WIN32_EXECUTABLE TRUE)
target_sources(lootcli
	PRIVATE
		game_settings.cpp
		game_settings.h
		lootthread.cpp
		lootthread.h
		${OS_SPECIFIC_DIR}/main.cpp
		pch.h
		version.h
		version.rc
		${CMAKE_CURRENT_SOURCE_DIR}/../include/lootcli/lootcli.h
)
target_compile_definitions(lootcli
	PRIVATE
	_UNICODE UNICODE
	_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
target_include_directories(lootcli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_precompile_headers(lootcli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch.h)

target_link_libraries(lootcli
    PRIVATE libloot::loot Boost::headers Boost::locale
	PkgConfig::tomlplusplus Qt6::Core unofficial::curlpp::curlpp)

if (MSVC)
	target_compile_options(lootcli
		PRIVATE
		"/MP"
		"/W4"
		"/external:anglebrackets"
		"/external:W0"
	)
	target_link_options(lootcli
		PRIVATE
		$<$<CONFIG:RelWithDebInfo>:/LTCG /INCREMENTAL:NO /OPT:REF /OPT:ICF>
	)
	target_compile_definitions(lootcli PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

	set_target_properties(lootcli PROPERTIES VS_STARTUP_PROJECT lootcli)
else()
	target_compile_options(lootcli
		PRIVATE
		"-Wall"
		"-Wextra"
	)
endif()

install(FILES
	$<TARGET_FILE:lootcli>
	$<TARGET_FILE:libloot::loot>
DESTINATION bin/loot)

endif()

# library to make the header available
add_library(lootcli-header INTERFACE)
target_sources(lootcli-header INTERFACE
	FILE_SET HEADERS
	BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../include
	FILES ${CMAKE_CURRENT_LIST_DIR}/../include/lootcli/lootcli.h)
add_library(mo2::lootcli-header ALIAS lootcli-header)

install(TARGETS lootcli-header EXPORT lootcliHeaderTargets FILE_SET HEADERS)
install(EXPORT lootcliHeaderTargets
	FILE mo2-lootcli-header-targets.cmake
	NAMESPACE mo2::
	DESTINATION lib/cmake/mo2-lootcli-header
)
