/**
 * GUK IL KIM
 * 3020-8670-72
 * CSCI 576 HW3 
 * 3/30/24
*/

#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#define pi 3.142857

using namespace std;
namespace fs = std::filesystem;

class MyApp : public wxApp {
 public:
  bool OnInit() override;
};

struct Block8x8 {
    float r[8][8];
    float g[8][8];
    float b[8][8];
};

struct MDCTBlock {
    float r[8][8];
    float g[8][8];
    float b[8][8];
};

struct IDCTBlock {
    float r[8][8];
    float g[8][8];
    float b[8][8];
};

class MyFrame : public wxFrame {
 public:
    MyFrame(const wxString &title, string imagePath, int quantizationLevel, int deliveryMode, int latency);

  private:
  void OnPaint(wxPaintEvent &event);
  void OnTimer(wxTimerEvent &event);
  MDCTBlock MDCT[64][64];
  IDCTBlock IDCT[64][64];
  wxImage inImage, decodedImage;
  wxScrolledWindow *scrolledWindow;
  wxTimer *v_timer;
  int width;
  int height;
  int quantizationLevel;
  int deliveryMode;
  int latency;
  string imagePath; //added after
};

unsigned char *readImageData(string imagePath, int width, int height);
void ApplyDCT(Block8x8 mBlocks[64][64], MDCTBlock MDCT[64][64]); //result stored in MDCT 
void Quantization(MDCTBlock MDCT[64][64], int quantizationLevel);

bool MyApp::OnInit() {
  wxInitAllImageHandlers();

  cout << "Number of command line arguments: " << wxApp::argc << endl;
  if (wxApp::argc != 5) {
    cerr << "The executable should be invoked with exactly four arguments: "
    "InputImage QuantizationLevel DeliveryMode Latency"
    << endl;
    exit(1);
  }
  string imagePath = wxApp::argv[1].ToStdString();
  int quantizationLevel = std::stoi(wxApp::argv[2].ToStdString());
  int deliveryMode = std::stoi(wxApp::argv[3].ToStdString());
  int latency = std::stoi(wxApp::argv[4].ToStdString());
  cout << "Quantization Level: " << quantizationLevel << endl;
  cout << "Delivery Mode: " << deliveryMode << endl;
  cout << "Latency: " << latency << endl;

  MyFrame *frame = new MyFrame("Image Display", imagePath, quantizationLevel, deliveryMode, latency);
  frame->Show(true);

  return true;
}

void Print(const MDCTBlock& block) {
  std::cout << "output Block:" << std::endl;
  for (int u = 0; u < 8; ++u) {
    for (int v = 0; v < 8; ++v) {
      std::cout << "(" << block.r[u][v] << ", " << block.g[u][v] << ", " << block.b[u][v] << ") ";
    }
    std::cout << std::endl;
  }
}

void Print(const IDCTBlock& block) {
  std::cout << "output Block:" << std::endl;
  for (int u = 0; u < 8; ++u) {
    for (int v = 0; v < 8; ++v) {
      std::cout << "(" << block.r[u][v] << ", " << block.g[u][v] << ", " << block.b[u][v] << ") ";
    }
    std::cout << std::endl;
  }
}

void Compress(Block8x8 Blocks[64][64], MDCTBlock MDCT[64][64], int quantizationLevel) {
  ApplyDCT(Blocks, MDCT);
  Quantization(MDCT, quantizationLevel);
}

void Dequantization(float singleBlock[8][8][3], int quantizationLevel) {
  int N = quantizationLevel; 
  for (int u = 0; u < 8; ++u) {
    for (int v = 0; v < 8; ++v) {
      singleBlock[u][v][0] *= pow(2, N);
      singleBlock[u][v][1] *= pow(2, N);
      singleBlock[u][v][2] *= pow(2, N);
    }
  }
}

