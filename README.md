# A Review of this Assignment
No outside help was used, except for references to lecture notes and to the man-pages of pthread and time.

This assignment was a good exercise, however, I think that the critical sections were extremely short. 
The memory allocation solution is actually kind of already implemented with the Local count which is not 
shared between threads. Since the number of points computed is large compared to the critical section, it's hard to know
that critical sections *did* overlap. I personally had more fun figuring out how to write the C code in my
"test_pthreads.c" where I passed around pointers to functions and set up arguments to be passed as void pointers and
reconstructed later. I may well have broken some C-language rules in doing so.

