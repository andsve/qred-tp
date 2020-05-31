if [ "$(uname)" == "Darwin" ]; then
    genie gmake
    # genie ninja
elif [ "$(expr substr $(uname -s) 1 10)" == "MINGW64_NT" ]; then
    genie gmake # vs2015
else
    # genie ninja
    genie gmake
fi
