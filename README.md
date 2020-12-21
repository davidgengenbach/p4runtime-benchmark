# Test `p4runtime` packet in performance

```
git clone --recurse-submodules https://github.com/davidgengenbach/p4runtime-benchmark.git
wget https://github.com/Kitware/CMake/releases/download/v3.19.2/cmake-3.19.2-Linux-x86_64.sh
bash cmake-3.19.2-Linux-x86_64.sh
sudo apt-get install pkg-config

mkdir -p build && cd build
cmake ..
make -j`nproc`
./p4runtime_benchmark
```