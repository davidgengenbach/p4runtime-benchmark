# Benchmark [p4runtime](https://github.com/p4lang/p4runtime.git) packet-in performance

```
git clone --recurse-submodules https://github.com/davidgengenbach/p4runtime-benchmark.git
cd p4runtime-benchmark

./misc/convert_proto.sh

mkdir -p build && cd build
cmake ..
make -j`nproc`
./p4runtime_benchmark
```

## Dependencies

```
wget https://github.com/Kitware/CMake/releases/download/v3.19.2/cmake-3.19.2-Linux-x86_64.sh
bash cmake-3.19.2-Linux-x86_64.sh
sudo apt-get install pkg-config
```

## MoonGen

Follow https://github.com/emmericp/MoonGen#installation

```
git clone --depth 1 --recurse-submodules https://github.com/emmericp/MoonGen.git
cd MoonGen
./build.sh
```