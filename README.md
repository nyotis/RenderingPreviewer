see Wiki


Development History below

=-- Nik Yotis,     13/05/2014 --=

- Removed Color Correction for clarity. Color Correction can easily be done in a package like Nuke.
- TODO


=-- Gaetan Guidet, 16/03/2014 --= 

I have forked the original arnold-rv repository (thanks Chad!) and re-worked it a bit. 
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


=--- Chad Dombrova (original implementation)
https://github.com/LumaPictures/arnold-rv