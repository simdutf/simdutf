FROM ubuntu:24.04
ARG CLANGVERSION=18
COPY llvm.gpg /etc/apt/trusted.gpg.d/llvm.gpg
RUN apt-get update && \
    apt-get dist-upgrade -y && \
    apt-get install -y --no-install-recommends ca-certificates && \
    echo "deb [signed-by=/etc/apt/trusted.gpg.d/llvm.gpg] https://apt.llvm.org/noble/ llvm-toolchain-noble-"${CLANGVERSION}" main" \
    > /etc/apt/sources.list.d/llvm${CLANGVERSION}.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends clang-format-$CLANGVERSION && \
    apt-get install -y --no-install-recommends git && \
    apt-get -q clean -y && \
    rm -rf /var/lib/apt/lists/* && \
    rm -f /var/cache/apt/*.bin
