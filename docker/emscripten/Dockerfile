FROM alpine:edge AS emsdk
RUN apk add git python3 && \
  git clone https://github.com/emscripten-core/emsdk.git && \
  cd emsdk && \
  ./emsdk install latest && \
  ./emsdk activate latest
  
FROM alpine:edge AS boost
RUN mkdir external; cd external; \
  wget -nv https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.gz && tar xzf boost_1_76_0.tar.gz; \
  rm boost_1_76_0.tar.gz

FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get -y update
RUN apt-get -y install tzdata
RUN apt-get -y install build-essential cmake wget git clang ninja-build libboost-all-dev valgrind xorg-dev libdbus-1-dev libssl-dev lcov ca-certificates

ENV RUSTUP_HOME=/usr/local/rustup
ENV CARGO_HOME=/usr/local/cargo
ENV PATH=/usr/local/cargo/bin:$PATH

RUN set -eux; \
    url="https://static.rust-lang.org/rustup/dist/x86_64-unknown-linux-gnu/rustup-init"; \
    wget "$url"; \
    chmod +x rustup-init; \
    ./rustup-init -y --no-modify-path --default-toolchain stable; \
    rm rustup-init; \
    chmod -R a+w $RUSTUP_HOME $CARGO_HOME; \
    rustup --version; \
    cargo --version; \
    rustc --version;

RUN rustup toolchain install nightly && rustup target add wasm32-unknown-emscripten
RUN apt-get -y install vim

ARG USER_ID=1000
RUN useradd -ms /bin/bash -u ${USER_ID} chainblocks;
USER chainblocks

ENV HOME=/home/chainblocks
ENV EMSDK=/home/chainblocks/emsdk

COPY --chown=chainblocks --from=emsdk ./emsdk $EMSDK

RUN cd /home/chainblocks; mkdir repo
WORKDIR /home/chainblocks/repo
COPY --from=boost --chown=chainblocks ./external ./external
COPY --chown=chainblocks ./ ./