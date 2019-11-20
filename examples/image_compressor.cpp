#include "progression.hpp"
#include "basis_universal/basisu_comp.h"
#include "stb_image/stb_image.h"
#include <thread>

using namespace Progression;

int main( int argc, char* argv[] )
{
    std::string filename = PG_RESOURCE_DIR "textures/brick.png";

    stbi_set_flip_vertically_on_load( true );
    int width, height, numComponents;
    unsigned char* pixels = stbi_load( filename.c_str(), &width, &height, &numComponents, 4 );
    LOG( "Num components = ", numComponents );

    if ( !pixels )
    {
        LOG_ERR( "Failed to load image '", filename, "'" );
        return 1;
    }

    basisu::image image( width, height );
    for ( int r = 0; r < height; ++r )
    {
        for ( int c = 0; c < width; ++c )
        {
            int p = 4 * ( r * width + c );
            image( c, r ).r = pixels[p + 0];
            image( c, r ).g = pixels[p + 1];
            image( c, r ).b = pixels[p + 2];
            image( c, r ).a = pixels[p + 3];
        }
    }

    free( pixels );

    basisu::basis_compressor_params params;

    uint32_t numThreads = std::max( 1u, std::thread::hardware_concurrency() );
    basisu::job_pool jobPool( numThreads );
    params.m_pJob_pool = &jobPool;
    params.m_source_images.emplace_back( std::move( image ) );
    params.m_out_filename             = "tmp.basis";
    params.m_write_output_basis_files = true;
    // params.m_source_filenames         = { filename };
    // params.m_read_source_images       = true;
    params.m_mip_gen                  = false;
    params.m_quality_level            = 128;
    basisu::basis_compressor compressor;
    if ( !compressor.init( params ) )
    {
        LOG_ERR( "Could not initialize compressor" );
        return 1;
    }
    auto err = compressor.process();
    if ( err != basisu::basis_compressor::error_code::cECSuccess )
    {
        LOG_ERR( "Could not encode the image to basis file" );
        return 1;
    }

    LOG( "Encoder successful" );



    // transcode
    basist::basisu_transcoder_init();
    basist::etc1_global_selector_codebook sel_codebook(basist::g_global_selector_cb_size, basist::g_global_selector_cb);

    const basist::transcoder_texture_format transcoder_tex_fmt = basist::transcoder_texture_format::cTFBC1;

	PG_ASSERT( !basist::basis_transcoder_format_is_uncompressed(transcoder_tex_fmt) );

    std::vector< uint8_t > basis_data;
	if ( !basisu::read_file_to_vec( params.m_out_filename.c_str(), basis_data ) )
	{
		LOG_ERR( "Failed reading file \"", params.m_out_filename, "\"" );
		return false;
	}

	if ( !basis_data.size() )
	{
		LOG_ERR( "File is empty!" );
		return 1;
	}

	if ( basis_data.size() > UINT32_MAX )
	{
		LOG_ERR( "File is too large!" );
		return 1;
	}

    basist::basisu_transcoder dec( &sel_codebook );

    basist::basisu_file_info fileinfo;
	if ( !dec.get_file_info( &basis_data[0], (uint32_t)basis_data.size(), fileinfo ) )
	{
		LOG_ERR( "Failed retrieving Basis file information!" );
		return 1;
	}

	assert( fileinfo.m_total_images == fileinfo.m_image_mipmap_levels.size() );
	assert( fileinfo.m_total_images == dec.get_total_images( &basis_data[0], (uint32_t)basis_data.size() ) );

	printf( "File info:\n" );
	printf( "  Version: %X\n", fileinfo.m_version );
	printf( "  Total header size: %u\n", fileinfo.m_total_header_size );
	printf( "  Total selectors: %u\n", fileinfo.m_total_selectors );
	printf( "  Selector codebook size: %u\n", fileinfo.m_selector_codebook_size );
	printf( "  Total endpoints: %u\n", fileinfo.m_total_endpoints );
	printf( "  Endpoint codebook size: %u\n", fileinfo.m_endpoint_codebook_size );
	printf( "  Tables size: %u\n", fileinfo.m_tables_size );
	printf( "  Slices size: %u\n", fileinfo.m_slices_size );
	printf( "  Texture type: %s\n", basist::basis_get_texture_type_name( fileinfo.m_tex_type ) );
	printf( "  us per frame: %u (%f fps)\n", fileinfo.m_us_per_frame, fileinfo.m_us_per_frame ? (1.0f / ( (float) fileinfo.m_us_per_frame / 1000000.0f ) ) : 0.0f );
	printf( "  Total slices: %u\n", (uint32_t)fileinfo.m_slice_info.size() );
	printf( "  Total images: %i\n", fileinfo.m_total_images );
	printf( "  Y Flipped: %u, Has alpha slices: %u\n", fileinfo.m_y_flipped, fileinfo.m_has_alpha_slices );
	printf( "  userdata0: 0x%X userdata1: 0x%X\n", fileinfo.m_userdata0, fileinfo.m_userdata1 );
	printf( "  Per-image mipmap levels: " );
	for (uint32_t i = 0; i < fileinfo.m_total_images; i++)
		printf("%u ", fileinfo.m_image_mipmap_levels[i]);
	printf("\n");

	printf("\nImage info:\n");
	for (uint32_t i = 0; i < fileinfo.m_total_images; i++)
	{
		basist::basisu_image_info ii;
		if (!dec.get_image_info(&basis_data[0], (uint32_t)basis_data.size(), ii, i))
		{
			LOG_ERR( "get_image_info() failed!" );
			return false;
		}

		printf("Image %u: MipLevels: %u OrigDim: %ux%u, BlockDim: %ux%u, FirstSlice: %u, HasAlpha: %u\n", i, ii.m_total_levels, ii.m_orig_width, ii.m_orig_height,
			ii.m_num_blocks_x, ii.m_num_blocks_y, ii.m_first_slice_index, (uint32_t)ii.m_alpha_flag);
	}

	printf("\nSlice info:\n");
	for (uint32_t i = 0; i < fileinfo.m_slice_info.size(); i++)
	{
		const basist::basisu_slice_info& sliceinfo = fileinfo.m_slice_info[i];
		printf("%u: OrigWidthHeight: %ux%u, BlockDim: %ux%u, TotalBlocks: %u, Compressed size: %u, Image: %u, Level: %u, UnpackedCRC16: 0x%X, alpha: %u, iframe: %i\n",
			i,
			sliceinfo.m_orig_width, sliceinfo.m_orig_height,
			sliceinfo.m_num_blocks_x, sliceinfo.m_num_blocks_y,
			sliceinfo.m_total_blocks,
			sliceinfo.m_compressed_size,
			sliceinfo.m_image_index, sliceinfo.m_level_index,
			sliceinfo.m_unpacked_slice_crc16,
			(uint32_t)sliceinfo.m_alpha_flag,
			(uint32_t)sliceinfo.m_iframe_flag);
	}
	printf("\n");

    uint32_t total_unpack_warnings = 0;
	uint32_t total_pvrtc_nonpow2_warnings = 0;

	for ( uint32_t image_index = 0; image_index < fileinfo.m_total_images; image_index++ )
	{
		for (uint32_t level_index = 0; level_index < fileinfo.m_image_mipmap_levels[image_index]; level_index++)
		{
			basist::basisu_image_level_info level_info;

			if (!dec.get_image_level_info(&basis_data[0], (uint32_t)basis_data.size(), level_info, image_index, level_index))
			{
				LOG_ERR("Failed retrieving image level information (%u %u)!\n", image_index, level_index);
				return false;
			}
										
			if ((transcoder_tex_fmt == basist::transcoder_texture_format::cTFPVRTC1_4_RGB) || (transcoder_tex_fmt == basist::transcoder_texture_format::cTFPVRTC1_4_RGBA))
			{
				if (!basisu::is_pow2(level_info.m_width) || !basisu::is_pow2(level_info.m_height))
				{
					total_pvrtc_nonpow2_warnings++;

					printf("Warning: Will not transcode image %u level %u res %ux%u to PVRTC1 (one or more dimension is not a power of 2)\n", image_index, level_index, level_info.m_width, level_info.m_height);

					// Can't transcode this image level to PVRTC because it's not a pow2 (we're going to support transcoding non-pow2 to the next larger pow2 soon)
					continue;
				}
			}

			basisu::texture_format tex_fmt = basis_get_basisu_texture_format(transcoder_tex_fmt);

			basisu::gpu_image& gi = gpu_images[(int)transcoder_tex_fmt][image_index][level_index];
			gi.init(tex_fmt, level_info.m_orig_width, level_info.m_orig_height);

			// Fill the buffer with psuedo-random bytes, to help more visibly detect cases where the transcoder fails to write to part of the output.
			basisu::fill_buffer_with_random_bytes(gi.get_ptr(), gi.get_size_in_bytes());

			uint32_t decode_flags = 0;
														
			if (!dec.transcode_image_level(&basis_data[0], (uint32_t)basis_data.size(), image_index, level_index, gi.get_ptr(), gi.get_total_blocks(), transcoder_tex_fmt, decode_flags))
			{
				error_printf("Failed transcoding image level (%u %u %u)!\n", image_index, level_index, format_iter);
				return false;
			}

			printf("Transcode of image %u level %u res %ux%u format %s succeeded in %3.3f ms\n", image_index, level_index, level_info.m_orig_width, level_info.m_orig_height, basist::basis_get_format_name(transcoder_tex_fmt), total_transcode_time);

		} // format_iter

	} // level_index

    if ( total_pvrtc_nonpow2_warnings )
    {
		LOG_ERR( "Warning: ", total_pvrtc_nonpow2_warnings, " images could not be transcoded to PVRTC1 because one or both dimensions were not a power of 2" );
    }

	if ( total_unpack_warnings )
    {
		LOG_ERR( "ATTENTION: ", total_unpack_warnings, " total images had invalid GPU texture data!" );
    }
	else
    {
		LOG( "Transcoding success\n" );
    }

    return 0;
}
