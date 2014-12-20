see Wiki


=---- Nik Yotis, 20/Dec/2014

see Wiki

=---- Gaetan Guidet's arnold-rv version, 2013

Hi there,

A recent mail from the RV support telling me the support case I opened for the RV 4.0 memory leak issue was closed 
reminded me that I had forgotten to notify the list that the issue was resolved a couple of months ago with RV 4.0.10.

For those interested, I have forked the original arnold-rv repository (thanks Chad!) and re-worked it a bit. 
It is accessible at https://github.com/gatgui/arnold-rv

Here are the modifications:
- removed dependency to boost libraries, introoduced others to gnet, gcore, excons
- support compiling with arnold 4.1
- when using a fixed media name, stack consecutive renders as frames in the media
- added option to add timestamp to media name (one might not necessarily want renders to be stacked as frames)
- re-order AOVs so that RGBA always comes first
- added a few color correction parameter:
color_correction: one of None, sRGB, Rec709, Gamma 2.2, Gamma 2.4, Custom Gamma, LUT or OCIO)
gamma: Used when mode is Custom Gamma. The ARNOLD_RV_DRIVER_GAMMA environment variable to control it
lut: Used when mode is LUT. The ARNOLD_RV_DRIVER_LUT environment variable to control it
ocio_profile: Use when mode is OCIO. The OCIO environment variable to control it)    

Note: To switch between OCIO and any other color correction mode, RV has to be re-started 
(the driver will start rv with the approriate command line when necessary)
- respond to RV ping signal
- addition of an RV module to start maya render/ipr (including region) from RV
Note: Works using the maya command port, so you need have one open. I also added a simple script to opens one if necessary. 
Also. whatever you do, maya render view will popup but I couldn't find a way to avoid that 
(it actually goes as far as to make maya crash in some cases).

Cheers,
Gaetan


=--- Chad Dombrova

=======================
Arnold RV Driver
=======================

This is a driver for sending Arnold AOVs to RV.

It uses boost asio for communicating with RV over sockets.

-----------------------
Disclaimer
-----------------------

At the time that I was writing this code (a year ago?) we discovered a crippling memory leak in RV
pertaining to its protocol for sending buffers over sockets.  We alerted Tweak, but it seemed there
was no progress on the issue. Hence, the unfinished state of the code. Perhaps it is fixed now?

Let me know if it works for you without issue, because I would love to actually use this in production.

All I ask is that if you do improve it, please fork and commit your changes back to github.


