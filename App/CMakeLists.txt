cmake_minimum_required(VERSION 3.14)

# Set application data
set(APP "Microvisor Weather Demo")
set(VERSION_NUMBER "3.0.0")
set(BUILD_NUMBER "0")

message("Building ${APP} ${VERSION_NUMBER} build ${BUILD_NUMBER}")

# Make app data available to the compiler
add_compile_definitions(APP_NAME="${APP}")
add_compile_definitions(APP_VERSION="${VERSION_NUMBER}")
add_compile_definitions(BUILD_NUM=${BUILD_NUMBER})

# Load in and use project env vars
if(DEFINED ENV{MVOW_API_KEY})
    add_compile_definitions(API_KEY="$ENV{MVOW_API_KEY}")
else()
    add_compile_definitions(API_KEY="UNDEFINED")
endif()

if(DEFINED ENV{MVOW_LAT})
    add_compile_definitions(LATITUDE=$ENV{MVOW_LAT})
else()
    add_compile_definitions(LATITUDE=51.52188334146633)
endif()

if(DEFINED ENV{MVOW_LNG})
    add_compile_definitions(LONGITUDE=$ENV{MVOW_LNG})
else()
    add_compile_definitions(LONGITUDE=-0.10347583821287779)
endif()

# Compile app source code file(s)
add_executable(${PROJECT_NAME}
    main.c
    cJSON.c
    ht16k33-matrix.c
    http.c
    i2c.c
    logging.c
    uart_logging.c
    openweather.c
    stm32u5xx_hal_timebase_tim_template.c
)

# Link built libraries
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC
    ST_Code
    twilio-microvisor-hal-stm32u5
    FreeRTOS)

# Optional informational and additional format generation
# NOTE From 2.0.3, this generates an alternative .bin file
#      than was previously the case
add_custom_command(OUTPUT EXTRA_FILES DEPENDS ${PROJECT_NAME}
    COMMAND mv "${PROJECT_NAME}" "${PROJECT_NAME}.elf"
    COMMAND ${CMAKE_SIZE} --format=berkeley "${PROJECT_NAME}.elf"
    COMMAND ${CMAKE_OBJDUMP} -h -S "${PROJECT_NAME}.elf" > "${PROJECT_NAME}.list"
    COMMAND ${CMAKE_OBJCOPY} --output-target ihex "${PROJECT_NAME}.elf" "${PROJECT_NAME}.hex"
    COMMAND ${CMAKE_OBJCOPY} --input-target ihex --output-target binary --gap-fill 0xFF "${PROJECT_NAME}.hex" "${PROJECT_NAME}.bin"
)

# Prepare the additional files
add_custom_target(extras ALL DEPENDS EXTRA_FILES)
