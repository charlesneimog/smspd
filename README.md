## SMS for Pure Data (Pd)
version 0.95 -- August 11, 2009 
* updated by Charles K. Neimog in 15-dec-2022

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

### TO INSTALL 
To compile type in main directory: "scons". To install type: "sudo scons install",
which will default to installing the files into /usr/local/lib/pd . If, instead you wanted 
to install in a different directory, for example in the Pd Extended directory on Mac OS X,
you can use something like: 

sudo scons install pd=/Applications/Pd-extended.app/Contents/Resources

Alternatively, you can tell pd where the [sms] external and help files are.

To get the sms external libary to load in pd, you have to do one of the following:
1. add "loadlibNUM sms" to either your .pdsettings file (linux) or info.plist file (mac), 
    and make sure you change the nloadlib command to equal NUM
2. start pd with something like "pd -lib sms"
3. make the [sms] object in your patch before making other objects in the library

You need to first build libsms, which is at the same url that you found this package 
(above). scons needs to know where libsms.a is, if it is not in a system wide location 
(like /usr/local/lib) then you can provide the location by typing "scons sms=/PATH/TO/LIB".
Likewise, scons also needs to know where sms.h is.  If you installed libsms with
default settings and a "sudo scons install", everything should be okay. 

---------- DEPENDENCIES -----------------------------------------------------------------------
- scons: http://www.scons.org/ 
- the pd header (m_pd.h) as well as pd if you plan to use the externals:
      http://crca.ucsd.edu/~msp/software.html
- libsndfile  ( http://www.mega-nerd.com/libsndfile/ )
- libpthread (standard on posix systems)

----------- KNOWN BUGS -------------------------------------------------------------------------
- Don't know of any, email me if you find anything

----------- COPYING -------------------------------------------------------------------------------
For copying information, please see the file COPYING included with this package.

----------- Where to send Questions, Comments, Love and Support --------------------
Send emails to Rich Eakin: rich.eakin@gmail.com
