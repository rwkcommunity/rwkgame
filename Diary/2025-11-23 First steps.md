To day was my first try at compiling anything at all.
I am on mac and I see that the supported systems are Linux and WIN, so I set up a docker to compile on mac.
My dockerfile was:
```
FROM ubuntu:latest
COPY setup.sh .
RUN /bin/bash setup.sh
```
Where setup.sh looks like 
```
#!/bin/bash

apt-get update

ln -fs /usr/share/zoneinfo/UTC /etc/localtime
DEBIAN_FRONTEND=noninteractive apt install -y tzdata
dpkg-reconfigure --frontend noninteractive tzdata

apt-get install -y cmake clang  libasound2-dev libxext-dev libglew-dev
```
Then I run on mac
```
$> cd rwkgame/docker
$> docker buildx build -t rwk .
$> docker run -it --rm --mount type=bind,source=$HOME/dev/rwkcommunity/rwkgame/RWK_Source,dst=/RWK_Source rwk /bin/bash
```
Once inside the container, I run:
```
cd RWK_Source/Games/RWK/Project/Linux/
cmake .
make
```
The output that I see ends with:
```
[ 83%] Built target glew_s
[ 83%] Building CXX object CMakeFiles/RWK.dir/RWK_Source/Framework/OS/Linux/graphics_core.cpp.o
/RWK_Source/Framework/OS/Linux/graphics_core.cpp:206:19: fatal error: use of undeclared identifier 'HASH8'
  206 |         if (PartnerQuery(HASH8("FORCEREZ")))
      |                          ^
1 error generated.
make[2]: *** [CMakeFiles/RWK.dir/build.make:412: CMakeFiles/RWK.dir/RWK_Source/Framework/OS/Linux/graphics_core.cpp.o] Error 1
make[1]: *** [CMakeFiles/Makefile2:130: CMakeFiles/RWK.dir/all] Error 2
make: *** [Makefile:136: all] Error 2
```

The missing pre-processor method `HASH8` is defined in `rapt_math.h` but I have no idea how
 that it supposed to be included (it's not a direct path).
I am probably trying to compile way too many things by making `all` (the defaul target). I 
should find a way to only generate one product, either a WASM or a native application, not 
both.

I tried naively adding an include statement, but that caused more problems than it solved.

# Conclusion
For now I am happy with getting this far. For next time, I will try to set up an AI agent 
that can run against the entire repo and see what that can come up with.
