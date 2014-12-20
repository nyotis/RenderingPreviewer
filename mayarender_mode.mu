module: mayarender_mode {
   /*In order to create a new mode you need to create a new module and derive 
   MayaRenderMode class from the MinorMode class in the rvtypes module*/

require rvtypes;          // rvtypes.MinorMode
require commands;         // Mu commands (rv.commands)

require extra_commands;   // rv.extra_commands

require rvui;             // rvui.setupProjection
require gl;               // method: displayRegion
require system;           // package system
require qt;               // UI
require io;
require runtime;          // rv.runtime
require python;

class: MayaRenderMode : rvtypes.MinorMode {

   bool _debug, _paused;
   float _startx, _starty, _endx, _endy;
   float left, right, bottom, top; // need these globals for rendering ROI 

      // UI-related to setupMaya method
   qt.QDialog _setupDlg;
   qt.QLineEdit _hostWidget;
   qt.QLineEdit _portWidget;
      // UI-related to setupMaya method
   
   string _host;
   int _port;
   python.PyObject _send;

   method: deb (void; string msg) {

      if (_debug == true) {
         print("[Maya Render] %s\n" % msg);
      }
   }

   method: clearRegion (void;) {

      _startx = -1.0;
      _starty = -1.0;
      _endx   = -1.0;
      _endy   = -1.0;

      commands.redraw(); // redraw only schedules an update to happen. It does not actually force one
   }

     // sends string-format command to Maya
   method: sendMayaCommand(void; string cmd) {
    
      deb("Send command: %s" % cmd);
      if ( !python.is_nil(_send) ) {
         python.PyObject_CallObject(_send, (cmd, _host, _port, false, 0, _debug));
      }
   }

    \: clamp(float; float v)
    {
        if (v > 1.0) {
            return 1.0;
        }
        if (v < 0.0) {
            return 0.0;
        }
        return v;
    }

   method: singleRender (void; Event e) {
        
        rvtypes.State state = commands.data();
        
        let pinfo  = state.pixelInfo.front(),
            sName  = commands.sourceNameWithoutFrame(pinfo.name),
            ip     = state.pointerPosition;
        let (w, h, bits, ch, hasfloat, nplanes) = extra_commands.sourceImageStructure(sName);
          // (w)idth, (h)eight set at Maya's Render Settings
          //  
          // -- default values --
          // bits     32-bits
          // ch       4 channels 
          // hasfloat true -------> check HalfFloat
          // nplanes  1

        let image_aspect = float(w) / float(h);
        float coord1 = 0.00, coord2 = 0.00, coord3 = (h - 0.00005), coord4 = (w - 0.00005); 

        print(coord1);
        print(coord2);
        print(coord3);
        print(coord4);

        sendMayaCommand( "bb_render %f %f %f %f;" % ( coord1, coord2, coord3, coord4 ) );

   }

   method: singleRenderRegion (void; Event e) {
  
        rvtypes.State state = commands.data();
          let pinfo  = state.pixelInfo.front(),
              sName  = commands.sourceNameWithoutFrame(pinfo.name),
              ip     = state.pointerPosition;

       let (w, h, bits, ch, hasfloat, nplanes) = extra_commands.sourceImageStructure(sName);

       let percent_image_float_start = commands.eventToImageSpace(sName, rvtypes.Point(_startx, _starty), false);
          //print(percent_image_float_min);
          // [percent_image_float_min.x, percent_image_float_min.y]

       let percent_image_float_end = commands.eventToImageSpace(sName, rvtypes.Point(_endx, _endy), false);
       //print(percent_image_float_max);

       let image_aspect = float(w) / float(h);

       //float coord1 = percent_image_float_min.x * h;
         // should be equivalent to
       float image_startx = percent_image_float_start.x / image_aspect * w;
       float image_starty = percent_image_float_start.y * h;
       
       clamp(image_startx);
       clamp(image_starty);

       float image_endx = percent_image_float_end.x / image_aspect * w;
       float image_endy = percent_image_float_end.y * h;

       clamp(image_endx);
       clamp(image_endy);

          // invalid cases to take care of
          // ERROR   | invalid render region (39,129)-->(302,536) for image resolution 640x480

       print(image_startx);
       print(image_starty);
       print(image_endx);
       print(image_endy);

       // take into account user's drawing preference as for the region's starting/end points

/*
// case 1  
 
 1   2

 4   3

// case 2

 2   3

 1   4 

// case 3

 4   1

 3   2

// case 4

 3   4

 2   1
 */

       if ( ( ( image_startx - image_endx ) < 1.0 ) && ( image_starty > image_endy ) ) {
          
          // case 1
          sendMayaCommand( "bb_renderRegion %f %f %f %f;" % ( image_startx, image_endy, image_endx, image_starty ) );
          //print(image_startx - image_endx);
       }
       else if ( ( ( image_starty - image_endy ) < 1.0 ) && ( image_starty < image_endy ) ) {

          // case 2  
          sendMayaCommand( "bb_renderRegion %f %f %f %f;" % ( image_startx, image_starty, image_endx, image_endy ) );
          //print(image_starty - image_endy);  
       }
       else {
          sendMayaCommand( "bb_renderRegion %f %f %f %f;" % ( left, bottom, right, top ) );
       }
/*       else if ( ( ( image_starty - image_endy ) < 1.0 ) && ( image_startx > image_endx ) ) {
          // case 3
          print("case 3: ");  
          //sendMayaCommand( "bb_renderRegion %f %f %f %f;" % ( image_endx, image_starty, image_startx, image_endy ) );
       }
       else if ( ( ( image_startx - image_endx ) < 1.0 ) && ( image_starty < image_endy ) ) {
       // else if ( ( image_startx > image_endx ) &&  ( ( image_startx - image_endx ) > 1.0 ) ) { 
          // case 4
          print("case 4: ");
          sendMayaCommand( "bb_renderRegion %f %f %f %f;" % ( image_endx, image_starty, image_startx, image_endy ) );
       }
*//*       else
          sendMayaCommand( "bb_renderRegion %f %f %f %f;" % ( image_startx, image_endy, image_endx, image_starty ) );        */ 
   }


   method: cancelRender (void; Event e) {

      clearRegion();
      let rvlive = system.getenv("RV_RVLIVE", "rvlive"); 
      system.system("echo killall -v %s" % rvlive);
      //system.system("killall kick");
   }

   method: updateMayaSetup (void; ) {        

      _host = _hostWidget.text();
      _port = int( _portWidget.text() );
   }

     //=----------------------------------------------------------------------- UI ----=

   method: setupMaya (void; Event e) {

      if (_setupDlg eq nil) {
         string uipath = io.path.join( supportPath("mayarender_mode", "mayarender"), 
                                                   "maya_command_port.ui" );
         
         if ( !io.path.exists(uipath) ) {
            uipath = "maya_command_port.ui";

            if ( !io.path.exists(uipath) ) { //ugly repetition
               print("Cannot find \"maya_command_port.ui\" file\n");
               return;
            }
         }

         let m       = commands.mainWindowWidget();
         _setupDlg   = qt.loadUIFile(uipath, m);
         _hostWidget = _setupDlg.findChild("hostEdit");
         _portWidget = _setupDlg.findChild("portEdit");

         qt.connect(_setupDlg, qt.QDialog.accepted, updateMayaSetup);
      }

      _hostWidget.setText(_host);
      _portWidget.setText("%d" % _port);
      _setupDlg.show();
   }

     //=----------------------------------------------------------------------- UI ----=

     //=--------------------------------------------------------- Maya Render Menu ----=

   method: buildMenu (rvtypes.Menu; ) {

      rvtypes.Menu m1 = rvtypes.Menu {
         {"Render", singleRender, nil, nil},
         {"Render Region", singleRenderRegion, nil, nil},
         {"_", nil},

         {"Cancel Render", cancelRender, nil, nil},
         {"_", nil},

         {"Configure...", setupMaya, nil, nil}
      };

      rvtypes.Menu ml = rvtypes.Menu {
         {"Maya Render", m1}
      };

      return ml;
   }

     //=--------------------------------------------------------- Maya Render Menu ----=

//=-------------------------------------------------------------- Select ROI in RV -------------=

   method: startRegion (void; Event event) {

        // mouse event
      _startx = event.pointer().x;
      _starty = event.pointer().y;
      _endx   = -1.0;
      _endy   = -1.0;
        // mouse event

      deb("Start region from (%f, %f)" % (_startx, _starty));
      
      commands.redraw();      
      event.reject();   // allows save command to continue processing
   }

   method: growRegion (void; Event event) {
    
      if (_startx >= 0.0 && _starty >= 0.0) {

         _endx = event.pointer().x;
         _endy = event.pointer().y;
         deb("Grow region to (%f, %f)" % (_endx, _endy));

         commands.redraw();
      }
      event.reject();
   }

   method: endRegion (void; Event event) {
    
      deb("End region");

      _endx = event.pointer().x;
      _endy = event.pointer().y;
      deb("Grow region to (%f, %f)" % (_endx, _endy));

      PixelImageInfo[] pinf = commands.sourceAtPixel( event.pointer() );

      // cater for zero size ROI
      if ( pinf.size() == 0 ) {
          
        event.reject();
        return;
      }
      
      (vector float[2])[] geom = commands.imageGeometry( pinf[0].name );
         // [0] bottom-left  [1] bottom-right 
         // [2] top-right    [3] top-left

      float img_bottom = geom[0][1];
      float img_top    = geom[2][1];
      float img_left   = geom[0][0];
      float img_right  = geom[1][0];
      float img_inv_aspect;
      
      int img_w, img_h;

      // reject invalid cases
      if (img_bottom >= img_top || img_left >= img_right) {
        
        event.reject();
        return;
      }

      img_inv_aspect = (img_top  - img_bottom) / (img_right - img_left);
      img_w = (int)(img_right - img_left);
      img_h = (int)(img_top - img_bottom);

/*      float left, right, top, bottom, nleft, nright, ntop, nbottom;*/
      float nleft, nright, ntop, nbottom;
        // nxyz stands for normalized xyz

        // when user picks an invalid -out of canvas- ROI to render,
        // clamp ROI inside valid boundaries of the rendered image
      if (_startx < _endx) {

         left  = if _startx < img_left then img_left else _startx;
         right = if _endx > img_right then img_right else _endx;
      }
      else {
      
         left  = if _endx < img_left then img_left else _endx;
         right = if _startx > img_right then img_right else _startx;
      }
      if (_starty < _endy) {
      
         bottom = if _starty < img_bottom then img_bottom else _starty;
         top    = if _endy > img_top then img_top else _endy;
      }
      else {
      
         bottom = if _endy < img_bottom then img_bottom else _endy;
         top    = if _starty > img_top then img_top else _starty;
      }

        // reject invalid cases
      if (bottom >= top || left >= right) {

         event.reject();
         return;
      }

        // normalized coords
      top     = top - img_bottom;
      bottom  = bottom - img_bottom;
      left    = left - img_left;
      right   = right - img_left;

      ntop    = top / (img_top - img_bottom);
      nbottom = bottom / (img_top - img_bottom);
      nleft   = left / (img_right - img_left);
      nright  = right / (img_right - img_left);

      deb("Normalized area (%f, %f) -> (%f, %f)" % (nbottom, nleft, ntop, nright));

        // remap top and bottom range from [0, 1] to [0, 1 / image aspect ratio]
      ntop    = ntop * img_inv_aspect;
      nbottom = nbottom * img_inv_aspect;

      commands.redraw();
      event.reject();

      //sendMayaCommand( "bb_renderRegion %f %f %f %f;" % ( left, bottom, right, top ) );
      //singleRenderRegion(event);
   }

//=-------------------------------------------------------------- Select ROI in RV -------------=

//=-------------------------------------------------------- Display Region Of Interest in RV ----=

   method: displayRegion (void; Event event) {
    
      if (_startx >= 0 && _starty >= 0 && _endx >= 0 && _endy >= 0) {
         rvui.setupProjection(event);

         gl.glColor(1.0, 0.0, 0.0);
         gl.glBegin(gl.GL_LINE_STRIP);
         gl.glVertex(_startx, _starty);
         gl.glVertex(_endx, _starty);
         gl.glVertex(_endx, _endy);
         gl.glVertex(_startx, _endy);
         gl.glVertex(_startx, _starty);
         gl.glEnd();
      }
      
      event.reject();
   }

//=-------------------------------------------------------- Display Region Of Interest in RV ----=

//=----------------------------------------------------------- Render History Folder ------------=

   \: findOrMakeRenderFolder(string; ) {
        string folder = nil;
        let folders = commands.nodesOfType("RVFolderGroup");

        for_each (f; folders) {
            if (extra_commands.uiName(f) == "Render History") {
                folder = f;
            }
        }

        if (folder eq nil) {
         //  Could not find existing folder, make one
            folder = commands.newNode ("RVFolderGroup");
            extra_commands.setUIName (folder, "Render History");
            commands.setStringProperty (folder + ".mode.viewType", string[] {"layout"}, true);
        }
        return folder;
   }

   method: sourceGroupHandler (void; Event event) {
      
        event.reject();
        let newSourceGroup = event.contents().split(";;")[0],
            imageSources = extra_commands.nodesInGroupOfType( newSourceGroup, "RVImageSource" );
        if ( imageSources.size() == 0 ) {
        //  Not an ImageSource, so ignore.
            return;
        }

        //  Find or make folder containing render history and get list of current contents.
        let folder = findOrMakeRenderFolder(),
            folderContents = commands.nodeConnections (folder, false)._0;
        
        //  Ensure that all previous renders are in Folder
        string[] imageSourcesOutsideFolder;
        for_each ( s; commands.nodesOfType("RVImageSource") ) {
            let group = commands.nodeGroup(s),
                outside = true;

            for_each (s; folderContents) 
               if (s == group) outside = false;

            if (outside && group != newSourceGroup) {
               //  Add this render to the folder contents
               folderContents.push_back(group);
               commands.setNodeInputs(folder, folderContents);
            }
        } 
        //  Set the current view to be this new render
        //commands.setViewNode(newSourceGroup);
   }
//=----------------------------------------------------------- Render History Folder ------------=

//=----------------------------------------------------------- Render Stack Folder --------------=

   \: findOrMakeRenderStackFolder(string; ) {
    
     string folder = nil;
     let folders = commands.nodesOfType("RVStackGroup");

     for_each (f; folders) {
         if (extra_commands.uiName(f) == "Render Stack") {
             folder = f;
         }
     }

     if (folder eq nil) {
           // if folder doesn't exist, make one
         folder = commands.newNode ("RVStackGroup");
         extra_commands.setUIName (folder, "Render Stack");
     }
     return folder;
   }

   method: stackGroupHandler (void; Event event) {

     event.reject();  

     let newStackGroup = event.contents().split(";;")[0],
         imageSources = extra_commands.nodesInGroupOfType( newStackGroup, "RVImageSource" );
     if ( imageSources.size() == 0 ) {
           //  Not an ImageSource, so ignore.
         return;
     }

       // find or make folder containing Render History and get list of current contents.
     let folder = findOrMakeRenderStackFolder(),
         folderContents = commands.nodeConnections (folder, false)._0,
           //  start the new inputs list with the incoming render, so it sits on top:
         newContents = string[] { newStackGroup };
     
       // add previous contents to new contents:
     for_each ( s; folderContents) newContents.push_back(s);

       // make sure no previous render was left out of Stack contents
     for_each ( s; commands.nodesOfType("RVImageSource") ) {
         let group = commands.nodeGroup(s),
             outside = true;

         for_each (s; newContents) 
            if (s == group) outside = false;

         if (outside) {
              // add this render to the folder contents
            newContents.push_back(group);
            commands.setNodeInputs(folder, folderContents);
         }
     } 
     commands.setNodeInputs(folder, newContents);
       //  force the view to the Render Stack 
     //commands.setViewNode(folder);
   }

//=----------------------------------------------------------- Render Stack Folder --------------=

   method: sourceAndStackGroupHandler(void; Event event) {
    event.reject();
    sourceGroupHandler(event);
    stackGroupHandler(event);
   }

   method: MayaRenderMode(MayaRenderMode; string name) {
    
      init( name,
            nil,
            [( "pointer-2--shift--push",     startRegion, "" ),
             ( "pointer-2--shift--drag",     growRegion,  "" ),
             ( "pointer-2--shift--release",  endRegion,   "" ),

             ( "source-group-complete", sourceGroupHandler, "" ), 
             ( "source-group-complete", stackGroupHandler, "" ),
             ( "source-group-complete", sourceAndStackGroupHandler, "" ),
             ( "render", displayRegion, "" )],
             buildMenu() );

      _paused = false;
      _debug  = true;

      _startx = -1.0;
      _starty = -1.0;
      _endx   = -1.0;
      _endy   = -1.0;

      _host = "localhost";
      _port = 5000; // investigate issue with 4800 not working  

      let mod = python.PyImport_Import("sendmaya"); 
      if ( !python.is_nil(mod) ) {

         _send = python.PyObject_GetAttr(mod, "command");
            
         if ( python.is_nil(mod) ) {
            print( "\"command\" function not found in \"sendmaya\" python module\n" );
         }
      }
      else {
         print( "Python \"sendmaya\" module not found\n" );
      }
   }
}

   \: createMode (rvtypes.Mode;) {
    
      print("Load mayarender\n");
      return MayaRenderMode("mayarender_mode");
   }

   \: theMode (MayaRenderMode; ) {
    
      MayaRenderMode m = rvui.minorModeFromName("mayarender_mode");
      return m;
   }
}