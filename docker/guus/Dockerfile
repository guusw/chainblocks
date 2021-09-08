# syntax=docker/dockerfile:experimental
FROM alpine

# C++ build environment
RUN apk add curl cmake g++ ninja

# Rust environment
ENV RUSTUP_HOME=/usr/local/rustup
ENV CARGO_HOME=/usr/local/cargo
ENV PATH=/usr/local/cargo/bin:$PATH
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain=nightly;

COPY . .
