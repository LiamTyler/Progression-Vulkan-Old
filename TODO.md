# TODO

## Resource related
- [x] make sure image loading is working
- [x] make sure image saving is working
- [x] texture2D refactor test (including the internal formats
- [x] save shader binary
- [x] load shader binary
- [x] model loading
- [x] model loading remove duplicate verts
- [x] mesh optimizer
- [x] material loader
- [x] resource manager
- [x] loading resource text file
- [x] re-use materials when loading models if its already been loaded
- [x] detect changed files + reload them
- [ ] convert resource file into 1 binary file
- [ ] compress / decompress resource file
- [ ] skybox loading
- [ ] select different data types for mesh data (like shorts for indices)
- [ ] select different mesh topology, like rendering with no index buffer, lines, etc
- [ ] Texture Compression (at least BC7)
- [ ] offline mipmap generation
- [ ] Change the resource DB types so that you dont have to resize by hand / forget
- [ ] make sure all memory is properly released on failure cases in resource loadings
- [ ] automatic LOD creation for models
- [ ] support per vertex colors
- [ ] easy system for dynamic meshes

## Rendering
- [ ] Meshes easily
- [ ] Shadows
- [ ] Normal maps
- [ ] PBR
- [ ] LOD system
- [ ] compress textures and vertex data

## Core
- [ ] switch to using ECS
- [ ] imgui
- [ ] add in Lua scripting
- [ ] multi-threaded logger

## Low priority software engineering related
- [ ] follow style / clang tidy
- [ ] doxygen comments fully
