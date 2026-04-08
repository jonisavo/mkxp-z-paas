FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Use Kitware's APT repo to get a newer CMake than Ubuntu 22.04 provides.
RUN apt-get update && apt-get install -y \
    ca-certificates \
    gnupg \
    wget \
  && wget -qO- https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg \
  && echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main" > /etc/apt/sources.list.d/kitware.list \
  && apt-get update && apt-get install -y \
    git \
    build-essential \
    cmake \
    meson \
    autoconf \
    automake \
    libtool \
    pkg-config \
    ruby \
    bison \
    zlib1g-dev \
    libbz2-dev \
    xorg-dev \
    libgl1-mesa-dev \
    libgtk-3-dev \
    libasound2-dev \
    libpulse-dev \
    xxd \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

# Install latest AppImageTool for building AppImages inside the container.
RUN wget -qO /usr/local/bin/appimagetool https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage \
  && chmod +x /usr/local/bin/appimagetool

WORKDIR /workspaces/mkxp-z

CMD ["sleep", "infinity"]
