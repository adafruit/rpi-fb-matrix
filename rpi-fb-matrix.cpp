// Program to copy the contents of the Raspberry Pi primary display to LED matrices.
// Author: Tony DiCola
#include <iostream>
#include <stdexcept>

#include <bcm_host.h>
#include <fcntl.h>
#include <led-matrix.h>
#include <linux/fb.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "Config.h"
#include "GridTransformer.h"

using namespace std;
using namespace rgb_matrix;


bool running = true;  // Global to keep track of if the program should run.
                      // Will be set false by a SIGINT handler when ctrl-c is
                      // pressed, then the main loop will cleanly exit.


// Class to encapsulate all the logic for capturing an image of the Pi's primary
// display.  Manages all the BCM GPU and CPU resources automatically while in scope.
class BCMDisplayCapture {
public:
  BCMDisplayCapture(int width, int height):
    _width(width),
    _height(height),
    _display(0),
    _screen_resource(0),
    _screen_data(NULL)
  {
    // Get information about primary/HDMI display.
    _display = vc_dispmanx_display_open(0);
    if (!_display) {
      throw runtime_error("Unable to open primary display!");
    }
    DISPMANX_MODEINFO_T display_info;
    if (vc_dispmanx_display_get_info(_display, &display_info)) {
      throw runtime_error("Unable to get primary display information!");
    }
    cout << "Primary display:" << endl
         << " resolution: " << display_info.width << "x" << display_info.height << endl
         << " format: " << display_info.input_format << endl;
     // Create a GPU image surface to hold the captured screen.
     uint32_t image_prt;
     _screen_resource = vc_dispmanx_resource_create(VC_IMAGE_RGB888, width, height, &image_prt);
     if (!_screen_resource) {
       throw runtime_error("Unable to create screen surface!");
     }
     // Create a rectangular region of the captured screen size.
     vc_dispmanx_rect_set(&_rect, 0, 0, _width, _height);
     // Allocate CPU memory for copying out the captured screen.  Must be aligned
     // to a larger size because of GPU surface memory size constraints.
     _pitch = ALIGN_UP(_width*3, 32);
     _screen_data = new uint8_t[_pitch*_height];
  }

  void capture() {
    // Capture the primary display and copy it from GPU to CPU memory.
    vc_dispmanx_snapshot(_display, _screen_resource, (DISPMANX_TRANSFORM_T)0);
    vc_dispmanx_resource_read_data(_screen_resource, &_rect, _screen_data, _pitch);
  }

  void getPixel(int x, int y, uint8_t* r, uint8_t* g, uint8_t* b) {
    // Grab the requested pixel from the last captured display image.
    uint8_t* row = _screen_data + (y*_pitch);
    *r = row[x*3];
    *g = row[x*3+1];
    *b = row[x*3+2];
  }

  ~BCMDisplayCapture() {
    // Clean up BCM and other resources.
    if (_screen_resource != 0) {
      vc_dispmanx_resource_delete(_screen_resource);
    }
    if (_display != 0) {
      vc_dispmanx_display_close(_display);
    }
    if (_screen_data != NULL) {
      delete[] _screen_data;
    }
  }

private:
  int _width,
      _height,
      _pitch;
  DISPMANX_DISPLAY_HANDLE_T _display;
  DISPMANX_RESOURCE_HANDLE_T _screen_resource;
  VC_RECT_T _rect;
  uint8_t* _screen_data;
};


void sigintHandler(int s) {
  running = false;
}

int main(int argc, char** argv) {
  try
  {
    // Expect one command line parameter with display configuration filename.
    if (argc != 2) {
      throw runtime_error("Expected configuration file name as only command line parameter!\r\nUsage: rpi-fb-matrix /path/to/display/config.cfg");
    }

    // Load the configuration.
    Config config(argv[1]);
    cout << "Using config values: " << endl
         << " display_width: " << config.getDisplayWidth() << endl
         << " display_height: " << config.getDisplayHeight() << endl
         << " panel_width: " << config.getPanelWidth() << endl
         << " panel_height: " << config.getPanelHeight() << endl
         << " chain_length: " << config.getChainLength() << endl
         << " parallel_count: " << config.getParallelCount() << endl;

    // Initialize matrix library.
    GPIO io;
    if (!io.Init()) {
      throw runtime_error("Failed to initialize rpi-led-matrix library! Make sure to run as root with sudo.");
    }

    // Create canvas and apply GridTransformer.
    RGBMatrix canvas(&io, config.getPanelHeight(), config.getChainLength(),
                     config.getParallelCount());
    GridTransformer grid = config.getGridTransformer();
    canvas.SetTransformer(&grid);
    canvas.Clear();

    // Initialize BCM functions and display capture class.
    bcm_host_init();
    BCMDisplayCapture displayCapture(config.getDisplayWidth(),
                                     config.getDisplayHeight());

    // Loop forever waiting for Ctrl-C signal to quit.
    signal(SIGINT, sigintHandler);
    cout << "Press Ctrl-C to quit..." << endl;
    while (running) {
      // Capture the current display image.
      displayCapture.capture();
      // Loop through the frame data and set the pixels on the matrix canvas.
      for (int y=0; y<config.getDisplayHeight(); ++y) {
        for (int x=0; x<config.getDisplayWidth(); ++x) {
          uint8_t red, green, blue;
          displayCapture.getPixel(x, y, &red, &green, &blue);
          canvas.SetPixel(x, y, red, green, blue);
        }
      }
      // Sleep for 25 milliseconds.
      usleep(25 * 1000);
    }
    canvas.Clear();
  }
  catch (const exception& ex) {
    cerr << ex.what() << endl;
    return -1;
  }
  return 0;
}
