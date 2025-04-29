ARG UBUNTU_VERSION=20.04
FROM ubuntu:${UBUNTU_VERSION}

ENV DEBIAN_FRONTEND=noninteractive 

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    g++ \
    git \
    gdb \
    valgrind \
    strace \
    nasm \
    man-db \
    doxygen \
    vim \
    wget \
    curl \
    zip unzip \
    && apt-get clean

WORKDIR /work
COPY . /work

CMD ["bash"]
