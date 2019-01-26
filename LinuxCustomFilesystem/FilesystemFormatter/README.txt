My implementation:
    * the program will not allow duplicates of files, if a file already exists
      the program should terminate with an error message and a -1
    * the program performs simple input parsing to make sure the target filename
      is valid, for my purposes this means no '/' characters because they caused
	 segfaults, a filename with '/' will cause an invalid arguments error
    * the program terminates and deletes any file created if a write could not be
	 done properly with an error message and a -2
    * I would like to believe that what my code is doing is well commented and
      terribly obvious, basically I hard code all of the bytes into the file
      one at a time the whole way through with some exceptions here and there
      for the timestamp.
    * I use no dynamically allocated memory and I don't pass pointers except in
      the case of time related things, so there shouldn't be any memory related
      segmentation faulting.

Running mkvmufs:
    * simply run the make file, and then "./mkvmufs FILENAMEHERE"
    * note that my program does not accept all possible filenames as stated above

Testing:
    * there are no test files associated with my mkvmufs, the only testing done was to
	 check the difference between my output files, and the output file provided by
	 the instructor.
    * a sample file "cool.img" is included, this was from an intance of mkvmufs ran
      at 09:01:19 PM EST on Friday, November 9th 2018.

Functionality:
    * as far as I have checked, mkvmufs compiles and runs with no errors and should be
      100% complete.
    * the snapshot of time is taken from when the timestamp is first created, not when
      the file is first created this causes a couple of seconds margin of error at most.
    * I assume that the century is always 20, so unfortunately my program is only good
      for the next 81 years.

return values:
 0: success
-1: invalid arguments
-2: could not write to file
