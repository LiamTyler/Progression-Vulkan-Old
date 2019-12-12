# Progression [![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

## Build status
- Linux build: [![Build Status](https://travis-ci.org/LiamTyler/Progression.svg?branch=FPS-in-city)](https://travis-ci.org/LiamTyler/Progression)
- Windows build: [![Build status](https://ci.appveyor.com/api/projects/status/3badv9456nqrow5f?svg=true)](https://ci.appveyor.com/project/LiamTyler/progression)

## Description
A C++ game engine I have been developing for Linux and Windows. See [here](https://liamtyler.github.io/portfolio/Progression/) for more details.

## Some Key Features
- Vulkan rendering system
- Deferred rendering pipeline
- SSAO
- Normal mapping
- Lua scripting system
- Skeletal animation system
- Serialized asset pipeline system
- Entity component system

## Installing + Building
This engine requires C++ 2017, and has only been tested on MSVC 2019 and GCC 9
```
git clone --recursive https://github.com/LiamTyler/Progression.git
cd Progression 
mkdir build
cd build
(linux) cmake -DCMAKE_BUILD_TYPE=[Debug/Release/Ship] ..
(windows) cmake -G "Visual Studio 16 2016" -A x64 ..
```

Note with building with gcc: gcc actually seems to timeout while doing a max parallel build of assimp with "make -j", so I do have to do "make -j6" instead
