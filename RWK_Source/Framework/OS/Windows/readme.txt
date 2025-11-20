This is a testbed port of Rapt to SDL+OpenGL on windows.
I did it to help me port graphics_core to OpenGL4.

Currently, sound doesn't play, I THINK because SDL has taken over the sound card 
(BASS gives an error that just says "-1, mysterious other error")

The REAL downside of this is all the DLLs you have to include (everything in the DDLs directory 
has to go in your distrib folder)



