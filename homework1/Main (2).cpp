/**
 * GUK IL KIM
 * 3020-8670-72
 * CSCI 576 HW1 
 * 2/11/24
*/

#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

/**
 * Display an image using WxWidgets.
 * https://www.wxwidgets.org/
 */

/** Declarations*/

/**
 * Class that implements wxApp
 */
class MyApp : public wxApp {
 public:
  bool OnInit() override;
};

/**
 * Class that implements wxFrame.
 * This frame serves as the top level window for the program
 */
class MyFrame : public wxFrame {
 public:
    MyFrame(const wxString &title, string imagePath, float zoomSpeed, float rotationAngle, int fps);

 private:
  void OnPaint(wxPaintEvent &event);
  void OnTimer(wxTimerEvent &event);
  wxTimer *v_timer;
  wxImage inImage;
  wxImage modifiedImage; //new image 
  wxScrolledWindow *scrolledWindow;
  int width;
  int height;
  double zoomFactor;
  float zoomSpeed;
  float rotationAngle;
  int fps;
  float totalAngle;
  int timecount; 
  //double storeFactor; 
};

/** Utility function to read image data */
unsigned char *readImageData(string imagePath, int width, int height);

/** Definitions */

/**
 * Init method for the app.
 * Here we process the command line arguments and
 * instantiate the frame.
 */
bool MyApp::OnInit() {
  wxInitAllImageHandlers();

  // deal with command line arguments here
  cout << "Number of command line arguments: " << wxApp::argc << endl;
 if (wxApp::argc != 5) {
    cerr << "The executable should be invoked with exactly one filepath "
            "argument. Example ./MyImageApplication '../../Lena_512_512.rgb'"
         << endl;
    exit(1);
  }
  string imagePath = wxApp::argv[1].ToStdString();
  float zoomSpeed = stof(wxApp::argv[2].ToStdString());
  float rotationAngle = stof(wxApp::argv[3].ToStdString());
  int fps = stoi(wxApp::argv[4].ToStdString());
  //float totalAngle = 0.0; 

  MyFrame *frame = new MyFrame("Image Display", imagePath, zoomSpeed, rotationAngle, fps);
  frame->Show(true);

  // return true to continue, false to exit the application
  return true;
}

/**
 * Constructor for the MyFrame class.
 * Here we read the pixel data from the file and set up the scrollable window.
 */
MyFrame::MyFrame(const wxString &title, string imagePath, float zoomSpeed, float rotationAngle, int fps)
    : wxFrame(NULL, wxID_ANY, title), zoomFactor(0.0), rotationAngle(rotationAngle), fps(fps), zoomSpeed(zoomSpeed), totalAngle(0.0), timecount(0){

  // Modify the height and width values here to read and display an image with
  // different dimensions.    
  width = 512;
  height = 512;

  unsigned char *inData = readImageData(imagePath, width, height);

  // the last argument is static_data, if it is false, after this call the
  // pointer to the data is owned by the wxImage object, which will be
  // responsible for deleting it. So this means that you should not delete the
  // data yourself.
  inImage.SetData(inData, width, height, false);

  // Set up the scrolled window as a child of this frame
  scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
  scrolledWindow->SetScrollbars(10, 10, width, height);
  scrolledWindow->SetVirtualSize(width, height);

  // Bind the paint event to the OnPaint function of the scrolled window
  scrolledWindow->Bind(wxEVT_PAINT, &MyFrame::OnPaint, this);

  // Set the frame size
  SetClientSize(width, height);

  // Set the frame background color
  SetBackgroundColour(*wxBLACK);
  v_timer = new wxTimer(this, wxID_ANY);
  Bind(wxEVT_TIMER, &MyFrame::OnTimer, this);
  v_timer->Start(1000 / fps);
}

/**
 * The OnPaint handler that paints the UI.
 * Here we paint the image pixels into the scrollable window.
 */
void MyFrame::OnPaint(wxPaintEvent &event) {
  wxBufferedPaintDC dc(scrolledWindow);
  scrolledWindow->DoPrepareDC(dc);

  if (modifiedImage.IsOk()) {  
  //cout << "modifiedImage is being displayed" << endl;
    wxBitmap modifiedBitmap(modifiedImage);
    dc.DrawBitmap(modifiedBitmap, 0, 0, false);
  } else {
    wxBitmap modifiedBitmap = wxBitmap(inImage);
    //wxBitmap modifiedBitmap(modifiedImage);
    dc.DrawBitmap(modifiedBitmap, 0, 0, false);
  }
  scrolledWindow->Refresh();
}

/**
 * Is a functin to calculate rotation
 *  and transformation of pixels.
 */
