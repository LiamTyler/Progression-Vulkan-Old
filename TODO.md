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
- [ ] detect changed files + reload them
- [ ] convert resource file into 1 binary file
- [ ] compress / decompress resource file
- [ ] skybox loading
- [ ] support per vertex colors
- [ ] select different data types for mesh data (like shorts for indices)
- [ ] select different mesh topology, like rendering with no index buffer, lines, etc
- [ ] easy system for dynamic meshes
- [ ] make sure all memory is properly released on failure cases in resource loadings
- [ ] figure out the if really need to pass around the freeCPUCopy all the time

## Rendering
- [ ] Meshes easily
- [ ] Shadows
- [ ] Normal maps
- [ ] PBR
- [ ] LOD system
- [ ] compress textures and vertex data

## Core
- [ ] switch to using ECS
- [ ] add in Lua scripting

## Low priority software engineering related
- [ ] follow clang tidy
- [ ] doxygen comments fully