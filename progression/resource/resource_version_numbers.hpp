#pragma once

#define PG_RESOURCE_IMAGE_VERSION       3  // Samplers names changed

#define PG_RESOURCE_MATERIAL_VERSION    3  // Got rid of Ka and Ke

#define PG_RESOURCE_MODEL_VERSION       2  // Adding tangents

#define PG_RESOURCE_SCRIPT_VERSION      0  // Initial version

#define PG_RESOURCE_SHADER_VERSION      1  // At least for now, marking all descriptor bindings to be available in all shader stages


#define PG_RESOURCE_MAGIC_NUMBER_GUARD ( 1452909455llu << 32 | 2667396458llu )