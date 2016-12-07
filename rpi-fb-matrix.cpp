// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
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

// Global to keep track of if the program should run.
// Will be set false by a SIGINT handler when ctrl-c is
// pressed, then the main loop will cleanly exit.
volatile bool running = true;

// Class to encapsulate all the logic for capturing an image of the Pi's primary
// display.  Manages all the BCM GPU and CPU resources automatically while in scope.
class BCMDisplayCapture {
public:
  BCMDisplayCapture(int width=-1, int height=-1):
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
    // If no width and height were specified then grab the entire screen.
    if ((_width == -1) || (_height == -1)) {
      _width = display_info.width;
      _height = display_info.height;
    }
    // Create a GPU image surface to hold the captured screen.
    uint32_t image_prt;
    _screen_resource = vc_dispmanx_resource_create(VC_IMAGE_RGB888, _width, _height, &image_prt);
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

static void sigintHandler(int s) {
  running = false;
}

static void usage(const char* progname) {
    std::cerr << "Usage: " << progname << " [flags] [config-file]" << std::endl;
    std::cerr << "Flags:" << std::endl;
    rgb_matrix::RGBMatrix::Options matrix_options;
    rgb_matrix::RuntimeOptions runtime_options;
    runtime_options.drop_privileges = -1;  // Need root
    rgb_matrix::PrintMatrixFlags(stderr, matrix_options, runtime_options);
}

int main(int argc, char** argv) {
  try {
    // Initialize from flags.
    rgb_matrix::RGBMatrix::Options matrix_options;
    rgb_matrix::RuntimeOptions runtime_options;
    runtime_options.drop_privileges = -1;  // Need root
    if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                           &matrix_options, &runtime_options)) {
      usage(argv[0]);
      return 1;
    }

    // Read additional configuration from config file if it exists
    Config config(&matrix_options, argc >= 2 ? argv[1] : "/dev/null");
    cout << "Using config values: " << endl
         << " display_width: " << config.getDisplayWidth() << endl
         << " display_height: " << config.getDisplayHeight() << endl
         << " panel_width: " << config.getPanelWidth() << endl
         << " panel_height: " << config.getPanelHeight() << endl
         << " chain_length: " << config.getChainLength() << endl
         << " parallel_count: " << config.getParallelCount() << endl;

    // Set screen capture state depending on if a crop region is specified or not.
    // When not cropped grab the entire screen and resize it down to the LED display.
    // However when cropping is enabled instead grab the entire screen (by
    // setting the capture_width and capture_height to -1) and specify an offset
    // to the start of the crop rectangle.
    int capture_width = config.getDisplayWidth();
    int capture_height = config.getDisplayHeight();
    int x_offset = 0;
    int y_offset = 0;
    if (config.hasCropOrigin()) {
      cout << " crop_origin: (" << config.getCropX() << ", " << config.getCropY() << ")" << endl;
      capture_width = -1;
      capture_height = -1;
      x_offset = config.getCropX();
      y_offset = config.getCropY();
    }


    // Initialize matrix library.
    // Create canvas and apply GridTransformer.
    RGBMatrix *canvas = CreateMatrixFromOptions(matrix_options, runtime_options);
    if (config.hasTransformer()) {
      canvas->ApplyStaticTransformer(config.getGridTransformer());
    }
    canvas->Clear();

    // Initialize BCM functions and display capture class.
    bcm_host_init();
    BCMDisplayCapture displayCapture(capture_width, capture_height);

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
          displayCapture.getPixel(x+x_offset, y+y_offset, &red, &green, &blue);
          canvas->SetPixel(x, y, red, green, blue);
        }
      }
      // Sleep for 25 milliseconds (40Hz refresh)
      usleep(25 * 1000);
    }
    canvas->Clear();
    delete canvas;
  }
  catch (const exception& ex) {
    cerr << ex.what() << endl;
    usage(argv[0]);
    return -1;
  }
  return 0;
}
