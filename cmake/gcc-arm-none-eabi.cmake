set(CMAKE_SYSTEM_NAME               Generic)
set(CMAKE_SYSTEM_PROCESSOR          arm)

set(CMAKE_C_COMPILER_ID GNU)
set(CMAKE_CXX_COMPILER_ID GNU)

# Some default GCC settings
# arm-none-eabi- must be part of path environment
set(TOOLCHAIN_PREFIX                arm-none-eabi-)

set(CMAKE_C_COMPILER                ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER              ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER              ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_LINKER                    ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_OBJCOPY                   ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE                      ${TOOLCHAIN_PREFIX}size)

set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# MCU specific flags
set(TARGET_FLAGS "-mcpu=cortex-m3 ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Wno-unused-parameter -fdata-sections -ffunction-sections")

# Debug флаги с поддержкой отладки и оптимизацией размера
set(CMAKE_C_FLAGS_DEBUG "-Oz -g3 -ffunction-sections -fdata-sections -fno-stack-protector")
# -Oz:                  максимальная оптимизация размера (приоритет размеру над скоростью)
# -g3:                  максимальная отладочная информация (включая макросы)
# -ffunction-sections:  размещать каждую функцию в отдельной секции (для --gc-sections)
# -fdata-sections:      размещать каждую переменную в отдельной секции (для --gc-sections)
# -fno-stack-protector: отключить защиту стека (экономия кода и производительности)
# Убрали: -DNDEBUG (для работы assert), -fomit-frame-pointer (для корректной отладки)

# Release флаги оптимизированы для минимального размера Flash памяти
set(CMAKE_C_FLAGS_RELEASE "-Os -g0 -ffunction-sections -fdata-sections -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer")
# -Os:                           оптимизация размера (размер приоритетнее скорости)
# -g0:                           отключить генерацию отладочной информации
# -ffunction-sections:           размещать каждую функцию в отдельной секции (для --gc-sections)
# -fdata-sections:               размещать каждую переменную в отдельной секции (для --gc-sections)
# -fno-stack-protector:          отключить защиту стека (экономия кода и производительности)
# -fno-unwind-tables:            отключить таблицы раскрутки стека (экономия Flash)
# -fno-asynchronous-unwind-tables: отключить асинхронные таблицы раскрутки (экономия Flash)
# -fomit-frame-pointer:          убрать указатели фреймов для экономии кода

set(CMAKE_CXX_FLAGS_DEBUG "-Oz -g3 -ffunction-sections -fdata-sections -fno-stack-protector") # Включена отладочная информация для Debug сборки
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0 -ffunction-sections -fdata-sections -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer") # Оптимизация размера для Release сборки

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics")

set(CMAKE_C_LINK_FLAGS "${TARGET_FLAGS}")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32F103C4TX_FLASH.ld\"")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} --specs=nano.specs")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lc -lm -Wl,--end-group")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--print-memory-usage")

# Debug-specific link flags (preserve debug symbols)
set(CMAKE_C_LINK_FLAGS_DEBUG "${CMAKE_C_LINK_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS_DEBUG "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lstdc++ -lsupc++ -Wl,--end-group")

# Release-specific link flags (strip debug symbols and optimize for size)
set(CMAKE_C_LINK_FLAGS_RELEASE "${CMAKE_C_LINK_FLAGS} -Wl,--strip-all -Wl,--strip-debug -Wl,--discard-all -Wl,--discard-locals")

set(CMAKE_CXX_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lstdc++ -lsupc++ -Wl,--end-group")
