#include <ai.h>
   // Arnold 

#include <sstream>
#include <algorithm>
   // STL

#include <gcore/gcore>
#include <gnet/gnet>
   // external dependencies

#define EXTENDED_DRIVER_API 
   // needed for RV version >= 4.0.10

#define FORCE_VIEW_NAME
   // RV version < 4.0.10 will crash when a View Name is not defined 

AI_DRIVER_NODE_EXPORT_METHODS(RVDriverMtd);
   // Output Driver node methods exporter

class Message {

public:
   typedef void ( *FreeFunc )( void* );

   Message() : mData(0), mLength(0), mFree(0) {}   
   Message(void* data, size_t length, FreeFunc freeData = 0) : mData(data), 
                                                               mLength(length), 
                                                               mFree(freeData) {}
   Message(const Message &rhs) : mData(rhs.mData), 
                                 mLength(rhs.mLength),
                                 mFree(rhs.mFree) {
      rhs.mFree = 0;
   }
   ~Message() {

      if (mData && mFree) {
         mFree(mData);
      }
   }
   
   Message& operator=(const Message &rhs) {  
      
      if (this != &rhs) {

         mData     = rhs.mData;
         mLength   = rhs.mLength;
         mFree     = rhs.mFree;
         rhs.mFree = 0;
      }
      return *this;
   }
   
   const char* bytes() const {
      return reinterpret_cast<const char*>(mData);
   }
   
   size_t length() const {
      return mLength;
   }
   
private:
   void* mData;
   size_t mLength;
   mutable FreeFunc mFree;
};

class Client {

public:
   Client(const std::string extraArgs="") : mSocket( 0 ), 
                                            mConn( 0 ),
                                            mRVStarted( false ),
                                            mExtraArgs( extraArgs ) {}
   ~Client() {

      close();
      if (mSocket) {
         delete mSocket;
      }
   }
   
   bool connect( std::string hostname, 
                 int port, 
                 bool runRV            = true, 
                 bool silent           = false, 
                 int maxRetry          = 10, 
                 unsigned long waitFor = 1000 )  {
      try {

         gnet::Host host(hostname, port);
         
         if ( mSocket ) {
            
            if ( mSocket->host().address() != host.address() || 
                 mSocket->host().port() != host.port() ) {
            
               close();
               delete mSocket;
               mSocket = 0;
            }
            else if (mConn) {
            
               if ( mConn->isValid() ) {

                  return true;
               }
               else {
                     // keep socket but close connection
                  mSocket->closeConnection(mConn);
                  mConn = 0;
               }
            }
         }
         
         if ( !mSocket ) {

            mSocket = new gnet::TCPSocket(gnet::Host(hostname, port));
         }
         
         mConn = mSocket->connect();
      }
      catch (std::exception &err)
      {
         if (mSocket) {
               // No risk having mConn != 0 here     
            delete mSocket;
            mSocket = 0;
         }
         
         if (runRV) {
            
            gcore::String cmd = "rv -network -networkPort " + gcore::String(port);
            if ( mExtraArgs.length() > 0 ) {
            
               cmd += " ";
               cmd += mExtraArgs;
            }
            
            gcore::Process p;

            p.keepAlive(   true );
            p.showConsole( true );
            p.captureOut( false );
            p.captureErr( false );
            p.redirectIn( false );

            if ( p.run(cmd) == gcore::INVALID_PID ) {
               return false;
            }
            
            int retry = 0;
            while (retry < maxRetry) {

               gcore::Thread::SleepCurrent(waitFor);
               if ( connect(hostname, port, false, true, 0, 0) ) {
                  break;
               }
               ++retry;
            }
            
            if (retry >= maxRetry) {
               if (!silent) {
                  return false;
               }
            }

            mRVStarted = true;
         }
         else {
            return false;
         }
      }
      
      return true;
   } // connect
   
   void write(Message& msg) {

      if (!isAlive()) {
         return; // try better
      }
            
      try {
         mConn->write(msg.bytes(), msg.length());
      }
      catch (std::exception &err) {
         close();
      }
   }

   void write(const std::string& msg) {

      if ( !isAlive() ) {
         return; // try better
      }
      
      try {

         mConn->write(msg.c_str(), msg.length());
      }
      catch (std::exception &err) {

         AiMsgWarning("[BB_RV_Driver] Error while writing: %s", err.what());
         close();
      }
   }  
   
   void writeMessage(const std::string& msg) {

      std::ostringstream oss;
      oss << "MESSAGE " << msg.size() << " " << msg;
      write(oss.str());
   }
   
   void readOnce(std::string &s) {

      if ( isAlive() ) {
         
         try {
            mConn->reads(s);
         }
         catch (std::exception &e) {

            AiMsgWarning( "[BB_RV_Driver] Read failed: %s", e.what() );
            s = "";
         }
      }
      else {
         s = "";
      }
   }
   
