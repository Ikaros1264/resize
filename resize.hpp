#ifndef RESIZE_H_
#define RESIZE_H_

#include "utils.hpp"
#include <cmath>
#include <thread>

inline float WeightCoeff(float x, float a) {
  if (x <= 1) {
    float x_2 = x*x;
    return 1 - (a + 3) * x_2 + (a + 2) * x * x_2;
  } else if (x < 2) {
    float x_2 = x*x;
    float a4 = 4 * a;
    return -1 * a4 + 2 * a4 * x - 5 * a * x_2 + a * x * x_2;
  }
  return 0.0;
}

static void CalcCoeff4x4(float x, float y, float *coeff) {
  const float a = -0.5f;

  float u = x - floor(x);
  float v = y - floor(y);

  u += 1;
  v += 1;

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      coeff[i * 4 + j] =
          WeightCoeff(fabs(u - i), a) * WeightCoeff(fabs(v - j), a);
    }
  }
}

static unsigned char BGRAfterBiCubic(RGBImage src, float x_float, float y_float, int channels, int d, float *coeff) {

  int x0 = floor(x_float) - 1;
  int y0 = floor(y_float) - 1;

  float sum = .0f;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      sum += coeff[i * 4 + j] * src.data[((x0 + i) * src.cols + y0 + j) * channels + d];
    }
  }
  return static_cast<unsigned char>(sum);
}


//void ResizeImageThread(RGBImage src, int i, int j, int src_x, int src_y, int channels, int thischannel, unsigned char *res, float *coeff){
//  int resize_cols = src.cols * 5.f;
// res[((i * resize_cols) + j) * channels + thischannel] = BGRAfterBiCubic(src, src_x, src_y, channels, thischannel, coeff);
//  return;
//}

void ResizeImagePart(RGBImage src, float ratio, int x_left, int x_right, int y_up, int y_down, int channels, unsigned char *res) {
//  static const int resize_rows = src.rows * ratio;
//  static const int resize_cols = src.cols * ratio;
  const int resize_cols = src.cols * ratio;
  auto check_perimeter = [src](float x, float y) -> bool {
    return x < src.rows - 2 && x > 1 && y < src.cols - 2 && y > 1;
  };
//  Timer part("part");
  for (register int i = x_left; i < x_right; i++) {
    for (register int j = y_up; j < y_down; j++) {
      float src_x = i / ratio;
      float src_y = j / ratio;
      if (check_perimeter(src_x, src_y)) {
        float coeff[16];
        CalcCoeff4x4(src_x, src_y, coeff);
        /*std::thread worker0(ResizeImageThread, src, i, j, src_x, src_y, channels, 0, res, coeff);
        std::thread worker1(ResizeImageThread, src, i, j, src_x, src_y, channels, 1, res, coeff);
        std::thread worker2(ResizeImageThread, src, i, j, src_x, src_y, channels, 2, res, coeff);
        worker0.join();
        worker1.join();
        worker2.join();*/
        res[((i * resize_cols) + j) * channels + 0] = BGRAfterBiCubic(src, src_x, src_y, channels, 0, coeff);
        res[((i * resize_cols) + j) * channels + 1] = BGRAfterBiCubic(src, src_x, src_y, channels, 1, coeff);
        res[((i * resize_cols) + j) * channels + 2] = BGRAfterBiCubic(src, src_x, src_y, channels, 2, coeff);
      }
    }
  }
  return ;
}

RGBImage ResizeImage(RGBImage src, float ratio) {
  register const int channels = src.channels;
  Timer timer("resize image by 5x");
  register const int resize_rows = src.rows * ratio;
  register const int resize_cols = src.cols * ratio;

  printf("resize to: %d x %d\n", resize_rows, resize_cols);

  auto res = new unsigned char[channels * resize_rows * resize_cols];
  std::fill(res, res + channels * resize_rows * resize_cols, 0);

  register const int n=4;
  std::thread PartThread[n][n];

  for(int x = 0 ; x < n ; x++)
    for(int y = 0 ; y < n ; y++){
      PartThread[x][y] = std::thread(ResizeImagePart, src, ratio, (int)((resize_rows/n)*x), (int)((resize_rows/n)*(x+1)), (int)((resize_cols/n)*y), (int)((resize_cols/n)*(y+1)), channels, res);
    }
  for(int x = 0 ; x < n ; x++)
    for(int y = 0 ; y < n ; y++){
      PartThread[x][y].join();
    }

  return RGBImage{resize_cols, resize_rows, channels, res};
}

#endif