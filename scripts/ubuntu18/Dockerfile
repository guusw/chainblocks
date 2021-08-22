FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /setup
COPY scripts/ .
RUN ubuntu18/install_packages

RUN ubuntu18/install_cmake

ENV RUSTUP_HOME=/usr/local/rustup
ENV CARGO_HOME=/usr/local/cargo
ENV PATH=${PATH}:${RUSTUP_HOME}/bin:${CARGO_HOME}/bin
RUN ubuntu18/install_rust

COPY ./ /repo
WORKDIR /repo
