#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # deep clean kernel build tree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    # Use provided configuration for 'virt' arm dev board
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    # build kernel image for QEMU
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    # Skip building modules to avoid too large kernel to fit in the initramfs with default memory.
    # make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    # build device tree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
# Copy image file to ${OUTDIR}
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Create necessary base directories
ROOTFS="${OUTDIR}/rootfs/"
mkdir -p ${ROOTFS}/bin
mkdir -p ${ROOTFS}/dev
mkdir -p ${ROOTFS}/etc
mkdir -p ${ROOTFS}/home
mkdir -p ${ROOTFS}/lib
mkdir -p ${ROOTFS}/lib64
mkdir -p ${ROOTFS}/proc
mkdir -p ${ROOTFS}/sbin
mkdir -p ${ROOTFS}/sys
mkdir -p ${ROOTFS}/tmp
mkdir -p ${ROOTFS}/var
mkdir -p ${ROOTFS}/usr/bin
mkdir -p ${ROOTFS}/usr/lib
mkdir -p ${ROOTFS}/usr/sbin
mkdir -p ${ROOTFS}/var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"


# Add library dependencies to rootfs
SYS_ROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot )
cp ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${ROOTFS}/lib
cp ${SYSROOT}/lib64/libm.so.6 ${ROOTFS}/lib64
cp ${SYSROOT}/lib64/libresolv.so.2 ${ROOTFS}/lib64
cp ${SYSROOT}/lib64/libc.so.6 ${ROOTFS}/lib64

# Make device nodes
sudo mknod -m 666 ${ROOTFS}/dev/null c 1 3
sudo mknod -m 600 ${ROOTFS}/dev/console c 5 1

# Clean and build the writer utility
make -C ${FINDER_APP_DIR} clean
make -C ${FINDER_APP_DIR} all

# Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp ${FINDER_APP_DIR}/writer ${ROOTFS}/home/
cp ${FINDER_APP_DIR}/finder.sh ${ROOTFS}/home/
cp ${FINDER_APP_DIR}/finder-test.sh ${ROOTFS}/home/
mkdir -p ${ROOTFS}/home/conf
cp ${FINDER_APP_DIR}/conf/username.txt ${ROOTFS}/home/conf/
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${ROOTFS}/home/


# Chown the root directory
sudo chown -R root:root ${ROOTFS}

# TODO: Create initramfs.cpio.gz
find ${ROOTFS} | cpio -H newc -ov --owner root:root > initramfs.cpio
gzip initramfs.cpio
