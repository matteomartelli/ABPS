This README explains how to to build a custom kernel with the support for TED (Transmission Error Detector) 
and how to test it with the TED application. Different build methods are needed whether you
 want to build the TED kernel for a GNU Linux distribution or Android distribution, since some additional 
steps are required for the latter.

#Kernel build
##Linux
###Get kernel sources
Clone the linux kernel repository in your local machine.

	git clone git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git linuxrepo

Then move to the version branch you want to build. 
TED patches are currently available for versions 4.1.0 and 3.4.0 but you can always move to a different version branch
and manually adjust the TED patch. Let's assume you chose the 4.1.0 version.

	cd linuxrepo
	git checkout v4.1

In order to be sure your git head is at the desired version, just check the first 3 lines
of the Makefile located in the repository root directory.

###Patch the kernel
If you chose the version 3.4.0 of the kernel you should first apply an official 
 patch needed by TED to work properly. This patch was introduced in later versions and allows
 TED to read some information about its socket at lower levels of the network stack.

From the root directory of the linux kernel repository:

	patch -p1 < path_to_abps_repo/kernel_patches/net-remove-skb_orphan_try.3.4.5.patch

Then apply the TED patch. For the 4.1.0 version only this step is needed to patch the kernel sources.
	
	patch -p1 < path_to_abps_repo/kernel_patches/ted_linux_(your_chosen_version).patch

###Build kernel
Several well documented guides explaining how to build a custom kernel exists in the web. 
Let's pick one as instance:
https://wiki.archlinux.org/index.php/Kernels/Traditional_compilation

As the guide itself explains, for a faster compilation `make localmodconf` 
is recommended but be sure that all the modules you will need later 
are currently loaded.  
At the end, to start the build just run:

	make -jN

(where N is the number of parallel compilation processes you want to spawn).

After your custom kernel is built just run with root privileges:

	make modules_install
	make install

or refer to your linux distribution kernel install method.

##Android
The following steps are known to be working for the Google Nexus 5.
- TODO: how to enable su

###Get tools and kernel sources
First give a look at the official android documentation, in order to understand
what kernel version you need depending on your device and get the right tools:
https://source.android.com/source/building-kernels.html .
Here I post the steps needed to get the right tools and kernel version for 
a Google Nexus 5 device.

Get the prebuilt toolchain (compiler, liker, etc..), move it wherever you like and export its path:

	git clone https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6
	sudo mv arm-eabi-4.6 /opt/
	export PATH=/opt/arm-eabi-4.6/bin:$PATH
You can also copy the last line inside your ``.bashrc``.

Get the kernel sources:

	git clone https://android.googlesource.com/kernel/msm
	git checkout android-msm-hammerhead-3.4-marshmallow-mr2

###Patch the kernel
From the root directory of the android kernel repository:

	patch -p1 < path_to_abps_repo/kernel_patches/net-remove-skb_orphan_try.3.4.5.patch
	patch -p1 < path_to_abps_repo/kernel_patches/ted_linux_3.4.patch

###Configure and build
Since TED requires a device which supports mac80211, 
the inner WiFi module (Broadcom BCM4329) of the Google Nexus 5 can't be used. 
In fact the BCM4329 module works as  a Full MAC device and a closed source firmware 
without any support for the mac80211 subsystem.

Thus in order to test TED with the Nexus 5 you should add in your custom kernel the
 support for a USB Wi-Fi dongle that is mac80211 capable. This repo provide a 
custom configuration which add the support for the Atheros ``ath9k_htc`` and Ralink ``rt2800``.
You can of course make your own configuration to add the support for different or more devices.

First of all let's prepare the environment. From the root directory of the android kernel repository:

	export ARCH=arm
	export SUBARCH=arm
	export CROSS_COMPILE=arm-eabi-

Then let's make the configuration. If you are fine with my custom configuration:

	cp path_to_abps_repo/android_build/hammerhead_defconfig_ted .config

Otherwise if you want to make your own configuration:

	cp arch/arm/configs/hammerhead_deconfig .config
	make menuconfig

Then start the build:

	make -jN

(where N is the number of parallel compilation processes you want to spawn).


####Common issues

Compiling old kernels from a new linux distribution may require some workaround.

If you encounter an error on ``scripts/gcc-wrapper.py``, it may be caused by the fact that
 your default python binary links to ``python3`` and that script wants ``python 2``.

A simple workaround consists in replacing the first line of ``scripts/gcc-wrapper.py`` with:

	#! /usr/bin/env python2

An other error that you may encounter is 

	Can't use 'defined(@array) (Maybe you should just omit the defined()?) at kernel/timeconst.pl

To avoid this just remove all the `defined()` invocation, without removing the inner array variables, 
from ``kernel/timeconst.pl`` as the comment in the error suggests.

Once the build is finished, the kernel image is located at ``arch/arm/boot/zImage-dtb``.

###Enable the USB Wi-Fi Dongle
As many Android devices, the Nexus 5 starts according to a init script contained in the ramdisk filesystem image, 
which itself is contained in the boot image together with the kernel image.

