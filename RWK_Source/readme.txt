RWK is located in /Games/RWK
If you're using Visual Studio, open /Games/RWK/Project/Win.
The version of Visual Studio I use replaces relative dirs with hard-mapped directories so you will need to adjust the locations of /Framework files.

If you're just using cmake, use it from /Games/RWK/Project/CMakeWin
All the paths here are relative, so invoke CMake from inside this directory.

The resulting exe will end up in /Resources.  This is the folder you zip up to distribute with.

* A note on the source:

So this source goes all the way back to iPhone one.  Because of the memory limits of iPhone 1, you'll find lots of code in here that got depreciated (blank functions and such).  Don't wonder too much about blank functions, mostly they're situations where it used to be a low-weight/low-memory menu system that got replaced.  Also, I took out all the actual server calls to connect to Makermall, but left them with example code, so you can build your own server-side level sharing without too much difficulty.  Just look in Framework/Rapt/rapt_comm.cpp and search for www, and replace those with actual URLs.

* Beepbox

The Beepbox folder contains a Beepbox composer that matches versions with the version baked into RwK.  It can be hosted anywhere.  In WorldEditor.cpp you should look for this line:

    OpenURL("http://www.yoururl.com/compose.html")

...and switch it to wherever you host the Beepbox.html

* Bundling graphics and sounds

I use a tool called Bundler to bundle up graphics or sounds.  There's a Linux and Win32 version in the /Tools folder.  Suppose you wanted to make a change to the graphics, let's say you wanted to change the robot's color or graphic; here's what you do:

    1. Make the changes you need in /RWK/Source_Images/Play to the robot graphics
    2. Run Bundler for your system
    3. Drag and drop the folder /RWK/Source_Images/Play right on top of bundler (it also works if you drag any image within the folder)
    4. Wait for Bundler to finish processing
    5. Recompile RwK

Bundler will remake the image in RWK/Resources/Images and will also adjust the source file in RWK/Source if you added or removed a graphic.  If you add or remove a graphic/sound you need to recompile, if you just change one you don't need to.  There's a text file inside each folder (_bundle.txt, _soundbundle.txt) that you can use to adjust image size or tweak other things.

Same for sounds, just add or remove a sound file, then drag the folder into bundler.  It will adjust the gSounds bundle with a variable that you can use to play/stop/etc the sound.

* Bass.dll/Libbass.so

    All my games use BASS for sound and music since I purchased an unlimited license...jeez decades ago.  Anyway, I don't think BASS is free distribution.  I've included the WASM version of the sound core in /Framework/OS/WASM to help you get around this.  The WASM version uses OpenAL and OpenMPT to play the sounds and music respectively.



