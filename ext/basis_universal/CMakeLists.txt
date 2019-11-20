project(basisu)

cmake_minimum_required(VERSION 3.0)

set(BASISU_SRC_LIST ${COMMON_SRC_LIST} 
	basisu_backend.cpp
	basisu_basis_file.cpp
	basisu_comp.cpp
	basisu_enc.cpp
	basisu_etc.cpp
	basisu_frontend.cpp
	basisu_global_selector_palette_helpers.cpp
	basisu_gpu_texture.cpp
	basisu_pvrtc1_4.cpp
	basisu_resampler.cpp
	basisu_resample_filters.cpp
	basisu_ssim.cpp
	basisu_tool.cpp
	basisu_astc_decomp.cpp
	lodepng.cpp
	transcoder/basisu_transcoder.cpp
    
    basisu_backend.h
	basisu_basis_file.h
	basisu_comp.h
	basisu_enc.h
	basisu_etc.h
	basisu_frontend.h
	basisu_global_selector_palette_helpers.h
	basisu_gpu_texture.h
	basisu_pvrtc1_4.h
	basisu_resampler.h
	basisu_resampler_filters.h
	basisu_ssim.h
	basisu_astc_decomp.h
	lodepng.h
	transcoder/basisu_transcoder.h
	transcoder/basisu.h
	transcoder/basisu_global_selector_cb.h
	transcoder/basisu_global_selector_palette.h
    
)

#add_executable(basisu ${BASISU_SRC_LIST})
add_library(basisu_lib ${BASISU_SRC_LIST})