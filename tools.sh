# Download genie binaries if missing

if [ "$QRED_PLATFORM" == "linux" ]; then
    if [ ! -f tools/linux/genie ]; then
        mkdir -p tools/linux/
        wget https://github.com/bkaradzic/bx/raw/master/tools/bin/linux/genie -O tools/linux/genie
        chmod +x tools/linux/genie
    fi
fi

if [ "$QRED_PLATFORM" = "macos" ]; then
    if [ ! -f tools/macos/genie ]; then
        mkdir -p tools/macos/
        wget https://github.com/bkaradzic/bx/raw/master/tools/bin/darwin/genie -O tools/macos/genie
        chmod +x tools/macos/genie
    fi
fi

if [ "$QRED_PLATFORM" = "windows" ]; then
    if [ ! -f tools/windows/genie.exe ]; then
        mkdir -p tools/windows/
        curl -L https://github.com/bkaradzic/bx/raw/master/tools/bin/windows/genie.exe --output tools/windows/genie.exe
        chmod +x tools/windows/genie.exe
    fi
fi
