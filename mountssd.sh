# ONLY SHOULD BE RUN ON THE PI
#!/bin/bash

if [ $(whoami) = 'root' ]; then
	if [$(uname -a | grep -oh x86_64 | uniq) != 'x86_64']
        sudo mount /dev/sda1 /data
    else
        echo "This script should only be run on the pi with the SSD connected"
    fi
else
    echo "Must be run with sudo privileges"
fi