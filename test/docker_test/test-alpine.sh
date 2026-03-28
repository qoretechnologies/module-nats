#!/bin/bash

set -e
set -x

ENV_FILE=/tmp/env.sh

. ${ENV_FILE}

# setup MODULE_SRC_DIR env var
cwd=`pwd`
if [ -z "${MODULE_SRC_DIR}" ]; then
    if [ -e "$cwd/src/nats-module.cpp" ]; then
        MODULE_SRC_DIR=$cwd
    else
        MODULE_SRC_DIR=$WORKDIR/module-nats
    fi
fi
echo "export MODULE_SRC_DIR=${MODULE_SRC_DIR}" >> ${ENV_FILE}

echo "export QORE_UID=1000" >> ${ENV_FILE}
echo "export QORE_GID=1000" >> ${ENV_FILE}

. ${ENV_FILE}

export MAKE_JOBS=4

# install nats.c client library from source
NATS_C_VERSION=v3.12.0
git clone --depth 1 --branch ${NATS_C_VERSION} https://github.com/nats-io/nats.c.git /tmp/nats-c

# patch nats.c bug: _update_last_error() called unconditionally even when
# err is NULL, causing NumErrors to increment on every successful request
sed -i 's/    _update_last_error(ep, err);/    if (err != NULL)\n        _update_last_error(ep, err);/' \
    /tmp/nats-c/src/micro_endpoint.c

mkdir -p /tmp/nats-c/build
cd /tmp/nats-c/build
cmake .. -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} -DNATS_BUILD_STREAMING=OFF -DBUILD_TESTING=OFF
make -j${MAKE_JOBS}
make install

# install nats-server binary
NATS_SERVER_VERSION=v2.11.1
ARCH=$(uname -m)
if [ "$ARCH" = "x86_64" ]; then
    NATS_ARCH="amd64"
elif [ "$ARCH" = "aarch64" ]; then
    NATS_ARCH="arm64"
fi
curl -sL "https://github.com/nats-io/nats-server/releases/download/${NATS_SERVER_VERSION}/nats-server-${NATS_SERVER_VERSION}-linux-${NATS_ARCH}.tar.gz" \
    | tar xz -C /tmp
cp /tmp/nats-server-${NATS_SERVER_VERSION}-linux-${NATS_ARCH}/nats-server /usr/local/bin/
chmod +x /usr/local/bin/nats-server

# install nats CLI for interoperability tests
which unzip || apk add --no-cache unzip
NATS_CLI_VERSION=0.3.1
curl -sL "https://github.com/nats-io/natscli/releases/download/v${NATS_CLI_VERSION}/nats-${NATS_CLI_VERSION}-linux-${NATS_ARCH}.zip" \
    -o /tmp/nats-cli.zip
unzip -o /tmp/nats-cli.zip -d /tmp
cp /tmp/nats-${NATS_CLI_VERSION}-linux-${NATS_ARCH}/nats /usr/local/bin/
chmod +x /usr/local/bin/nats

# build module and install
echo && echo "-- building module --"
mkdir -p ${MODULE_SRC_DIR}/build
cd ${MODULE_SRC_DIR}/build
cmake .. -DCMAKE_BUILD_TYPE=debug -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}
make -j${MAKE_JOBS}
make install

# add Qore user and group
if ! grep -q "^qore:x:${QORE_GID}" /etc/group; then
    addgroup -g ${QORE_GID} qore
fi
if ! grep -q "^qore:x:${QORE_UID}" /etc/passwd; then
    adduser -u ${QORE_UID} -D -G qore -h /home/qore -s /bin/bash qore
fi

# own everything by the qore user
chown -R qore:qore ${MODULE_SRC_DIR}

# run the tests
export QORE_MODULE_DIR=${MODULE_SRC_DIR}/qlib:${QORE_MODULE_DIR}
cd ${MODULE_SRC_DIR}
for test in test/*.qtest; do
    gosu qore:qore qore --enable-debug $test -vv
done
