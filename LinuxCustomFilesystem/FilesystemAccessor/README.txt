Functionality:
	* As far as I can tell, my implementation of vmufs.c works as long
	  as it is ran with the -f and -s arguments (foreground and single
	  threading).
	* "ls" works on my mounted file system BUT I did not bother with the
	  time stamps yet.
	* "cat", "hexdump", and "xxd" work on all of the starter files given
	  by Sebald such as populated1.img, populated2.img... etc. again, I
	  did not include the timestamp in the file statistics.

Input Validation:
	* I trust the user of the fuse client to enter valid information in my
	  main function.
	* I don't parse the fuse arguments before passing them to the fuse client
	* I don't parse the image file before passing it to the fuse client
	* I validate the size of the image file in readDir in seperate locations
	* I validate that the file contains a directory
	* I validate that the file contains a FAT
	* I validate the FAT of the file, making sure that files are actually the
	  size that the directory says they are and that they do not have any
	  values that they shouldn't where they shouldn't. (a file can't have
	  portions of the directory allocated to it, or the root, or the FAT).
	* I do NOT validate the root block whatsoever, in other words if the last
	  512 bytes of the file were all garbage my program would never know.

Errors:
	* running without the -f and -s flags does not work whatsoever, I don't
	  know why and at the time I submitted this it was too late to find out
	  if I wanted it to be on time.
	* all of the errors I'm returning to the fuse client are fairly straight
	  forward:
	* -EINVAL means there's something wrong with the image file (bad argument
	  passed to the client initially)
	* -ENOMEM means there's not enough memory for some variable I'm trying to
	  allocate
	* -EIO means that for some reason I can't open and read from the file

Usage:
	* run the make file
	* ./vmufs <image_file> <fuse_args> in one terminal
	* open up another terminal and perform reading operations on the mounted
	  location
	* as far as I can tell the only way to stop the client is to gracefully
	  terminate it with keyboard interruption (ctrl+c), killing the process
	  causes the mount location to be reserved by the OS and will be unusable
	  until you forcefully unmount it as root with umount -f <mount_location>
	* again, the only way my implementation works is with the -s and -f flags
	  in the fuse_args.
