# Project Islay
My very work in progress hobby kernel and maybe in the future some sort of os.
The main purpose of this project is not to make a fully working os, it's merely
a learning project, so use it own your own risk. 

## Licensing
Licensed under 3-Clause BSD, see LICENSE.TXT for more details.

## Open Source Acknowledgements
During the creation of the project, a few different sources of open source code
has been used. I'm very grateful for their work, and would like to acknowledge
them.

* Sebastian Raase, a colleague and friend of mine, has not only contributed with 
  great design ideas, I have also used his ROMFS FUSE implementation (licensed
  under MIT, see LICENSES/romfs-license.txt) as a foundation of the kernel's 
  romfs implementation.

* Minix 2, by Andrew S. Tanenbaum and Albert S. Woodhull. Apart from the book
  itself (Operating Systems: Design And Implementation), which has been very
  helpfully in describing some os concepts, some Minix code (licensed under
  3-Clause BSD, see LICENSES/minix-license.txt) has served as a foundation to 
  parts of the file system implementation.  

* OSDev Wiki and its community have provided valuable insights in os design both
  through tutorials and forum threads, parts of the kernel contains some code
  snippets from those sourced licensed under Public Domain (CC0).