void ApplyIDCT(float singleBlock[8][8][3], IDCTBlock& singleIDCTBlock) {
  const int N = 8;
  const float PI = acos(-1.0f);
  for (int x = 0; x < N; x++) {
    for (int y = 0; y < N; y++) {
      float sumR = 0, sumG = 0, sumB = 0;
      for (int u = 0; u < N; u++) {
        for (int v = 0; v < N; v++) {
          float c_u = (u == 0) ? 1 / sqrt(2) : 1;
          float c_v = (v == 0) ? 1 / sqrt(2) : 1;
          float cos_u = cos((2 * x + 1) * u * PI / 16);
          float cos_v = cos((2 * y + 1) * v * PI / 16);
          sumR += c_u * c_v * singleBlock[u][v][0] * cos_u * cos_v;
          sumG += c_u * c_v * singleBlock[u][v][1] * cos_u * cos_v;
          sumB += c_u * c_v * singleBlock[u][v][2] * cos_u * cos_v;
        }
      }
    singleIDCTBlock.r[x][y] = max(0.0f, min(sumR / 4.0f, 255.0f));
    singleIDCTBlock.g[x][y] = max(0.0f, min(sumG / 4.0f, 255.0f));
    singleIDCTBlock.b[x][y] = max(0.0f, min(sumB / 4.0f, 255.0f));
    }
  }
}

MyFrame::MyFrame(const wxString &title, string imagePath, int quantizationLevel, int deliveryMode, int latency)
    : wxFrame(NULL, wxID_ANY, title), quantizationLevel(quantizationLevel), deliveryMode(deliveryMode), latency(latency), imagePath(imagePath){
  width = 512;
  height = 512;
  bool encoded = false;
  unsigned char *inData = readImageData(imagePath, width, height);
  unsigned char* decodedData = new unsigned char[width * height * 3](); //for decoded image
  inImage.SetData(inData, width, height, false);
  decodedImage.SetData(decodedData, width, height, true); //need to figure out where this goes
  Block8x8 Blocks[64][64]; //original data stored in blocks
  for (int i = 0; i < height; i += 8) {
    for (int j = 0; j < width; j += 8) {
      for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
          int idx = ((i + x) * width + (j + y)) * 3;
          Blocks[i / 8][j / 8].r[x][y] = inData[idx];
          Blocks[i / 8][j / 8].g[x][y] = inData[idx + 1];
          Blocks[i / 8][j / 8].b[x][y] = inData[idx + 2];
          }
        }
      }
    }
  if(!encoded) {  
    Compress(Blocks, MDCT, quantizationLevel); //values in MDCT 
    encoded = true;
  }

  scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
  scrolledWindow->SetScrollbars(10, 10, width, height);
  scrolledWindow->SetVirtualSize(width, height);

  // Bind the paint event to the OnPaint function of the scrolled window
  scrolledWindow->Bind(wxEVT_PAINT, &MyFrame::OnPaint, this);

  // Set the frame size
  SetClientSize(width * 2, height); //due to decoded image

  // Set the frame background color
  SetBackgroundColour(*wxBLACK); //decoded image will be black
  v_timer = new wxTimer(this, wxID_ANY);
  Bind(wxEVT_TIMER, &MyFrame::OnTimer, this);
  if (latency == 0) { //when latency is 0, set it to 1
    latency = 1;
  }
  v_timer->Start(latency);   
}

void GetSingleBlock(const MDCTBlock MDCT[64][64], int blockX, int blockY, float singleBlock[8][8][3]) {
  for (int u = 0; u < 8; ++u) {
    for (int v = 0; v < 8; ++v) {
      singleBlock[u][v][0] = MDCT[blockX][blockY].r[u][v];
      singleBlock[u][v][1] = MDCT[blockX][blockY].g[u][v];
      singleBlock[u][v][2] = MDCT[blockX][blockY].b[u][v];
    }
  }
}

