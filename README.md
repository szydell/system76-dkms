[![COPR](https://copr.fedorainfracloud.org/coprs/szydell/system76/package/system76-dkms/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/szydell/system76/package/system76-dkms/)

# system76-dkms
System76 DKMS driver
---
Repository for automated RPM creations.

Want to use system76-dkms on Fedora? Check this COPR -> https://copr.fedorainfracloud.org/coprs/szydell/system76/

Primary repo: https://github.com/szydell/system76-dkms
Secondary repo: https://pagure.io/system76/system76-dkms

=======
# system76-dkms

On System76 laptops with Clevo proprietary firmware, these drivers control
some of the hotkeys and allow for custom fan control.

## Kernel support

This project targets 5.15, the last LTS of the 5.x series.

## Development

To install this as a kernel module:

```
# Compile the module
make
# Remove any old instances
sudo modprobe -r system76
# Insert the new module
sudo insmod src/system76.ko dyndbg=+p
# View log messages
dmesg | grep system76
```

## Resources

- <https://docs.kernel.org/admin-guide/dynamic-debug-howto.html>
- <https://docs.kernel.org/dev-tools/checkpatch.html>
