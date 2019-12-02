#pragma once

#define PG_RESOURCE_IMAGE_VERSION       3  // Samplers names changed

#define PG_RESOURCE_MATERIAL_VERSION    2  // Fixed image names for unmanaged textures in material::serialize

#define PG_RESOURCE_MODEL_VERSION       2  // Adding tangents

#define PG_RESOURCE_SCRIPT_VERSION      0  // Initial version

#define PG_RESOURCE_SHADER_VERSION      0  // Initial version


#define PG_RESOURCE_MAGIC_NUMBER_GUARD ( 1452909455llu << 32 | 2667396458llu )