#!/bin/bash

# Check if "--no-confirmation" argument is not passed
if [[ "$1" != "--no-confirmation" ]]; then
    read -p "This will configure the current directory to be a valid OpenWRT package. Please move the repository root to the \"packages/\" in the OpenWRT SDK (y/N) " -n 1 -r
    echo    # (optional) move to a new line
    if [[ ! $REPLY =~ ^[Yy]$ ]]
    then
        echo "Aborting..."
        [[ "$0" = "$BASH_SOURCE" ]] && exit 1 || return 1 # handle exits from shell or function but don't exit interactive shell
    fi
fi

# Cleaning
make clean

# Remove src if it exists
rm -rf src
# Make src directory
mkdir src

# Move all sources and headers to src
mv *.c *.h src/ 2>/dev/null

# Move include/ and lib/ dirs to src
mv include src/
mv lib src/
mv factories src/

# Move Makefile to src
mv Makefile src/

# Rename the openwrt Makefile
mv Makefile.openwrt Makefile