#ifndef RESIZE_H_
#define RESIZE_H_

#include "utils.hpp"
#include <cmath>
#include <thread>

float WeightCoeff(float x, float a) {
  if (x <= 1) {
    return 1 - (a + 3) * x * x + (a + 2) * x * x * x;
  } else if (x < 2) {
    return -4 * a + 8 * a * x - 5 * a * x * x + a * x * x * x;
  }
  return 0.0;
}

void CalcCoeff4x4(float x, float y, float *coeff) {
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

unsigned char BGRAfterBiCubic(RGBImage src, float x_float, float y_float,
                              int channels, int d) {
  float coeff[16];

  int x0 = floor(x_float) - 1;
  int y0 = floor(y_float) - 1;
  CalcCoeff4x4(x_float, y_float, coeff);

  float sum = .0f;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      sum += coeff[i * 4 + j] *
             src.data[((x0 + i) * src.cols + y0 + j) * channels + d];
    }
  }
  return static_cast<unsigned char>(sum);
}

void ResizeImageThread(RGBImage src, float ratio, int x_left, int x_right, int y_up, int y_down, int channels, int thischannel, unsigned char *res) {
  int resize_rows = src.rows * ratio;
  int resize_cols = src.cols * ratio;
  auto check_perimeter = [src](float x, float y) -> bool {
    return x < src.rows - 2 && x > 1 && y < src.cols - 2 && y > 1;
  };

  for (register int i = x_left; i < x_right; i++) {
    for (register int j = y_up; j < y_down; j++) {
      float src_x = i / ratio;
      float src_y = j / ratio;
      if (check_perimeter(src_x, src_y)) {
          res[((i * resize_cols) + j) * channels + thischannel] =
              BGRAfterBiCubic(src, src_x, src_y, channels, thischannel);
      }
    }
  }
  return ;
}

void ResizeImagePart(RGBImage src, float ratio, int x_left, int x_right, int y_up, int y_down, int channels, unsigned char *res) {
  std::thread t0(ResizeImageThread, src, ratio, x_left, x_right, y_up, y_down, channels, 0, res);
  std::thread t1(ResizeImageThread, src, ratio, x_left, x_right, y_up, y_down, channels, 1, res);
  std::thread t2(ResizeImageThread, src, ratio, x_left, x_right, y_up, y_down, channels, 2, res);
  t0.join();
  t1.join();
  t2.join();
  return ;
}

RGBImage ResizeImage(RGBImage src, float ratio) {
  register const int channels = src.channels;
  Timer timer("resize image by 5x");
  register int resize_rows = src.rows * ratio;
  register int resize_cols = src.cols * ratio;

  printf("resize to: %d x %d\n", resize_rows, resize_cols);

  auto res = new unsigned char[channels * resize_rows * resize_cols];
  std::fill(res, res + channels * resize_rows * resize_cols, 0);

  register const int n=3;
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