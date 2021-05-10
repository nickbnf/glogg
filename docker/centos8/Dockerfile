FROM centos:8
RUN dnf install -y epel-release && \
    dnf install -y qt5-qtbase-devel openssl-devel boost-devel ragel gcc-c++ make zlib-devel wget python38 rpm-build && \
    dnf --enablerepo=powertools install -y ninja-build && \
    wget https://github.com/Kitware/CMake/releases/download/v3.20.2/cmake-3.20.2-linux-x86_64.sh && \
    chmod 755 cmake-3.20.2-linux-x86_64.sh && \
    ./cmake-3.20.2-linux-x86_64.sh --prefix=/opt/ --exclude-subdir --skip-license