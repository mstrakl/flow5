# flow5 Linux build / dev / packaging image
#
#   Base: AlmaLinux 9 (RHEL 9, glibc 2.34) with gcc-toolset-14 -> the AppImage runs on
#         Ubuntu 22.04+, Debian 12+, RHEL/Alma/Rocky 9+, Fedora 35+, ...
#         gcc-toolset links whatever the system's older libstdc++ lacks statically,
#         so the binaries only need the distro's own libstdc++ (GCC 11 or newer).
#
#   OpenCascade and gmsh are built from source: the distro packages are either too
#   old (flow5 needs OCCT >= 7.8) or linked against another OCCT than the one flow5
#   uses, which would put two OCCT copies in the same process.
#
#   Build:  ./docker/dev.sh image

FROM almalinux:9

# Versions are declared next to the step that uses them, so bumping one only
# rebuilds from that step on.

ENV LANG=C.UTF-8 \
    LC_ALL=C.UTF-8

# ---------------------------------------------------------------- system packages
RUN dnf install -y epel-release dnf-plugins-core \
    && dnf config-manager --set-enabled crb \
    && dnf install -y --setopt=install_weak_deps=False \
        # toolchain
        gcc-toolset-14-gcc gcc-toolset-14-gcc-c++ gcc-toolset-14-binutils \
        make cmake ninja-build pkgconf-pkg-config ccache mold \
        git file which xz findutils python3.12 \
        # numerics (EPEL, Fedora layout: <openblas/cblas.h>, -lopenblasp)
        openblas-devel \
        # OpenCascade / gmsh build deps
        freetype-devel fontconfig-devel mesa-libGL-devel mesa-libEGL-devel mesa-libGLU-devel \
        libX11-devel libXi-devel libXext-devel libXmu-devel \
        # Qt runtime (xcb platform plugin + OpenGL), also needed by linuxdeploy-plugin-qt
        libxkbcommon libxkbcommon-x11 libxkbcommon-devel xcb-util-cursor xcb-util-image \
        xcb-util-keysyms xcb-util-renderutil xcb-util-wm libxcb libXrender \
        libwayland-client libwayland-cursor libwayland-egl dbus-libs glib2 \
        mesa-dri-drivers mesa-libEGL mesa-libGL libglvnd-glx glx-utils \
        dejavu-sans-fonts \
    && dnf clean all \
    && echo -e "/usr/local/lib\n/usr/local/lib64" > /etc/ld.so.conf.d/usr-local.conf

ENV PATH=/opt/rh/gcc-toolset-14/root/usr/bin:${PATH}

# ---------------------------------------------------------------- Qt (official binaries)
ARG QT_VERSION=6.11.2
RUN python3.12 -m venv /opt/aqt \
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
# ccache's compiler links come first, they call the gcc-toolset compilers.
ENV PATH=/usr/lib64/ccache:${PATH} \
    CCACHE_DIR=/ccache \
    CCACHE_MAXSIZE=5G \
    APPIMAGE_EXTRACT_AND_RUN=1
RUN mkdir -p /ccache && chmod 1777 /ccache \
    && for c in gcc g++ cc c++; do ln -sf /usr/bin/ccache /usr/lib64/ccache/$c; done

WORKDIR /src
