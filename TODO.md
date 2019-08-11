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
- [x] convert resource file into 1 binary file
- [ ] deserialization from RAM, not file
- [ ] avoid copies during deserialization if freeCpuCopy == true
- [ ] LZ4 compress / decompress resource file
- [ ] Resource file loading from "my changes" workflow
- [ ] skybox loading
- [ ] Change the resource DB types so that you dont have to resize by hand / forget
- [ ] make sure all memory is properly released on failure cases in resource loadings

### Meshes:
- [ ] Add support for 16 bit indices
- [ ] Tangents and bitangents
- [ ] MTL file edits dont re-optimize the mesh
- [ ] Automatic LOD generation
- [ ] Specify topology?

### Textures:
- [ ] Compressed formats
- [ ] Mipmap generation

## Rendering
- [ ] Expand graphics api so there are 0 opengl calls otherwise
- [ ] Meshes easily
- [ ] Frustum culling
- [ ] Shadows
- [ ] Load screen
- [ ] Normal maps
- [ ] PBR
- [ ] Bloom
- [ ] Filmic tonemapping
- [ ] LOD system
- [ ] compress render target and vertex data

## Audio
- [ ] Get basic sounds clips working, no streaming (linux)
- [ ] Background music + positional clips working (linux)
- [ ] Get working on linux and windows

## Core
- [ ] switch to using ECS
- [ ] imgui
- [ ] add in Lua scripting
- [x] multi-threaded logger

## Other
- [ ] have a remote console
- [x] Have a full converter for each resource type and fastfiles
- [ ] have a defines file instead of just common
- [ ] using #includes in the glsl
- [ ] Debug, Release, Ship modes
- [x] Get rid of the config initialization style when dumb, like logger

## Low priority software engineering related
- [x] follow style / clang tidy
