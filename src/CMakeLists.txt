cmake_minimum_required(VERSION 3.16)

add_executable(lootcli WIN32)

mo2_configure_executable(lootcli
	WARNINGS OFF
	TRANSLATIONS OFF
	PRIVATE_DEPENDS boost Qt::Core loot tomlplusplus curlpp)
target_include_directories(lootcli
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

mo2_install_target(lootcli INSTALLDIR bin/loot)

if (UNIX)
	# set runtime path to "."
	target_compile_options(lootcli PRIVATE "-Wl,-R,.")
endif()
