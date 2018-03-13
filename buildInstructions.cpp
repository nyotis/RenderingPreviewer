1. Clone repository

   git clone $PATH/framebufferTest https://github.com/nyotis/RenderingPreviewer


2. Build dependencies [1].

   git submodule init
   git submodule update


3. Build framebuffer

   scons debug=1 with-arnold=$ARNOLD_PATH -j2


4. Set RV version TO 4.0.10 and install the MayaRender Mode RV package

   setrv 4.0.10
   export RV_SUPPORT_PATH=PATH/framebufferTest/rv

   cd PATH/framebufferTest/rvSpecific/mayarenderMode.mu
   ./createpackage
   cd ../
   rvpkg -install ./Packages/mayarender-1.0.rvpkg

Open RV and make sure you get a "Load Maya Render Mode loaded" in RVs console 
without any errors. If Maya Render Mode Menu does not appear in RV, go to 
Packages Tab under RV Preferences Window and make sure Maya Render Support is 
Installed and Loaded. 


5. Set Arnold path so that the dynamic library gets picked at runtime

   echo $ARNOLD_VERSION
   export ARNOLD_PLUGIN_PATH=PATH/framebufferTest/debug/x64


6. Test the framebuffer

Open maya and load one *.ma | *.mb scene to test the buffer.

Execute the Python script [missing] in Mayas console. RVlive button should appear 
on the right of Arnold-to-Mplay in Maya. Push the RVlive button and a progressive 
rendering of the scene in RV should appear.

Set as Maya Port the one you picked earlier with the python script, say 4700.
Set Port to 4700​ under Maya Render Menu -> Configure in RV. Now, you can trigger 
renders directly from RV: right click and pick Maya Render -> Render. 
Hit x, while in RV to see the Folder/Sources/Stacks structure

When executing the python script with a different port number, a new RVlive 
button will be generated.

Draw a region of interest (ROI) by dragging the middle mouse button in RV, while 
simultaneously holding the Shift button. A red rectangular region should appear, 
right-click and pick Maya Render -> Render Region.

Feel free to iterate with different scenes, image sizes. 
Any trouble/failure/success, do let me know.


Nikolaos Giotis


[1] Gaetan Guidet https://github.com/gatgui/arnold-rv