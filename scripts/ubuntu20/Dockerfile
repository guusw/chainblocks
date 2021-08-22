FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /setup
COPY scripts/linux/install_packages .
RUN ./install_packages

ENV RUSTUP_HOME=/usr/local/rustup
ENV CARGO_HOME=/usr/local/cargo
ENV PATH=${PATH}:${RUSTUP_HOME}/bin:${CARGO_HOME}/bin
COPY scripts/linux/install_rust .
RUN ./install_rust

COPY ./ /repo
WORKDIR /repo