   void read() {

      bool done = false;
      char* bytes = 0;
      size_t len = 0;
      
      try {
         
         while (isAlive() && !done) {
            
            mConn->read(bytes, len);
            done = (len == 0);
            
            if (bytes) {

                  // beware of multi-threading if you need to write to the socket
               if ( len >= 8 && !strncmp( bytes, "GREETING", 8 ) ) {}
               else if ( len >= 8 && !strncmp( bytes, "PING 1 p", 8 ) ) {

                  mConn->write( "PONG 1 p", 8 );
               }
               else if ( len >= 7 && !strncmp( bytes, "MESSAGE", 7 ) ) {

                  if ( strstr( bytes, "DISCONNECT" ) != 0 ) {

                     free(bytes);
                     break;
                  }
               }
               free(bytes);
               bytes = 0;
            }
         }
      }
      catch ( gnet::Exception &e ) {

         AiMsgDebug( "[BB_RV_Driver] Read thread terminated: %s", e.what() );
      }
   }
   
   void close() {
      
      if (mSocket && mConn) {

         mSocket->closeConnection(mConn);
            // closeConnection may not neccessarily delete mConn if it is not connected to it
         mConn = 0;
      }
   }
   
   bool isAlive() const {
      return ( mSocket && mConn && mConn->isValid() );
   }
      
   bool startedRV() const {
      return mRVStarted;
   }
      
private:
   gnet::TCPSocket *mSocket;
   gnet::TCPConnection *mConn;

   bool mRVStarted;
   std::string mExtraArgs;
}; // class Client

unsigned int ReadFromConnection(void *data) {

   Client* client = (Client*) data;
   client->read();
   return 1;  //try better
}

void FreeTile(void *data) {

   AtRGBA* pixels = (AtRGBA*) data;
   delete[] pixels;
}

namespace {

    enum RVDriverParams {
   
       p_host = 0,
       p_port,
       p_extra_args,
       p_media_name,
       p_add_timestamp,
    };
}

struct ShaderData {

   void* thread;
   Client* client;
   std::string* media_name;
   int nchannels;
   int frame;
   
   ShaderData() : thread(NULL), 
                  client(NULL), 
                  media_name(NULL), 
                  nchannels(-1), 
                  frame(1) {}
};

node_parameters {

   AiParameterSTR( "host",       "localhost" );
   AiParameterINT( "port",       45124       ); // RV defaults to 45124
   AiParameterSTR( "extra_args", ""          );
   AiParameterSTR( "media_name", ""          );
   
   AiParameterBOOL( "add_timestamp", false);
      
   AiMetaDataSetBool( mds, NULL, "display_driver",      true  );
   AiMetaDataSetBool( mds, NULL, "single_layer_driver", false );
   
      // Maya-to-Arnold specific metadata 
   AiMetaDataSetStr( mds, NULL, "maya.name",        "aiAOVDriver" );
   AiMetaDataSetStr( mds, NULL, "maya.translator",  "rv"          );
   AiMetaDataSetStr( mds, NULL, "maya.attr_prefix", "rv_"         );
}

node_initialize {

   ShaderData* data = (ShaderData*) AiMalloc( sizeof(ShaderData) );
   
   data->thread     = NULL;
   data->client     = NULL;
   data->media_name = NULL;
   data->nchannels  = -1;
   
   AiDriverInitialize(node, true, data);
}

node_update {
   //
}

driver_supports_pixel_type {
   return true;
}

driver_extension {
   return NULL;
}

