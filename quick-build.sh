# Setup env vars
export QRED_HOME="`pwd`"
if [ "$(uname)" == "Darwin" ]; then
    export QRED_PLATFORM="macos"
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then
    export QRED_PLATFORM="windows"
else
    export QRED_PLATFORM="linux"
fi
export PATH=$PATH:$QRED_HOME/tools/$QRED_PLATFORM/

./tools.sh

if command -v ccache >/dev/null; then
    export CC="ccache gcc"
    export CXX="ccache g++"
fi

./generate.sh
cd build
make

ls -al ../bin/qred_tp
