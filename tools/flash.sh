SAMPLE=${1:-hal_fpu}
rm -rf build
mkdir -p build
cd build
cmake .. -DSTANDALONE=OFF -DSAMPLE=$SAMPLE -DTEST=OFF -G "Unix Makefiles"
cmake --build . --target flash
cd ..