void Calc(int &x, int &y, int w, int h, double &zoomFactor, double pi, float &angle) {
  double xt, yt, xf,yf; 
  xt = x - 0.5 * w;  
  yt = y - 0.5 * h; 
  xf = (xt * cos(angle * (pi/180)) / zoomFactor - yt * sin(angle * (pi/180)) / zoomFactor);
  yf = (xt * sin(angle * (pi/180)) / zoomFactor + yt * cos(angle * (pi/180)) / zoomFactor);
  //round values to map to original image 
  x = (int)round(xf + 0.5 * w); 
  y = (int)round(yf + 0.5 * h); 
}

void MyFrame::OnTimer(wxTimerEvent& event) {
  timecount += 1;
  zoomFactor = 1 + (zoomSpeed - 1) * timecount;  
  //cout << "this is current zoomfactor: " << zoomFactor << endl;
  unsigned char* modData = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
  modifiedImage.SetData(modData, width, height, true);
  double pi = 3.14159265359;
  double xt, yt;
  totalAngle += rotationAngle; 
  //cout << "total angle: " << totalAngle << endl;
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
        int xn, yn, xf, yf;
        xf = x;
        yf = y;

        if (zoomFactor <= 0) {
          xf = 0; yf = 0;
          modifiedImage.SetRGB(x,y,0,0,0);
        } else { 
          Calc(xf, yf, width, height, zoomFactor, pi, totalAngle);
        }

        if (xf < 0 || xf >= width || yf < 0 || yf >= height) {
          int index = (x * width + y) * 3;
          modifiedImage.GetData()[index] = 0;  // R
          modifiedImage.GetData()[index + 1] = 0;  // G
          modifiedImage.GetData()[index + 2] = 0;  // B
          continue;
        } 

        //condition for zoom out 
        if (zoomSpeed < 1) {  
          if (xf >= 1 && xf < width - 1 && yf >= 1 && yf < height - 1) {
            //cout << "entering 3 x 3" << endl;
            double v1 = 0, v2 = 0, v3 = 0;
            int count = 0;
            for (int i = -1; i <= 1; ++i) {
              for (int j = -1; j <= 1; ++j) {
                v1 += inImage.GetData()[3 * ((xf + i) * width + (yf + j))];
                v2 += inImage.GetData()[3 * ((xf + i) * width + (yf + j)) + 1];
                v3 += inImage.GetData()[3 * ((xf + i) * width + (yf + j)) + 2];
                count++;
            }
          }
            //cout << "zoom out 3 x 3" << endl;
            int index = (x * width + y) * 3;
            modifiedImage.GetData()[index] = v1 / count;
            modifiedImage.GetData()[index + 1] = v2 / count;
            modifiedImage.GetData()[index + 2] = v3 / count;
            continue; 
            }
          } else if (zoomSpeed > 1) {
            int index = (x * width + y) * 3;
            int data = 3 * (xf * width + yf);
            modifiedImage.GetData()[index] = inImage.GetData()[data];
            modifiedImage.GetData()[index + 1] = inImage.GetData()[data + 1];
            modifiedImage.GetData()[index + 2] = inImage.GetData()[data + 2];
            continue;
        }
      }
      //once looping finish, set background to black 
      modifiedImage.SetRGB(0,0,0,0,0);
    }
  modifiedImage.Destroy();
  modifiedImage.Create(width, height, modData, false);
  Refresh(false);
  Update();
}

/** Utility function to read image data */
unsigned char *readImageData(string imagePath, int width, int height) {

  // Open the file in binary mode
  ifstream inputFile(imagePath, ios::binary);

  if (!inputFile.is_open()) {
    cerr << "Error Opening File for Reading" << endl;
    exit(1);
  }

  // Create and populate RGB buffers
  vector<char> Rbuf(width * height);
  vector<char> Gbuf(width * height);
  vector<char> Bbuf(width * height);

  /**
   * The input RGB file is formatted as RRRR.....GGGG....BBBB.
   * i.e the R values of all the pixels followed by the G values
   * of all the pixels followed by the B values of all pixels.
   * Hence we read the data in that order.
   */

  inputFile.read(Rbuf.data(), width * height);
  inputFile.read(Gbuf.data(), width * height);
  inputFile.read(Bbuf.data(), width * height);

  inputFile.close();

  /**
   * Allocate a buffer to store the pixel values
   * The data must be allocated with malloc(), NOT with operator new. wxWidgets
   * library requires this.
   */
  unsigned char *inData =
      (unsigned char *)malloc(width * height * 3 * sizeof(unsigned char));
      
  for (int i = 0; i < height * width; i++) {
    // We populate RGB values of each pixel in that order
    // RGB.RGB.RGB and so on for all pixels
    inData[3 * i] = Rbuf[i];
    inData[3 * i + 1] = Gbuf[i];
    inData[3 * i + 2] = Bbuf[i];
  }

  return inData;
}

wxIMPLEMENT_APP(MyApp);