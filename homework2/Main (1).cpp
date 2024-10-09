// GUK IL KIM 
// HW 2 
// CSCI 576
// USC ID: 3020867072

#include <wx/wx.h>
#include <wx/dcbuffer.h>  
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

using namespace std;
namespace fs = std::filesystem;

class MyApp : public wxApp
{
public:
  bool OnInit() override;
};

class MyFrame : public wxFrame
{
public:
  MyFrame(const wxString &title, string imagePath, int mode, int totalBuckets);
  void uniformQuantization(unsigned char *inData, int width, int height, int totalBuckets);
  void nonUniformQuantization(unsigned char *inData, int width, int height, int totalBuckets);

private:
  int calculateChannelError(unsigned char *originalData, unsigned char *quantizedData, int width, int height);
  static vector<int> calculateBucketAverages(const vector<int> &hist, int totalBuckets);
  void calculateHistogram(unsigned char *inData, int width, int height, vector<int> &histogramR, vector<int> &histogramG, vector<int> &histogramB);
  void OnPaint(wxPaintEvent &event);
  wxImage inImage;
  wxScrolledWindow *scrolledWindow;
  int width;
  int height;
  int mode;
  int totalBuckets;
};

/** Utility function to read image data */
unsigned char *readImageData(string imagePath, int width, int height);

bool MyApp::OnInit()
{
  wxInitAllImageHandlers();

  // deal with command line arguments here
  cout << "Number of command line arguments: " << wxApp::argc << endl;
  if (wxApp::argc != 4)
  {
    cerr << "The executable should be invoked with exactly one filepath "
            "argument. Example ./MyImageApplication '../../Lena_512_512.rgb'"
         << endl;
    exit(1);
  }

  string imagePath = wxApp::argv[1].ToStdString();
  int mode = stoi(wxApp::argv[2].ToStdString());
  int totalBuckets = stoi(wxApp::argv[3].ToStdString());

  MyFrame *frame = new MyFrame("Image Display", imagePath, mode, totalBuckets);
  frame->Show(true);

  // return true to continue, false to exit the application
  return true;
}

/**
 * Constructor for the MyFrame class.
 * Here we read the pixel data from the file and set up the scrollable window.
 */
MyFrame::MyFrame(const wxString &title, string imagePath, int mode, int totalBuckets)
    : wxFrame(NULL, wxID_ANY, title)
{

  // Modify the height and width values here to read and display an image with
  // different dimensions.
  width = 512;
  height = 512;
  vector<int> totalErrorsUniform;
  vector<int> totalErrorsNonUniform;
  vector<int> indexes; // for indexing

  unsigned char *inData = readImageData(imagePath, width, height);

  if (mode == 1)
  {
    uniformQuantization(inData, width, height, totalBuckets);
    for (int tb = 2; tb <= 255; tb += 8)
    {
      indexes.push_back(tb);
      // Create a copy of the original data for quantization without modifying it
      unsigned char *quantizedData = new unsigned char[width * height * 3];
      copy(inData, inData + (width * height * 3), quantizedData);
      uniformQuantization(quantizedData, width, height, tb); //this should carry the quantizd value
      int uniformError = calculateChannelError(inData, quantizedData, width, height);
      totalErrorsUniform.push_back(uniformError);
      delete[] quantizedData;
    }

    for (int error : totalErrorsUniform)
    {
      cout << error << " ";
    }
    cout << endl;
  }
  else if (mode == 2)
  {
    nonUniformQuantization(inData, width, height, totalBuckets);
     for (int tb = 2; tb <= 255; tb += 8)
    {
      indexes.push_back(tb);
      unsigned char *quantizedDataNonUniform = new unsigned char[width * height * 3];
      copy(inData, inData + (width * height * 3), quantizedDataNonUniform);
      nonUniformQuantization(quantizedDataNonUniform, width, height, tb);
      int nonUniformError = calculateChannelError(inData, quantizedDataNonUniform, width, height);
      totalErrorsNonUniform.push_back(nonUniformError);
      delete[] quantizedDataNonUniform;
    }
    for (int error : totalErrorsNonUniform)
    {
      cout << error << " ";
    }
    cout << endl;
  }

  cout << "Indexes: ";
  for (int index : indexes)
  {
    cout << index << " ";
  }

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
}

// calculate function channel goes from 2 - 255 need to implement this
int calculateAbsoluteError(int original, int quantized)
{
  return abs(original - quantized);
}

// calcultates total errors for R,G,B
int MyFrame::calculateChannelError(unsigned char *originalData, unsigned char *quantizedData, int width, int height)
{
  int totalErrorR = 0, totalErrorG = 0, totalErrorB = 0;

  for (int i = 0; i < width * height * 3; i += 3)
  {
    totalErrorR += calculateAbsoluteError(originalData[i], quantizedData[i]);
    totalErrorG += calculateAbsoluteError(originalData[i + 1], quantizedData[i + 1]);
    totalErrorB += calculateAbsoluteError(originalData[i + 2], quantizedData[i + 2]);
  }
  return totalErrorR + totalErrorG + totalErrorB;
}

// mode 1
void MyFrame::uniformQuantization(unsigned char *inData, int width, int height, int totalBuckets)
{
  unsigned char *originalData = new unsigned char[width * height * 3];
  if (originalData)
  {
    copy(inData, inData + (width * height * 3), originalData);
    int bucketsPerChannel = cbrt(totalBuckets);
    int bucketRange = 256 / bucketsPerChannel;

    for (int i = 0; i < width * height * 3; i += 3)
    {
      for (int j = 0; j < 3; ++j)
      { 
        int bucketIndex = inData[i + j] / bucketRange;
        int newValue = bucketIndex * bucketRange + bucketRange / 2; // Mid-point of the bucket
        inData[i + j] = min(newValue, 255);
      }
    }
    // int TotalError = calculateChannelError(originalData, inData, width, height);
    // cout << "Total sum of absolute error for uniform quantization: " << TotalError << endl;
    delete[] originalData;
  }
}

