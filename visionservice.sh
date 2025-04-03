#!/bin/bash
echo "Object detection service started"
cd /home/Vision/2702-Vision/2025/Object
sudo mount /dev/sda1 /data
if [ -d "/data/2702-Vision" ]; then
        sudo rm -rf /home/Vision/2702-Vision/
        sudo cp /data/2702-Vision/ /home/Vision/
        sudo rm -rf /data/2702-Vision/
        cd /home/Vision/2702-Vision/lib/
        make
        cd /home/Vision/2702-Vision/2025/Object
        make
fi
/home/Vision/2702-Vision/2025/Object/object