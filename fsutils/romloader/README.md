# ROMLoader

ROMLoader is a utility application that creates and mounts a ROM File System based on the files and folders stored inside the ```apps/fsutils/romloader/rom``` folder. It is designed for use in situations where a read-only file system is required.

The ROMLoader utility works by scanning the files and folders in the ```rom``` folder and creating a file system image based on their contents. The resulting file system image can then be mounted as a read-only file system by running ```romloader```, allowing applications to access its contents without the ability to modify them. Currently, the ROMFS and CROMFS file systems are supported by ROMLoader.

Additionally, other applications can add files and folders to the ROMFS file system by adding the file/folder path to the "ROMLOADER_COPY" variable inside its Makefile. This allows for easy customization and expansion of the ROMFS file system.

**WARNING**: The ```apps/fsutils/romloader/rom``` folder is automatically cleaned when using ```make distclean```. This means that any files added to this folder will be deleted when cleaning the configuration.

## Configuration Options

The ROMLoader utility can be configured using the following options in the Kconfig:

* ```CONFIG_FSUTILS_ROMLOADER```: Enables the ROMLoader utility;
* ```CONFIG_FSUTILS_ROMLOADER_ROMFS```: Uses the ROMFS file system;
* ```CONFIG_FSUTILS_ROMLOADER_CROMFS```: Uses the CROMFS file system;
* ```CONFIG_FSUTILS_ROMLOADER_MOUNTPOINT```: Sets the mount point for the file system.

For the ROMFS file system, the following options are also available:

* ```CONFIG_FSUTILS_ROMLOADER_SECTORSIZE```: Sets the sector size of the ROMFS file system;
* ```CONFIG_FSUTILS_ROMLOADER_DEVMINOR```: Sets the minor device number of the ROMFS block;
* ```CONFIG_FSUTILS_ROMLOADER_DEVPATH```: The path to the ROMFS block driver device.

## Examples

### Using ROMLoader with other applications

In this example, we will add the file ```file_a.txt``` inside a ```folder_a``` from the ```app_a``` application to the ROM.
To add the file, add the following line to the "Makefile" inside the ```app_a``` folder:

```ROMLOADER_COPY += folder_a/file_a.txt```

Note that if this file is generated during compilation, an explicit rule with the file as target must also be added to the "Makefile" to ensure that it is generated before the ROM is created. For example:

```folder_a/file_a.txt: generate_file_a```

### Using ROMLoader with user files

In this example, we will manually add the file ```file_b``` to the ROM image.
Just copy the desired file to the ```apps/fsutils/romloader/rom``` folder. It will be automatically added to the file system.

### Mounting the file system

To mount the file system, run the ```romloader``` application inside NSH:

```
nsh> romloader
Registering romdisk at /dev/ram0
Mounting ROMFS filesystem at target=/mnt/romloader with source=/dev/ram0
nsh> ls mnt/romloader
/mnt/romloader:
 file_a
 file_b
```
