I have managed to compile the game, It can be launched, and levels can be opened.
Limitation: In-game controls are not working.

I am running and compiling the game in an ubuntu VM running ARM vitualization with UTM on MacBook.
I was also able to compile the game in a docker image.

To achieve this I had to learn a bit about CMAKE, because, it seems, the linux CMAKE file was not 
completely prepared. 
I had a lot of trouble with the included libraries being in the wrong architecture.
I was able to handle most of them by simply deleting the included libraries, and going with the 
standard OS versions of them. For LIBBASS I had to make it point at the correct hard-coded architecture
version of the library. I also had to copy that libbass.so into the Resources folder. It seems that it is
taken from there during the game. I had to change a few import statements here and there to fix the code 
to work with system libraries.

Next step: I will have to get the controls working. 