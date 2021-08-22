FROM alpine:edge AS emsdk
WORKDIR /setup
COPY scripts/emscripten/install_emsdk .
RUN apk add git python3 && \
  ./install_emsdk

FROM alpine:edge AS boost
WORKDIR /setup
COPY scripts/emscripten/install_boost .
RUN ls -lah && pwd && ./install_boost

FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /setup
COPY scripts/emscripten/install_packages .
RUN ./install_packages

ENV RUSTUP_HOME=/usr/local/rustup
ENV CARGO_HOME=/usr/local/cargo
ENV PATH=/usr/local/cargo/bin:$PATH
COPY scripts/emscripten/install_rust ./
RUN ./install_rust

ENV EMSDK=/emsdk
COPY --from=emsdk /setup/emsdk $EMSDK
COPY --from=boost /setup/external /repo/external
COPY ./ /repo
WORKDIR /repo
