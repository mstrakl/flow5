# flow5 Linux build / dev / packaging image
#
#   Base: Ubuntu 24.04 (glibc 2.39) -> the AppImage runs on distros from ~2024 on
#         (Ubuntu 24.04+, Debian 13+, Fedora 40+, openSUSE Tumbleweed, ...).
#
#   OpenCascade and gmsh are built from source: the distro packages are either too
#   old (flow5 needs OCCT >= 7.8) or linked against another OCCT than the one flow5
#   uses, which would put two OCCT copies in the same process.
#
#   Build:  ./docker/dev.sh image

FROM ubuntu:24.04

# Versions are declared next to the step that uses them, so bumping one only
# rebuilds from that step on.

ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8

# ---------------------------------------------------------------- system packages
RUN apt-get update && apt-get install -y --no-install-recommends \
        # toolchain
        build-essential g++-14 gcc-14 gfortran cmake ninja-build pkg-config \
        ccache mold git curl ca-certificates file xz-utils python3-venv \
        # numerics
        libopenblas-dev liblapack-dev liblapacke-dev \
        # OpenCascade / gmsh build deps
        libfreetype-dev libfontconfig-dev libgl-dev libegl-dev libglu1-mesa-dev \
        libx11-dev libxi-dev libxext-dev libxmu-dev \
        # Qt runtime (xcb platform plugin + OpenGL), also needed by linuxdeploy-plugin-qt
        libxkbcommon-x11-0 libxkbcommon-dev libxcb-cursor0 libxcb-icccm4 libxcb-image0 \
        libxcb-keysyms1 libxcb-render-util0 libxcb-shape0 libxcb-xinerama0 libxcb-xkb1 \
        libxcb-randr0 libxcb-sync1 libxcb-xfixes0 libxcb-shm0 libxcb-util1 libx11-xcb1 \
        libwayland-client0 libwayland-cursor0 libwayland-egl1 libdbus-1-3 \
        libglib2.0-0t64 libgl1 libegl1 libglx-mesa0 libegl-mesa0 libgl1-mesa-dri mesa-utils \
        fonts-dejavu-core \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100 \
    && update-alternatives --install /usr/bin/cc  cc  /usr/bin/gcc-14 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-14 100 \
    && rm -rf /var/lib/apt/lists/*

# flow5's .pro files use Fedora's layout: #include <openblas/cblas.h> and -lopenblasp.
# Map Ubuntu's pthread OpenBLAS onto those names instead of patching the sources.
# (Ubuntu's OpenBLAS has no lapack.h; the reference LAPACK one from liblapacke-dev is identical.)
RUN mkdir -p /usr/local/include/openblas /usr/local/lib \
    && ln -s /usr/include/x86_64-linux-gnu/openblas-pthread/*.h /usr/local/include/openblas/ \
    && ln -s /usr/include/lapack.h /usr/local/include/openblas/lapack.h \
    && ln -s /usr/lib/x86_64-linux-gnu/libopenblas.so /usr/local/lib/libopenblasp.so

# ---------------------------------------------------------------- Qt (official binaries)
ARG QT_VERSION=6.11.2
RUN python3 -m venv /opt/aqt \
    && /opt/aqt/bin/pip install --no-cache-dir aqtinstall \
    && /opt/aqt/bin/aqt install-qt linux desktop ${QT_VERSION} linux_gcc_64 -O /opt/Qt \
    && rm -rf /opt/aqt /root/.cache

ENV QTDIR=/opt/Qt/${QT_VERSION}/gcc_64
ENV PATH=${QTDIR}/bin:${PATH}

# ---------------------------------------------------------------- OpenCascade
ARG OCCT_TAG=V7_9_3
RUN mkdir -p /tmp/occt && cd /tmp/occt \
    && curl -fsSL https://github.com/Open-Cascade-SAS/OCCT/archive/refs/tags/${OCCT_TAG}.tar.gz \
        | tar xz --strip-components=1 \
    && cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DINSTALL_DIR=/usr/local \
        -DBUILD_LIBRARY_TYPE=Shared \
        -DBUILD_MODULE_Draw=OFF \
        -DBUILD_DOC_Overview=OFF \
        -DBUILD_SAMPLES_QT=OFF \
        -DUSE_TK=OFF -DUSE_FREEIMAGE=OFF -DUSE_RAPIDJSON=OFF -DUSE_DRACO=OFF \
        -DUSE_TBB=OFF -DUSE_VTK=OFF -DUSE_FFMPEG=OFF -DUSE_OPENVR=OFF \
    && cmake --build build \
    && cmake --install build \
    && cd / && rm -rf /tmp/occt

# ---------------------------------------------------------------- gmsh (SDK, linked to the OCCT above)
ARG GMSH_VERSION=4.14.1
RUN mkdir -p /tmp/gmsh && cd /tmp/gmsh \
    && curl -fsSL https://gmsh.info/src/gmsh-${GMSH_VERSION}-source.tgz \
        | tar xz --strip-components=1 \
    && cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DCMAKE_PREFIX_PATH=/usr/local \
        -DENABLE_BUILD_DYNAMIC=ON \
        -DENABLE_BUILD_SHARED=ON \
        -DENABLE_OCC=ON -DENABLE_OCC_CAF=ON \
        -DENABLE_FLTK=OFF -DENABLE_MPI=OFF -DENABLE_PETSC=OFF -DENABLE_SLEPC=OFF \
        -DENABLE_MED=OFF -DENABLE_CGNS=OFF -DENABLE_OPENMP=ON \
    && cmake --build build \
    && cmake --install build \
    && cd / && rm -rf /tmp/gmsh \
    && ldconfig

# ---------------------------------------------------------------- AppImage tooling
# Extracted up front so no FUSE is needed inside the container.
ARG LINUXDEPLOY_TAG=1-alpha-20251107-1
ARG LINUXDEPLOY_QT_TAG=1-alpha-20250213-1
RUN mkdir -p /opt/linuxdeploy && cd /opt/linuxdeploy \
    && curl -fsSLo ld.AppImage https://github.com/linuxdeploy/linuxdeploy/releases/download/${LINUXDEPLOY_TAG}/linuxdeploy-x86_64.AppImage \
    && curl -fsSLo qt.AppImage https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/${LINUXDEPLOY_QT_TAG}/linuxdeploy-plugin-qt-x86_64.AppImage \
    && chmod +x *.AppImage \
    && ./ld.AppImage --appimage-extract >/dev/null && mv squashfs-root linuxdeploy \
    && ./qt.AppImage --appimage-extract >/dev/null && mv squashfs-root plugin-qt \
    && rm *.AppImage \
    && ln -s /opt/linuxdeploy/linuxdeploy/AppRun /usr/local/bin/linuxdeploy \
    && ln -s /opt/linuxdeploy/plugin-qt/AppRun  /usr/local/bin/linuxdeploy-plugin-qt

# Any uid can use the image (dev.sh runs as the host user); keep caches out of $HOME.
ENV PATH=/usr/lib/ccache:${PATH} \
    CCACHE_DIR=/ccache \
    CCACHE_MAXSIZE=5G \
    APPIMAGE_EXTRACT_AND_RUN=1
RUN mkdir -p /ccache && chmod 1777 /ccache

WORKDIR /src
