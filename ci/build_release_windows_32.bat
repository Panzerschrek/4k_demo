call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" amd64_x86
mkdir build_dir
cmake -S . -B build_dir -G "Ninja" -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build_dir