void MyFrame::calculateHistogram(unsigned char *inData, int width, int height, vector<int> &histR, vector<int> &histG, vector<int> &histB)
{
  histR.assign(256, 0);
  histG.assign(256, 0);
  histB.assign(256, 0);

  for (int i = 0; i < width * height * 3; i += 3)
  {
    ++histR[inData[i]];
    ++histG[inData[i + 1]];
    ++histB[inData[i + 2]];
  }
}

//mode 2
void MyFrame::nonUniformQuantization(unsigned char *inData, int width, int height, int totalBuckets)
{
  unsigned char *originalData = new unsigned char[width * height * 3];
  if (originalData)
  {
    copy(inData, inData + (width * height * 3), originalData);

    vector<int> histR(256), histG(256), histB(256);
    calculateHistogram(inData, width, height, histR, histG, histB);

    vector<int> avgR = calculateBucketAverages(histR, totalBuckets);
    vector<int> avgG = calculateBucketAverages(histG, totalBuckets);
    vector<int> avgB = calculateBucketAverages(histB, totalBuckets);

    int size = width * height * 3;

    // cout << "Original and Quantized/Average values for each channel:" << endl;
    // for (int i = 0; i < totalBuckets; ++i)
    // {
    //   cout << "Bucket " << i << " - Original (R, G, B): (" << histR[i] << ", " << histG[i] << ", " << histB[i] << ")"
    //        << " - Quantized/Average (R, G, B): (" << avgR[i] << ", " << avgG[i] << ", " << avgB[i] << ")" << endl;
    // }

    int bucketsPerChannel = cbrt(totalBuckets);
    int pixelsPerBucket = 256 / bucketsPerChannel;

    for (int i = 0; i < size; i += 3) 
    {
      int bucketIndexR = inData[i] / pixelsPerBucket;
      int bucketIndexG = inData[i + 1] / pixelsPerBucket;
      int bucketIndexB = inData[i + 2] / pixelsPerBucket;

      inData[i] = avgR[bucketIndexR];
      inData[i + 1] = avgG[bucketIndexG];
      inData[i + 2] = avgB[bucketIndexB];
    }
    // int TotalError = calculateChannelError(originalData, inData, width, height);
    // cout << "Total sum of absolute error for non-uniform quantization: " << TotalError << endl;
    
    delete[] originalData;
  }
}

vector<int> MyFrame::calculateBucketAverages(const vector<int>& hist, int totalBuckets) {
  vector<int> averages(totalBuckets, 0);
  int bucketsPerChannel = cbrt(totalBuckets);
  int pixelsPerBucket = 256 / bucketsPerChannel;
  //int pixelsPerBucket = max(1, 256 / bucketsPerChannel);

  for (int i = 0; i < totalBuckets; i++)
  {
    long sum = 0;
    int count = 0;
    int startRange = i * pixelsPerBucket;
    int endRange = min((i + 1) * pixelsPerBucket, 256); // to ensure no overflow

    for (int j = startRange; j < endRange && j < 256; j++) {
      sum += hist[j] * j;
      count += hist[j];
    }
    averages[i] = count > 0 ? static_cast<int>(ceil(static_cast<double>(sum) / count)) : 0;

  }

  return averages;
}


/**
 * The OnPaint handler that paints the UI.
 * Here we paint the image pixels into the scrollable window.
 */
void MyFrame::OnPaint(wxPaintEvent &event)
{
  wxBufferedPaintDC dc(scrolledWindow);
  scrolledWindow->DoPrepareDC(dc);

  wxBitmap inImageBitmap = wxBitmap(inImage);
  dc.DrawBitmap(inImageBitmap, 0, 0, false);
}

/** Utility function to read image data */
unsigned char *readImageData(string imagePath, int width, int height)
{

  // Open the file in binary mode
  ifstream inputFile(imagePath, ios::binary);

  if (!inputFile.is_open())
  {
    cerr << "Error Opening File for Reading" << endl;
    exit(1);
  }

  // Create and populate RGB buffers
  vector<char> Rbuf(width * height);
  vector<char> Gbuf(width * height);
  vector<char> Bbuf(width * height);

  inputFile.read(Rbuf.data(), width * height);
  inputFile.read(Gbuf.data(), width * height);
  inputFile.read(Bbuf.data(), width * height);

  inputFile.close();

  unsigned char *inData =
      (unsigned char *)malloc(width * height * 3 * sizeof(unsigned char));

  for (int i = 0; i < height * width; i++)
  {
    // We populate RGB values of each pixel in that order
    // RGB.RGB.RGB and so on for all pixels
    inData[3 * i] = Rbuf[i];
    inData[3 * i + 1] = Gbuf[i];
    inData[3 * i + 2] = Bbuf[i];
  }

  return inData;
}

wxIMPLEMENT_APP(MyApp);

// mode 2 - average, analyze: maintain count of array = freq of diff values then divide them invidiually into ranges, average of the pixels within the range [quant]
// currently can find total error for that exact totalBuckets but need to analyze the differences as the total buckets gets bigger 
// thus need 2 - 255 loop and run the quantization 