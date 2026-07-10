#!/bin/bash

read -p "This will discard all local changes (git reset --hard), pull the latest changes from the remote and reconfigure the OpenWRT package (y/N) " -n 1 -r
echo    # (optional) move to a new line
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    echo "Aborting..."
    [[ "$0" = "$BASH_SOURCE" ]] && exit 1 || return 1 # handle exits from shell or function but don't exit interactive shell
fi

git reset --hard
git pull
./configure-openwrt.sh