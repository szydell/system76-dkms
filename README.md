[![COPR](https://copr.fedorainfracloud.org/coprs/szydell/system76/package/system76-dkms/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/szydell/system76/package/system76-dkms/)

# system76-dkms
System76 DKMS driver
---
Repository for automated RPM creations.

Want to use system76-dkms on Fedora? Check this COPR -> https://copr.fedorainfracloud.org/coprs/szydell/system76/
=======
# system76-dkms
System76 DKMS driver

On newer System76 laptops, this driver controls some of the hotkeys and allows for custom fan control.

## Development

To install this as a kernel module:

```
# Compile the module
make
# Remove any old instances
sudo modprobe -r system76
# Insert the new module
sudo insmod system76.ko
# View log messages
dmesg | grep system76
```