The init script is the one who starts the ``wpa_supplicant`` daemon with the default interface ``wlan0``.
I had to modify the init script inside the ramdisk image in order to start ``wpa_supplicant`` with the external
usb dongle, which is ``wlan1``. To do so, you'd need to extract the original boot image from your device, extract the ramdisk image,
modify the ``init.hammerhead.rc`` file substituting all the ``wlan0`` with ``wlan1``.

This repository provides an already modified ramdisk image, anyway here I write the steps you'd need to do in order to
 retrieve your original boot image from the Nexus 5:

####Get the original boot.img
```bash
#use the following commands to find the boot partition
ls -l /dev/block/platform/
#now we know the device platform is msm_sdcc.1
ls -l /dev/block/platform/msm_sdcc.1/by-name
#now we know the boot partition is cat mmcblk0p19 by [boot -> /dev/block/mmcblk0p19]
#use the following command to retrieve the boot.img
su
cat /dev/block/mmcblk0p19 > /sdcard/boot-from-android-device.img
chmod 0666 /sdcard/boot-from-android-device.img
```
Then you can copy the image in your machine with ``adb pull`` or the MPT protocol.

####Extract the original boot.img

Once you have your original boot image in your hand, you can proceed with the extraction.

I recommend to read this http://www.slideshare.net/chrissimmonds/android-bootslides20 and use 
this tool for the boot image extraction: https://github.com/csimmonds/boot-extract. 
The latter is mirrored in this repository also under ``path_to_abps_repo/android_build/boot/boot-extract``.

	path_to_abps_repo/android_build/boot/boot-extract/boot-extract boot.img

Store the output of the extraction, since it will be necessary later for the boot image re-creation.
In my case this is the output:

```
Boot header
  flash page size	2048
  kernel size		0x86e968
  kernel load addr	0x8000
  ramdisk size		0x12dab8
  ramdisk load addr	0x2900000
  second size		0x0
  second load addr	0xf00000
  tags addr			0x2700000
  product name		''
  kernel cmdline	'console=ttyHSL0,115200,n8 androidboot.hardware=hammerhead user_debug=31 maxcpus=2 msm_watchdog_v2.enable=1'

zImage extracted
ramdisk offset 8845312 (0x86f800)
ramdisk.cpio.gz extracted
```
####Extract and edit the original ramdisk
Once you have your ramdisk image ``ramdisk.cpio.gz`` you can extract it with:

	mkdir ramdisk_dir
	cd ramdisk_dir
	zcat ../ramdisk.cpio.gz | cpio -i

Finally you can edit the ``init.hammerhead.rc`` file substituting all the ``wlan0`` with ``wlan1``.

####Create the custom ramdisk
After that, re-create the ramdisk filesystem image with the ``mkbootfs`` tool:

	cd ..
	path_to_abps_repo/android_build/boot/mkbootfs/mkbootfs ramdisk_dir > ramdisk.ted.cpio
	gzip ramdisk.ted.cpio

The result is a modified ramdisk image: ``ramdisk.ted.cpio.gz``. If this fails to boot you can try with the
 pre-made custom ramdisk available at ``path_to_abps_repo/android_build/boot/ramdisk.ted.cpio.gz``.

####Create the custom boot.img
Now re-create the boot image from both the custom kernel and the custom ramdisk image:

	path_to_abps_repo/android_build/boot/mkbootimg/mkbootimg --base 0 --pagesize 2048 --kernel_offset 0x00008000 \
	--ramdisk_offset 0x02900000 --second_offset 0x00f00000 --tags_offset 0x02700000 \
	--cmdline 'console=ttyHSL0,115200,n8 androidboot.hardware=hammerhead user_debug=31 maxcpus=2 msm_watchdog_v2.enable=1' \
	--kernel path_to_kernel/zImage-dtb --ramdisk path_to_ramdisk/ramdisk.ted.cpio.gz -o boot.img

Note how the offsets correspond to the addresses printed out by the boot image extract script.

####Enable wlan1 in system files
Editing the init rc file it's not enough, as some other android services still refer to the wlan0 device.

You need to do the substitution of ``wlan0`` with ``wlan1`` also in the files ``/system/buil.prop`` and ``/system/etc/dhcpcd/dhcpcd.conf``.
These files are persistent in the android root filesystem thus you just need to edit them once.

At the end you can boot the custom boot.img with fastboot:
	
	adb reboot bootloader
	sudo fastboot boot boot.img

Or if you are sure of what you are doing you can flash it:

	sudo fastboot flash boot boot.img

Once the Nexus rebooted it can be useful to activate the remote adb access:

	adb tcpip 5555

Then you can plug the external Wi-Fi dongle with a OTG cable and control the device with remote adb:
	
	adb connect nexus_ip_address:5555

####Open issue
The sleep mode of the external Wi-Fi dongle is not handled properly. In fact turning off the LCD screen cause
 the Wi-Fi device to de-associate from the network. As a simple workaround you can use an app to force 
your screen active but consider that this will lead to rapidly exhaust the Nexus 5 battery power.

TESTAPP:
linux:
- put uapi in the right folder and just make the app
android:
- how to download ndk
- how to copy uapi in ndk for building the application
	cp uapi/errqueue.h uapi/socket.h android-ndk-r11c/platforms/android-21/arch-arm/usr/include/linux/
