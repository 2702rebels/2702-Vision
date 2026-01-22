### How to run VM:
1. `cd ~`
2. `git clone https://git.qemu.org/git/qemu.git`
3. `cd qemu`
4. `./configure
  --target-list="aarch64-softmmu,arm-softmmu,x86_64-softmmu,i386-softmmu,riscv64-softmmu,riscv32-softmmu,ppc-softmmu,ppc64-softmmu,mips-softmmu,mipsel-softmmu,mips64-softmmu,mips64el-softmmu,s390x-softmmu,sparc-softmmu,sparc64-softmmu,xtensa-softmmu,xtensaeb-softmmu,alpha-softmmu,sh4-softmmu,sh4eb-softmmu,or1k-softmmu,avr-softmmu,rx-softmmu,loongarch64-softmmu"
  --enable-virtfs --enable-slirp`
5. make -j$(nproc)
6. cd ..
7. mkdir qemu-host
8. `sudo apt install cloud-image-utils`
9. Use the user-data and meta-data files included in this folder or create your own and put them in the qemu-host folder
10. `cloud-localds seed.iso user-data meta-data`
11. `curl -O https://cloud-images.ubuntu.com/noble/current/noble-server-cloudimg-arm64.img`
12. `~/qemu/build/qemu-img resize noble-server-cloudimg-arm64.img +20G` (or however much storage you need)
13. `sudo apt-get install qemu-efi-aarch64`
14. `truncate -s 64m varstore.img`
15. `truncate -s 64m efi.img`
16. `dd if=/usr/share/qemu-efi-aarch64/QEMU_EFI.fd of=efi.img conv=notrunc`
17. `~/qemu/build/qemu-system-aarch64   -M virt,virtualization=true,gic-version=3   -cpu max,pauth-impdef=on   -smp 8 -m 12228   -drive if=pflash,format=raw,file=efi.img,readonly=on   -drive if=pflash,format=raw,file=varstore.img   -drive if=none,id=hd0,file=noble-server-cloudimg-arm64.img,format=qcow2   -device virtio-blk-device,drive=hd0   -drive if=none,id=seed,file=seed.iso,format=raw   -device virtio-blk-device,drive=seed   -object rng-random,filename=/dev/urandom,id=rng0   -device virtio-rng-pci,rng=rng0   -device virtio-net-pci,netdev=net0   -netdev user,id=net0,hostfwd=tcp::8022-:22   -nographic` 
18. You may either end up in the OS or in the UEFI shell. If you end up in the UEFI shell, follow steps 19&20. If you end up in the OS, skip to step 21
19. `FS0:`
20. `EFI\BOOT\BOOTAA64.EFI`
21. Login with username `ubuntu` and password `ubuntu`
Why this is so complicated, I have no idea.
