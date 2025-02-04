#!/bin/bash
echo 'file <ov9281.c> +p'>/sys/kernel/debug/dynamic_debug/control
echo 'file <tegracam_ctrls.c> +p'>/sys/kernel/debug/dynamic_debug/control
echo 'file <tegracam_v4l2.c> +p'>/sys/kernel/debug/dynamic_debug/control
#echo "8    4    1    7" > /proc/sys/kernel/printk
echo "9    8    8    8" > /proc/sys/kernel/printk
