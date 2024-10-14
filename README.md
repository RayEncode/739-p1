# T0 KV STORAGE
You can either run this project by CSL Machine or Docker

---
# CSL Version 

## 1. Install Conda

First, create a directory for Miniconda and download Miniconda at your home directory:

```bash
mkdir -p ~/miniconda3
wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda3/miniconda.sh
bash ~/miniconda3/miniconda.sh -b -u -p ~/miniconda3
rm ~/miniconda3/miniconda.sh
```

Initialize Conda:

```bash
~/miniconda3/bin/conda init bash
# Uncomment the line below if using zsh
# ~/miniconda3/bin/conda init zsh
```

Update the current shell environment:

```bash
source ~/.bashrc
```

## 2. Install gRPC and Protocol Buffers

Install gRPC using Conda:

```bash
conda install conda-forge::libgrpc
```

If needed, you can install additional dependencies with the following command:

```bash
conda install --solver=classic conda-forge::conda-libmamba-solver conda-forge::libmamba conda-forge::libmambapy conda-forge::libarchive
```

## 3. Install LevelDB

Install LevelDB and Abseil C++ using the following commands:

```bash
conda install conda-forge/label/cf202003::leveldb
conda install -c conda-forge abseil-cpp
```

## 4. Clone the Project

Clone the Durable KV Storage project using Git and checkout CSL-version branch:

```bash
git clone https://github.com/YushanQ/Durable-kv-storage.git
cd Durable-kv-storage
git checkout CSL-version
```

## 5. Compile the Code

Navigate to your project directory and compile the code:

```bash
# In the project directory
mkdir build && cd build
cmake ..   # or use cmake -DCMAKE_BUILD_TYPE=Debug .. to enable gdb debugging
make
```

## 6. Run the Application

After compiling, you can run the application with the following command:

```bash
./your_application_executable
```

Replace `your_application_executable` with the actual name of your compiled executable.

### Example

```bash
./test
```

## Good Luck!

If you have any questions, please refer to the project's [GitHub page](https://github.com/YushanQ/Durable-kv-storage).


---
# Docker Version 

## Pull Code

Clone the Durable KV Storage project using Git:

```bash
git clone https://github.com/YushanQ/Durable-kv-storage.git
``` 

## Set up docker
```sh
docker pull ubuntu:22.04
docker run --name [container name] -v [path/to/local]:[path/to/container] -itd [image name]
// example
docker run --name kv-test -v [path/to/Durable-kv-storage]:/app -itd d04dcc2ab57b
```

## Prerequisites

Before running the application, ensure you have the following prerequisites installed:

```sh
sudo apt-get update && sudo apt-get install build-essential autoconf libtool pkg-config
sudo apt-get install cmake
sudo apt-get install -y protobuf-compiler libprotobuf-dev
sudo apt-get install libleveldb-dev
```

## Installing gRPC

To install gRPC manually, follow these steps:

```sh
# at project directory
git clone -b v1.54.0 https://github.com/grpc/grpc
cd grpc
git submodule update --init
mkdir -p cmake/build
cd cmake/build
cmake ../..
make
```

## Compiling the Code

Navigate to your project directory and compile the code:

```sh
# at project directory
mkdir build && cd build
cmake ..   # or cmake -DCMAKE_BUILD_TYPE=Debug .. to enable gdb debugging
make
```

## Running the Application

After compiling, you can run the application with the following command:

```sh
./your_application_executable
```

Replace `your_application_executable` with the actual name of your compiled executable.

```sh
#example
./server
# expect output: Server listening on 0.0.0.0:5001
./client 0.0.0.0:5001 put 1 1234
```
