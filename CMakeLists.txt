cmake_minimum_required(VERSION 3.14.0)
project(AirshipAce LANGUAGES ASM C)
set(ACE_DEBUG ON)
set(ACE_DEBUG_UAE ON)
#set(CMAKE_C_STANDARD 17)
file(GLOB_RECURSE SOURCES src/*.c main.c)

if(ELF2HUNK)
	set(AirshipAce_EXECUTABLE AirshipAce.elf)
	set(AirshipAce_OUTPUT_EXECUTABLE AirshipAce.exe)
	add_executable(${AirshipAce_EXECUTABLE} ${SOURCES})
	
	target_link_libraries(${AirshipAce_EXECUTABLE} -Wl,-Map=AirshipAce.map)
	
	add_custom_command(
		TARGET ${AirshipAce_EXECUTABLE} POST_BUILD
		COMMAND ${ELF2HUNK} ${AirshipAce_EXECUTABLE} ${AirshipAce_OUTPUT_EXECUTABLE}
	)
	add_custom_command(
		TARGET ${AirshipAce_EXECUTABLE} POST_BUILD
		COMMAND ${OBJDUMP} --disassemble -S ${AirshipAce_EXECUTABLE} > AirshipAce.s
	)
else()
	SET(AirshipAce_EXECUTABLE AirshipAce)
	SET(AirshipAce_OUTPUT_EXECUTABLE AirshipAce)
	
	add_executable(${AirshipAce_EXECUTABLE} ${SOURCES})
endif()

add_subdirectory(deps/ace ace)
message(STATUS "executable: ${AIRSHIP_EXECUTABLE}")
message(STATUS "src: ${RES_DIR}/Palettes/MarsT1.gpl")
message(STATUS "dst: ${DATA_DIR}/menupal.plt")
set(RES_DIR ${CMAKE_CURRENT_LIST_DIR}/res)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)
file(MAKE_DIRECTORY ${DATA_DIR})
convertPalette(${AirshipAce_EXECUTABLE} ${RES_DIR}/Palettes/MarsT1.gpl ${DATA_DIR}menupal.plt)
#convertBitmaps(${AirshipAce_EXECUTABLE} ${DATA_DIR}/menupal.plt ${RES_DIR}/ConceptArt/testBG2.png ${DATA_DIR}/menuBG.bm)

# Version stuff
string(TIMESTAMP YEAR "%y")
string(TIMESTAMP DAY "%d")
string(TIMESTAMP MONTH "%m")
MATH(EXPR VER_MAJOR "0 + ${YEAR}")
MATH(EXPR VER_MINOR "0 + ${MONTH}")
MATH(EXPR VER_FIX "0 + ${DAY}")
set(VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_FIX}")


target_compile_definitions(${AirshipAce_EXECUTABLE} PRIVATE AirshipAce_VERSION="${VERSION}")
include_directories(${SDL2_INCLUDE_DIRS} include/)
target_link_libraries(${AirshipAce_EXECUTABLE} ace )  
target_compile_options(${AirshipAce_EXECUTABLE} PUBLIC -Wall)


if(GAME_DEBUG)
	target_compile_definitions(${AirshipAce_EXECUTABLE} PRIVATE GAME_DEBUG)
	target_compile_definitions(ace PUBLIC ACE_DEBUG_ALL ACE_DEBUG_UAE)
endif()

set(RES_DIR ${CMAKE_CURRENT_LIST_DIR}/res)
set(DATA_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)


# Generating ZIP
set(AirshipAce_ZIP_NAME "${CMAKE_PROJECT_NAME} ${VER_MAJOR}_${VER_MINOR}_${VER_FIX}")
add_custom_target(generateZip COMMAND
	${CMAKE_COMMAND} -E tar "cf" "${AirshipAce_ZIP_NAME}.zip" --format=zip
	"${CMAKE_CURRENT_BINARY_DIR}/${AirshipAce_OUTPUT_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/myacefont.fnt"
	COMMENT "Generating ${AirshipAce_ZIP_NAME}.zip"
)
# Generating ADF
set(ADF_DIR "${CMAKE_CURRENT_BINARY_DIR}/adf")
add_custom_target(generateAdf
	COMMAND ${CMAKE_COMMAND} -E make_directory "${ADF_DIR}/s"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${AirshipAce_OUTPUT_EXECUTABLE}" "${ADF_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/myacefont.fnt" "${ADF_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${DATA_DIR}" "${ADF_DIR}/data"
	COMMAND ${CMAKE_COMMAND} -E echo "${AirshipAce_OUTPUT_EXECUTABLE}" > "${ADF_DIR}/s/startup-sequence"
	COMMAND exe2adf -l ${CMAKE_PROJECT_NAME} -a "${CMAKE_PROJECT_NAME}.adf" -d ${ADF_DIR}
	COMMAND ${CMAKE_COMMAND} -E rm -rf "${ADF_DIR}"
	COMMENT "Generating ADF file"
)