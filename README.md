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

protobuf-compiler-grpc 

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

## Notes
```
table_entry['SwitchIngress.punt_table'].read(print)
table_entry['SwitchIngress.punt_table'].read(lambda x: x.delete())
    # set_egress_port_static
    # clone_to_cpu
te = table_entry['SwitchIngress.punt_table'](action='set_egress_port_static') 
te.priority = 1
te.match['ig_intr_md.ingress_port'] = 0
te.insert()

all_te = table_entry['SwitchIngress.punt_table']; all_te.counter_data; all_te.read(print)
all_te = table_entry['SwitchIngress.punt_table']
all_te.counter_data
for te in all_te.read():
    te = next(te.read())
    if te.counter_data.byte_count == 0: continue
    print(te)


- CPU load, detection time as function of # seeds
    - Hardware
    - How many Seeds can we create realistically
- How long does FARM need to mitigate in practice
- How do we find a distributed use-case which is comparable?

for iteration in range(170):
    te = table_entry['SwitchIngress.punt_table'](action='set_egress_port_static') 
    te.priority = iteration + 1
    te.match['ig_intr_md.ingress_port'] = str(iteration)
    te.insert()
```