call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" amd64
mkdir build_dir
cmake -S . -B build_dir -G "Ninja" -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build_dir