void MyFrame::OnTimer(wxTimerEvent& event) {
  static unsigned char* decodedData = new unsigned char[width * height * 3]; 
    static int currentBlockIndex = 0;
    static int totalBlocks = 64 * 64; // Total number of blocks
    static bool encoded = false;

   if (deliveryMode == 1 && currentBlockIndex < totalBlocks) {
    int blockX = currentBlockIndex % 64;
    int blockY = currentBlockIndex / 64;
    float singleBlock[8][8][3];

    GetSingleBlock(MDCT, blockY, blockX, singleBlock);
    Dequantization(singleBlock, quantizationLevel);
    IDCTBlock singleIDCTBlock;
    ApplyIDCT(singleBlock, singleIDCTBlock);

    for (int x = 0; x < 8; ++x) {
      for (int y = 0; y < 8; ++y) {
        int index = ((blockY * 8) + y) * width + (blockX * 8) + x; 
        decodedData[index * 3] = singleIDCTBlock.r[y][x];
        decodedData[index * 3 + 1] = singleIDCTBlock.g[y][x];
        decodedData[index * 3 + 2] = singleIDCTBlock.b[y][x];
      }
    }
    currentBlockIndex++;
  } else if (deliveryMode == 2) { //zig-zag 

  } 
  if (currentBlockIndex >= totalBlocks && currentBlockIndex < totalBlocks) {
      v_timer->Stop();
    } else {
      decodedImage.SetData(decodedData, width, height, true);
      Refresh();
    }
}


void MyFrame::OnPaint(wxPaintEvent &event) {
  wxBufferedPaintDC dc(scrolledWindow);
  scrolledWindow->DoPrepareDC(dc);
  wxBitmap inImageBitmap = wxBitmap(inImage);
  dc.DrawBitmap(inImageBitmap, 0, 0, false);
  wxBitmap decodedImageBitmap = wxBitmap(decodedImage);
  dc.DrawBitmap(decodedImageBitmap, width, 0, false);
}

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

  inputFile.read(Rbuf.data(), width * height);
  inputFile.read(Gbuf.data(), width * height);
  inputFile.read(Bbuf.data(), width * height);

  inputFile.close();

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

void ApplyDCT(Block8x8 Blocks[64][64], MDCTBlock MDCT[64][64]) {
  const int N = 8;
  const float PI = acos(-1.0f);
  for (int blockX = 0; blockX < 64; blockX++) {
    for (int blockY = 0; blockY < 64; blockY++) {
      for (int u = 0; u < 8; u++) {
        for (int v = 0; v < 8; v++) {
          float sumR = 0, sumG = 0, sumB = 0;
          float c_u = ((u == 0) ? 1 / sqrt(2) : 1);
          float c_v = ((u == 0) ? 1 / sqrt(2) : 1);
          for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
              float cos_u = cos((2 * x + 1) * u * PI / 16);
              float cos_v = cos((2 * y + 1) * v * PI / 16);
              sumR += Blocks[blockX][blockY].r[x][y] * cos_u * cos_v;
              sumG += Blocks[blockX][blockY].g[x][y] * cos_u * cos_v;
              sumB += Blocks[blockX][blockY].b[x][y] * cos_u * cos_v;
            }
          }
          MDCT[blockX][blockY].r[u][v] = sumR * c_u * c_v / 4.0f;
          MDCT[blockX][blockY].g[u][v] = sumG * c_u * c_v / 4.0f;
          MDCT[blockX][blockY].b[u][v] = sumB * c_u * c_v / 4.0f;
          }
        }
      }
  }
}

void Quantization(MDCTBlock MDCT[64][64], int quantizationLevel) {
  int N = quantizationLevel;
  for (int blockX = 0; blockX < 64; ++blockX) {
    for (int blockY = 0; blockY < 64; ++blockY) {
      for (int u = 0; u < 8; ++u) {
        for (int v = 0; v < 8; ++v) {
          MDCT[blockX][blockY].r[u][v] = round(MDCT[blockX][blockY].r[u][v] / pow(2, N));
          MDCT[blockX][blockY].g[u][v] = round(MDCT[blockX][blockY].g[u][v] / pow(2, N));
          MDCT[blockX][blockY].b[u][v] = round(MDCT[blockX][blockY].b[u][v] / pow(2, N));
        }
      }
    }
  }
}

wxIMPLEMENT_APP(MyApp);
