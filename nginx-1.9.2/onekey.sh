#!/bin/bash

CFLAGS="-pipe -W -Wall -Wpointer-arith -Wno-unused -g3"
prefix="/usr/local/nginx"
# patch_file="patch/nginx-1.9.2.patch"
# checkout_tag="release-1.9.2"
# checkout_tag="master"
user_notation="@@@@"

# git branch | grep "$checkout_tag" 2>&1 > /dev/null
# if [ $? -ne 0 ]; then
#     echo "$user_notation Checking out to tag $checkout_tag"
#     git checkout $checkout_tag
#     if [ $? -ne 0 ]; then
#         echo "$user_notation git checkout failed"
#         exit 1
#     fi
# else
#     echo "$user_notation Already in branch $checkout_tag"

# fi

export CFLAGS
# ./auto/configure --prefix=/opt/nginx --with-threads --with-http_ssl_module
./configure --prefix=$prefix --with-threads
if [ $? -ne 0 ]; then
    echo "$user_notation configure failed"
    exit 1
fi

# Check if need to apply patch
# grep "CRYPT_DATA_INTERNAL_SIZE" src/os/unix/ngx_user.c
# if [ $? -ne 0 ]; then
#     if [ ! -f $patch_file ]; then
#         echo "$user_notation Patch file $patch_file does not exist"
#         exit 1
#     fi
#     git apply $patch_file
#     if [ $? -ne 0 ]; then
#         echo "$user_notation Apply patch failed"
#     	exit 1
#     fi
# else
#     echo "$user_notation Patch already applied"
# fi

# bear make -j12
make -j12
if [ $? -ne 0 ]; then
    echo "$user_notation make failed"
    exit 1
fi

echo "$user_notation Remove the write privilege of the compile_commands.json file"
chmod -w compile_commands.json
echo "$user_notation Congratulations! You have successfully built nginx."
echo "$user_notation Now you can run 'sudo make install' to install nginx to $prefix."
exit 0

if [ -d $prefix ]; then
    if [ "$(ls -A $prefix)" ]; then
	echo "$user_notation Destination directory $prefix exists and not empty, delete it first"
	sudo rm -rf $prefix
    fi
fi

sudo make install
if [ $? -ne 0 ]; then
    echo "$user_notation make install failed"
    exit 1
fi
echo "$user_notation Congratulations! You have successfully installed nginx to $prefix."
