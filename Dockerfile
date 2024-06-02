# Use Ubuntu as the base image
FROM ubuntu:22.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    python3 \
    python3-pip \
    python3-setuptools \
    ninja-build \
    wget \
    libssl-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Set the LLVM version
ARG LLVM_VERSION=16

# Clone the LLVM project repository
RUN git clone --depth=1 https://github.com/llvm/llvm-project.git

# Create a build directory
WORKDIR /llvm-project/build

# Configure the build with CMake
RUN cmake -G Ninja \
    -DLLVM_ENABLE_PROJECTS="clang;libcxx;libcxxabi" \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_TARGETS_TO_BUILD="X86" \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    ../llvm

# Build and install LLVM and Clang
RUN cmake --build . --target install

# Set the environment variables for the newly built Clang
ENV CC=/usr/local/bin/clang
ENV CXX=/usr/local/bin/clang++
ENV PATH=/usr/local/bin:$PATH

# Verify the installation
RUN clang --version
WORKDIR ./
RUN mkdir build && cd build && cmake .. && cmake --build .
# Default command to run when starting the container
CMD ["bash"]
