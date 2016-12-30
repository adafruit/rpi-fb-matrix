// Program to aid in the testing of LED matrix chains.
// Author: Tony DiCola
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <led-matrix.h>
#include <signal.h>
#include <unistd.h>

#include "Config.h"
#include "glcdfont.h"
#include "GridTransformer.h"

using namespace std;
using namespace rgb_matrix;


bool running = true;  // Global to keep track of if the program should run.
                      // Will be set false by a SIGINT handler when ctrl-c is
                      // pressed, then the main loop will cleanly exit.


void printCanvas(Canvas& canvas, int x, int y, const string& message,
                        int r = 255, int g = 255, int b = 255) {
  // Loop through all the characters and print them starting at the provided
  // coordinates.
  for (auto c: message) {
    // Loop through each column of the character.
    for (int i=0; i<5; ++i) {
      unsigned char col = glcdfont[c*5+i];
      x += 1;
      // Loop through each row of the column.
      for (int j=0; j<8; ++j) {
        // Put a pixel for each 1 in the column byte.
        if ((col >> j) & 0x01) {
          canvas.SetPixel(x, y+j, r, g, b);
        }
      }
    }
    // Add a column of padding between characters.
    x += 1;
  }
}

void sigintHandler(int s) {
  running = false;
}

int main(int argc, char** argv) {
  try
  {
    // Expect one command line parameter with display configuration filename.
    if (argc != 2) {
      throw runtime_error("Expected configuration file name as only command line parameter!\r\nUsage: display-test /path/to/display/config.cfg");
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

	cout << " grid rows: " << grid.getRows() << endl
	     << " grid cols: " << grid.getColumns() << endl;

    // Clear the canvas, then draw on each panel.
    canvas.Fill(0, 0, 0);
    for (int j=0; j<grid.getRows(); ++j) {
      for (int i=0; i<grid.getColumns(); ++i) {
        // Compute panel origin position.
        int x = i*config.getPanelWidth();
        int y = j*config.getPanelHeight();
        // Print the current grid position to the top left (origin) of the panel.
        stringstream pos;
        pos << i << "," << j;
        printCanvas(canvas, x, y, pos.str());
      }
    }
    // Loop forever waiting for Ctrl-C signal to quit.
    signal(SIGINT, sigintHandler);
    cout << "Press Ctrl-C to quit..." << endl;
    while (running) {
      sleep(1);
    }
    canvas.Clear();
  }
  catch (const exception& ex) {
    cerr << ex.what() << endl;
    return -1;
  }
  return 0;
}
