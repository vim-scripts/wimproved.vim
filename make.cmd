if not exist Build32 mkdir Build32
if not exist Build64 mkdir Build64

cd Build32
cmake -G "Visual Studio 11 2012" ..
cd ..

cd Build64
cmake -G "Visual Studio 11 2012 Win64" ..
cd ..

cmake --build Build32 --config Release
cmake --build Build64 --config Release

copy Build32\wimproved.dll wimproved32.dll
copy Build64\wimproved.dll wimproved64.dll