driver_open {
   
   ShaderData *data = (ShaderData*) AiDriverGetLocalData(node);   
   const char* host = AiNodeGetStr(node, "host");

   int port = AiNodeGetInt(node, "port");
   bool use_timestamp = AiNodeGetBool(node, "add_timestamp");

      // Create client if needed
   if ( data->client == NULL ) {
      data->client = new Client( AiNodeGetStr(node, "extra_args") );
   }
   else {
         // assume we're still connected
      return;
   }
      
   if ( !data->client->connect(host, port) ) {

      AiMsgWarning("[BB_RV_Driver] Could not connect to %s:%d", host, port);
      if (data->media_name != NULL) {
         delete data->media_name;
      }
      data->media_name = NULL;
      data->thread     = NULL;
      return;
   }
   

   if ( data->media_name == NULL ) {

         // Generate a unique media name, which cannot contain spaces. Everything following the last 
         // slash is stripped from the name displayed in RV, so we put the timestamp in front 
         // followed by a slash so that it won't clutter the catalogue.
      std::string mn = AiNodeGetStr(node, "media_name");
         // add time stamp
      
      if ( mn.length() == 0 ) {
         mn = gcore::Date().format("%Y-%m-%d_%H:%M:%S");
      }
      else if ( use_timestamp ) {
         mn += "_" + gcore::Date().format("%Y-%m-%d_%H:%M:%S");
      }
      
      data->media_name = new std::string(mn);
   }
   
   std::ostringstream oss;
   
      // Send PID greeting to RV
   int pid = getpid();
   std::string greeting_pid;

   std::ostringstream convert;
   convert << pid;
   greeting_pid = convert.str();
   std::string greeting = "rv-shell-1 " + greeting_pid + " arnold";
      // Send PID greeting to RV
   
   oss << "NEWGREETING " << greeting.size() << " " << greeting;
   data->client->write( oss.str() );
   
   data->client->readOnce( greeting );
         
      // Create the list of AOVs in Mu syntax
      // mayarender_mode.mu needs access into this 
   int pixel_type;
   const char* aov_name;
   std::string aov_names = "";
   std::string aov_cmds  = "";
   data->nchannels = 0;
   unsigned int i  = 0;
   
   while ( AiOutputIteratorGetNext( iterator, &aov_name, &pixel_type, NULL ) ) {

/*      oss.str(""); 
      oss << "newImageSourcePixels(src, frame, \"" << aov_name << "\", \"master\");\n";
*/
         // Make RGBA the 'top' layer
      if ( !strcmp( aov_name, "RGBA" ) ) {
         
         if ( i > 0 ) {
            aov_names += ",";
         }
         aov_names += std::string("\"") + aov_name + "\"";
/*         aov_cmds  += oss.str();
*/      }
      else {
         
         if (i > 0) {
            aov_names = std::string("\"") + aov_name + "\"," + aov_names;
         }
         else {
            aov_names = std::string("\"") + aov_name + "\"";
         }
/*         aov_cmds = oss.str() + aov_cmds;*/
      }
      ++i;
   }
   
   data->nchannels = 4;
      // Convert to RGBA, because RV does not allow layers containing differing types/channels

      // No need to set the data window for region renders, because 
      // the tiles place themselves appropriately within the image.
   oss.str( "" );
   oss << "RETURNEVENT remote-eval * ";
   oss << "{ string media = \"" << *(data->media_name) << "\";" << "\n";
   oss << "  bool found = false;" << "\n";
   oss << "  string src = \"\";" << "\n";
   oss << "  int frame = 1;" << "\n";
   oss << "  for_each (source; nodesOfType(\"RVImageSource\")) {" << "\n";
   oss << "    if (getStringProperty(\"%s.media.name\" % source)[0] == media) {" << "\n";
   oss << "      found = true;" << "\n";
   oss << "      src = source;" << "\n";
   oss << "      break;" << "\n";
   oss << "    }" << "\n";
   oss << "  }" << "\n";
   oss << "  if (!found) {" << "\n";
   oss << "    src = newImageSource(media, ";  // name

   oss << ( display_window.maxx - display_window.minx + 1 ) << ", ";  // w
   oss << ( display_window.maxy - display_window.miny + 1 ) << ", ";  // h
   oss << ( display_window.maxx - display_window.minx + 1 ) << ", ";  // uncrop w
   oss << ( display_window.maxy - display_window.miny + 1 ) << ", ";  // uncrop h
   
   oss << "0, 0, 1.0, ";                       // x offset, y offset, pixel aspect
   oss << data->nchannels << ", ";             // channels
   oss << "32, true, 1, 1, 24.0, ";            // bit-depth, floatdata, start frame, end frame, frame rate
   oss << "string[] {" << aov_names << "}, ";  // layers

   oss << "string[] {\"master\"});" << "\n";   // views [Note: without a view name defined, 
                                               // RV 4 (< 4.0.10) will just crash]

   oss << aov_cmds << "\n";  // source pixel commands
   oss << "  } else {" << "\n";
   oss << "    frame = getIntProperty(\"%s.image.end\" % src)[0] + 1;" << "\n";
   oss << "    setIntProperty(\"%s.image.end\" % src, int[]{frame});" << "\n";
   oss << aov_cmds << "\n";
   oss << "  }" << "\n";

   oss << "  setViewNode(nodeGroup(src));" << "\n";
   oss << "  setFrameEnd(frame);" << "\n";
   oss << "  setOutPoint(frame);" << "\n";
   oss << "  setFrame(frame);" << "\n";
   oss << "  frame;" << "\n";
   oss << "}" << "\n";
   
   std::string cmd = oss.str();
   data->client->writeMessage(cmd);
   
   std::string ret;
   int msgsz = 0;
   
   data->client->readOnce(ret);
   
   if (data->thread == 0) {
      data->thread = AiThreadCreate( ReadFromConnection, (void*)data->client, AI_PRIORITY_NORMAL );
   }
} // driver_open

