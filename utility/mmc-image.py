#!/usr/bin/python3

import sys
import os
import pexpect

class switch(object):
	def __init__ (self, value):
		self.value=value
		self.fall=False

	def __iter__ (self):
		yield self.match
		raise StopIteration

	def match (self, *args):
		if self.fall or not args:
			return True
		elif self.value in args:
			self.fall=True
			return True
		else:
			return False

flash_page_size	= 512
flash_image_pages = 524288

# create a image
def create(img):
	mk_img = "dd if=/dev/zero of=" + img + " bs=" + str(flash_page_size) + " count=" + str(flash_image_pages)

	os.system(mk_img)

# cp file to image
def cp_file(th_image, bh_image, zImage, rootfs, sd_image_name):
	cp_th_file= "sudo cp " + th_image + " /mnt/MLO"
	cp_bh_file= "sudo cp " + bh_image + " /mnt/"
	cp_zImage = "sudo cp " + zImage + " /mnt/"
	cp_rootfs = "sudo cp " + rootfs + " /mnt/"

	print "add \"" + th_image + "\" to", sd_image_name
	os.system(cp_th_file)

	print "add \"" + bh_image + "\" to", sd_image_name
	os.system(cp_bh_file)

	if zImage != "":
		print "add \"" + zImage + "\" to", sd_image_name
		os.system(cp_zImage)

	if rootfs != "":
		print "add \"" + rootfs + "\" to", sd_image_name
		os.system(cp_rootfs)

# making ext2 File system
def mk_ex2(sd_image_name):
	mk_fs_ext2= "sudo mkfs.ext2 " + sd_image_name

	foo = pexpect.spawn(mk_fs_ext2)
	foo.expect("")
	foo.sendline("y")
	foo.interact()

# making ext3 File system
def mk_ex3(sd_image_name):
	mk_fs_ext3= "sudo mkfs.ext3 " + sd_image_name

	foo = pexpect.spawn(mk_fs_ext3)
	foo.expect("")
	foo.sendline("y")
	foo.interact()

# making vfat File system
def mk_vfat(sd_image_name):
	mk_fs_vfat= "sudo mkfs.vfat " + sd_image_name

	os.system(mk_fs_vfat)

def mk_ubi(sd_image_name): #fixme
	print "ubifs"

def mk_img(th_image, bh_image, zImage, rootfs, fs, sd_image_name):
	mount_fs  = "sudo mount -o loop " + sd_image_name + " /mnt/"
	umount_fs = "sudo umount /mnt/"

	if fs == "ext2":
		mk_ex2(sd_image_name)
	elif fs == "ext3":
		mk_ex3(sd_image_name)
	elif fs == "vfat":
		mk_vfat(sd_image_name)
	elif fs == "ubifs":
		mk_ubi(sd_image_name)
	else:
		sys.exit()

	os.system(mount_fs)

	cp_file(th_image, bh_image, zImage, rootfs, sd_image_name)

	os.system(umount_fs)
	print "making " + sd_image_name + " done"

if __name__ == "__main__":
	if len(sys.argv) != 3:
		print 'Usage: ' + sys.argv[0] + ' <fstype[ext2, ext3, vfat, ubifs]> <destimage>'
		sys.exit()

	fs_type = sys.argv[1]
	sd_image_name = sys.argv[2]

	if os.path.exists("witrom.bin"):
		th_image = "witrom.bin"
	else:
		print "witrom.bin does not exists"
		sys.exit()

	if os.path.exists("witOS.bin"):
		bh_image = "witOS.bin"
	else:
		print "witOS.bin does not exists"
		sys.exit()

	if os.path.exists(sd_image_name):
		print sd_image_name, "exists"
	else:
		print "create image ..."
		create(sd_image_name)
		print "create image done"

	if os.path.exists("zImage"):
		print "add zImage to image"
		zImage = "zImage"
	else:
		print "zImage does not exists"
		zImage = ""

	if os.path.exists("rootfs"):
		print "add rootfs to image"
		rootfs = "rootfs"
	else:
		print "rootfs does not exists"
		rootfs = ""

	# choose File system
	for case in switch(fs_type):
		if case("ext2"):
			fs = "ext2"
			mk_img(th_image, bh_image, zImage, rootfs, fs, sd_image_name)
			break

		if case("ext3"):
			fs = "ext3"
			mk_img(th_image, bh_image, zImage, rootfs, fs, sd_image_name)
			break

		if case("vfat"):
			fs = "vfat"
			mk_img(th_image, bh_image, zImage, rootfs, fs, sd_image_name)
			break

		if case("ubifs"): # fixme
			fs = "ubifs"
			mk_img(th_image, bh_image, zImage, rootfs, fs, sd_image_name)
			break
		else:
			print "fstype error"
			print 'Usage: ' + sys.argv[0] + ' <fstype[ext2, ext3, vfat, ubifs]> <destimage>'
			sys.exit()
