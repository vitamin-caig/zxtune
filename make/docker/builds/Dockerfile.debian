FROM debian:stretch-slim
RUN apt-get update && \
    apt-get install -y --no-install-recommends ca-certificates debhelper build-essential git zip python3-minimal \
      libpulse-dev libclalsadrv-dev \
      libboost-filesystem-dev libboost-locale-dev libboost-program-options-dev libboost-system-dev libicu-dev libicu57 \
    libqt4-dev libxfixes-dev libxrandr-dev libfontconfig-dev && \
    find / -name 'libboost*.so' -exec rm {} \; && \
    find / -name 'libicu*.so*' -exec rm {} \; && \
    rm -rf /var/lib/apt/lists/*
WORKDIR /build/zxtune
RUN git init && \
    git remote add --tags origin https://bitbucket.org/zxtune/zxtune.git && \
    echo "platform=linux\narch=x86_64\npackaging?=deb\ndistro?=stretch\nqt.includes=/usr/include/qt4\nlibraries.linux+=icuuc icudata\ntools.python=python3" > variables.mak
COPY entrypoint.sh .
ENTRYPOINT ["./entrypoint.sh"]
CMD ["package", "-C", "apps/bundle"]
