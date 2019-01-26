(README files in deeper directories were primarily for the usage of the instructor and grader and may
contain irrelevant information)

subdirectory "FilesystemAccessor"
----------------------------------
vmufs.c is compiled and ran in order to allow the Linux operating system to interface with a custom
filesystem image. Custom filesystem images were provided by the instructor for this course, and also
blank filesystem images can be generated with mkvmufs.c in "FilesystemFormatter"

subdirectory "FilesystemFormatter"
----------------------------------
mkvmufs.c is compiled and ran in order to create a blank filesystem image of a format provided by the
instructor (which is similar to that of a SEGA dreamcast memory cartridge)