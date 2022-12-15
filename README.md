## SMS for Pure Data (Pd)
_version 0.95 -- August 11, 2009_ |||    _updated by Charles K. Neimog in 15-dec-2022_

### OVERVIEW
smspd is an external library of externals for Miller Puckette's PureData ("pd"). 
It uses the libsms library for Spectral Modeling and Synthesis Techniques to 
accomplish SMS analysis, synthesis, and modifications in real-time. This code,
along with many other SMS related things, was developed in the Music Technology 
Group at UPF.  See the SMS homepage there: http://mtg.upf.edu/sms

The webpage to download both smspd and libsms is: http://mtg.upf.edu/static/libsms/

Currently there are the following externals: [smsbuf], [smsanal], [smssynth~],
and [smsedit].  The documentation for these externals is in the form of pd help files.
To see an overview of the set together, see sms-help.pd

### Compiling for Windows
* Install msys2: `winget install msys2.msys2`
* Open the Mingw64 terminal then run: `pacman -S mingw-w64-x86_64-libsndfile mingw-w64-x86_64-gsl mingw-w64-x86_64-cmake`. 
* All inside Mingw64 terminal, 
    * go to libsms folder;
    * `mkdir build && cd build`
    * `cmake -G"MSYS Makefiles" ..`
    * `cmake --build .`
    * go to smspd folder and run `make`.
* From C:\msys64\mingw64\bin put next to sms.dll:
   * `libsndfile-1`
   * `libFLAC.dll`
   * `libgslcblas-0.dll`
   * `libogg-0.dll`
   * `libopus-0.dll`
   * `libvorbis-0.dll`
   * `libvorbisenc-2.dll`
   * `libgsl-27.dll`
   * `libsms.dll`
Yes, a lot of dependencies... 

### COPYING
For copying information, please see the file COPYING included with this package.

Where to send Questions, Comments, Love and Support
Send emails to Rich Eakin: rich.eakin@gmail.com
