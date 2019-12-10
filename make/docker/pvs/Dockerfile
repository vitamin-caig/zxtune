FROM debian:stretch-slim
RUN apt-get update && \
    apt-get install -y --no-install-recommends apt-transport-https ca-certificates debhelper build-essential git zip python3-minimal \
      wget gnupg2 strace \
      libpulse-dev libclalsadrv-dev \
      libboost-filesystem-dev libboost-locale-dev libboost-program-options-dev libboost-system-dev libicu-dev libicu57 \
      libqt4-dev libxfixes-dev libxrandr-dev libfontconfig-dev && \
    find / -name 'libboost*.so' -exec rm {} \; && \
    find / -name 'libicu*.so*' -exec rm {} \; && \
    rm -rf /var/lib/apt/lists/*

RUN wget -q -O - https://files.viva64.com/etc/pubkey.txt | apt-key add - && \
    wget -q -O /etc/apt/sources.list.d/viva64.list https://files.viva64.com/etc/viva64.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends pvs-studio && \
    pvs-studio --version && \
    rm -rf /var/lib/apt/lists/*

ARG pvs_name
ARG pvs_code
RUN pvs-studio-analyzer credentials ${pvs_name} ${pvs_code}

WORKDIR /build/zxtune
RUN git init && \
    git remote add --tags origin https://bitbucket.org/zxtune/zxtune.git && \
    echo "platform=linux\narch=x86_64\npackaging?=deb\ndistro?=stretch\nqt.includes=/usr/include/qt4\nlibraries.linux+=icuuc icudata\ntools.python=python3" > variables.mak
COPY entrypoint.sh .
ENTRYPOINT ["./entrypoint.sh"]
CMD ["-C", "apps/bundle"]
