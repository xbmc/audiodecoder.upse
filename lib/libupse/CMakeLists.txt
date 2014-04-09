project(libupse)

enable_language(C)

cmake_minimum_required(VERSION 2.6)

set(SOURCES upse_bios_manager.c
            upse_container_xsf.c
            upse_eventloop.c
            upse_filesystem.c
            upse_loader.c
            upse_loader_psf1.c
            upse_loader_psf2.c
            upse_module.c
            upse_ps1_bios_base.c
            upse_ps1_counters.c
            upse_ps1_dma_manager.c
            upse_ps1_executive.c
            upse_ps1_gpu.c
            upse_ps1_hal.c
            upse_ps1_memory_manager.c
            upse_ps1_spu_base.c
            upse_ps1_spu_dma_handler.c
            upse_ps1_spu_lowpass_filter.c
            upse_ps1_spu_nyquist_filter.c
            upse_ps1_spu_register_io.c
            upse_ps2_iop_base.c
            upse_r3000_abstract.c
            upse_r3000_bytecode_evaluator.c
            upse_string.c
            upse_util.c
            Neill/spu.c
            Neill/spucore.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

add_definitions(-DEMU_COMPILE -DEMU_LITTLE_ENDIAN)

add_library(upse STATIC ${SOURCES})