driver_needs_bucket {
   // called to determine whether a bucket will be rendered -the renderer locks around this function
   return true;
}

driver_prepare_bucket {} 

driver_process_bucket {
   // Called once a bucket has been rendered, but before it is written out
   // We could send something to RV here to denote the tile we're about to render
} 

driver_write_bucket {

   ShaderData *data = (ShaderData*) AiDriverGetLocalData(node);
   
   if (data->media_name == NULL) {
      AiMsgFatal( "driver open failed: check connection to port/host" );
      //return;
   }
   
   std::ostringstream oss;

   int yres = AiNodeGetInt( AiUniverseGetOptions(), "yres" );
   size_t tile_size = bucket_size_x * bucket_size_y * data->nchannels * sizeof(float);
   oss << "PIXELTILE(media=" << *(data->media_name);
   
   std::string layercmd1 = oss.str();
   oss.str("");

   oss << ",view=master,w=" << bucket_size_x << ",h=" << bucket_size_y;
   oss << ",x=" << bucket_xo << ",y=" << (yres - bucket_yo - bucket_size_y);  
      // flip bucket coordinates vertically

   std::string layercmd2 = oss.str();
   
   int pixel_type;
   const void* bucket_data;
   const char* aov_name;
   
   while ( AiOutputIteratorGetNext( iterator, &aov_name, &pixel_type, &bucket_data ) ) {
      AtRGBA* pixels = new AtRGBA[bucket_size_x * bucket_size_y];
      
      switch(pixel_type) {

         case AI_TYPE_RGBA: {

            for (int j = 0; j < bucket_size_y; ++j) {
               for (int i = 0; i < bucket_size_x; ++i) {

                  unsigned int in_idx = j * bucket_size_x + i;
                  unsigned int out_idx = (bucket_size_y - j - 1) * bucket_size_x + i; 
                     // Flip vertically
                  
                  AtRGBA* dest = &pixels[out_idx];
                  AtRGBA src = ((AtRGBA*)bucket_data)[in_idx];

                  dest->r = src.r;
                  dest->g = src.g;
                  dest->b = src.b;
                  dest->a = src.a;
               }
            }
            break;
         }
         case AI_TYPE_RGB:
         case AI_TYPE_VECTOR:
         case AI_TYPE_POINT: {
            
            for (int j = 0; j < bucket_size_y; ++j) {
               for (int i = 0; i < bucket_size_x; ++i) {

                  unsigned int in_idx = j * bucket_size_x + i;
                  unsigned int out_idx = (bucket_size_y - j - 1) * bucket_size_x + i;
                     // Flip vertically

                  AtRGBA* dest = &pixels[out_idx];
                  AtRGB src = ((AtRGB*)bucket_data)[in_idx];
                  dest->r = src.r;
                  dest->g = src.g;
                  dest->b = src.b;
                  dest->a = 0.0f;
               }
            }
            break;
         }
         case AI_TYPE_FLOAT: {
            
            for (int j = 0; j < bucket_size_y; ++j) {
               for (int i = 0; i < bucket_size_x; ++i) {
                  unsigned int in_idx = j * bucket_size_x + i;
                  unsigned int out_idx = (bucket_size_y - j - 1) * bucket_size_x + i;
                     // Flip vertically
                  
                  AtRGBA* dest = &pixels[out_idx];

                  float src = ((float*)bucket_data)[in_idx];
                  dest->r = src;
                  dest->g = src;
                  dest->b = src;
                  dest->a = 0.0f;
               }
            }
            break;
         }
      }
      
      oss.str("");
      oss << layercmd1 << ",layer=" << aov_name << layercmd2 << 
      ",f=" << data->frame << ") " << tile_size << " ";
      
      std::cout << oss.str() << "<data>" << "\n";
      Message msg(pixels, tile_size, FreeTile);
      
      data->client->write(oss.str());
      data->client->write(msg);
   }   
} // driver_write_bucket

driver_close {}

node_finish {
   
   ShaderData *data = (ShaderData*) AiDriverGetLocalData(node);
   
   if (data->media_name != NULL) {
      
      delete data->media_name;
      
      if ( data->client->isAlive() ) {
      
         data->client->write( "MESSAGE 10 DISCONNECT" );
      }
      
      AiThreadWait( data->thread);
      AiThreadClose(data->thread);
   }
   
   delete data->client;
   
   AiFree(data);
   AiDriverDestroy(node);
}

node_loader {
   
   if (i == 0)  {
   
      node->methods = (AtNodeMethods*) RVDriverMtd;
      node->output_type = AI_TYPE_RGBA;
      node->name = "BB_RV_Driver";
      node->node_type = AI_NODE_DRIVER;
      sprintf(node->version, AI_VERSION);
      return true;
   }
   else {
   
      return false;
   }
}