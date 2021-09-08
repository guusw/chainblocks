# SPDX-License-Identifier: BSD-3-Clause
# Copyright © 2019 Fragcolor Pte. Ltd.

FROM archlinux/base

RUN pacman -Syu --noconfirm && pacman -S --noconfirm base-devel git cmake wget clang ninja boost binaryen emscripten

ARG USER_ID=1000

RUN useradd -ms /bin/bash -u ${USER_ID} chainblocks

USER chainblocks

WORKDIR /home/chainblocks

COPY --chown=chainblocks ./ /home/chainblocks/repo

WORKDIR /home/chainblocks/repo

ENV HOME=/home/chainblocks

# needed to init wasm
RUN source /etc/profile.d/binaryen.sh && \
    source /etc/profile.d/emscripten.sh && \
    emcc --version
