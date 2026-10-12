# Build environment for CustomOS (used by ./build.sh --docker).
FROM debian:12

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        bc build-essential bzip2 ca-certificates cpio file g++ gcc git \
        libncurses-dev make patch perl python3 python3-setuptools rsync \
        unzip wget xz-utils zstd locales \
    && rm -rf /var/lib/apt/lists/* \
    && sed -i 's/# en_US.UTF-8/en_US.UTF-8/' /etc/locale.gen && locale-gen

ENV LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8
# build.sh runs as the invoking user's UID; give that user a writable HOME.
ENV HOME=/tmp
WORKDIR /work
