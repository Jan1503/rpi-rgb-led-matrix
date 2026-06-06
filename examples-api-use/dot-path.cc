// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// One-pixel motion pattern for spotting frame merge or row-order issues.
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"

#include <algorithm>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

using rgb_matrix::Color;
using rgb_matrix::FrameCanvas;
using rgb_matrix::RGBMatrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

enum UpdateMode {
  kSwapOnVSync,
  kDirect,
};

enum PathStyle {
  kPathAll,
  kPathVertical,
  kPathHorizontal,
  kPathDiagonal,
};

static int usage(const char *progname) {
  fprintf(stderr, "usage: %s [options]\n", progname);
  fprintf(stderr,
          "Shows one moving pixel: vertical, horizontal, then diagonal.\n"
          "The pixel can be drawn through SwapOnVSync or directly to the live canvas.\n\n");
  fprintf(stderr, "Options:\n\n");
  fprintf(stderr,
          "\t-f <frames>               : Hold each dot position for this many updates. Default: 1\n"
          "\t-m <swap|direct>          : Update mode. Default: swap\n"
          "\t-p <path>                 : Path to show: all, vertical, horizontal, diagonal. Default: all\n"
          "\t-s <position>             : Start position along the path, wrapping around. Default: 0\n"
          "\t-u <usec>                 : Direct-mode delay after each update. Default: 16667\n"
          "\t-x <column>               : Column for the vertical path. Default: center\n"
          "\t-y <row>                  : Row for the horizontal path. Default: center\n\n");
  rgb_matrix::PrintMatrixFlags(stderr);
  return 1;
}

static bool ParseUpdateMode(const char *str, UpdateMode *mode) {
  if (strcmp(str, "swap") == 0 || strcmp(str, "vsync") == 0) {
    *mode = kSwapOnVSync;
    return true;
  }
  if (strcmp(str, "direct") == 0 || strcmp(str, "live") == 0) {
    *mode = kDirect;
    return true;
  }
  return false;
}

static bool ParsePathStyle(const char *str, PathStyle *style) {
  if (strcmp(str, "all") == 0) {
    *style = kPathAll;
    return true;
  }
  if (strcmp(str, "vertical") == 0 || strcmp(str, "v") == 0) {
    *style = kPathVertical;
    return true;
  }
  if (strcmp(str, "horizontal") == 0 || strcmp(str, "h") == 0) {
    *style = kPathHorizontal;
    return true;
  }
  if (strcmp(str, "diagonal") == 0 || strcmp(str, "d") == 0) {
    *style = kPathDiagonal;
    return true;
  }
  return false;
}

static int WrapPosition(int position, int limit) {
  if (limit <= 0)
    return 0;

  const int wrapped = position % limit;
  return wrapped < 0 ? wrapped + limit : wrapped;
}

static void ShowDot(RGBMatrix *matrix, FrameCanvas **canvas, UpdateMode mode,
                    int x, int y, const Color &color, int hold_frames,
                    int direct_delay_us) {
  for (int frame = 0; frame < hold_frames && !interrupt_received; ++frame) {
    if (mode == kSwapOnVSync) {
      (*canvas)->Clear();
      (*canvas)->SetPixel(x, y, color.r, color.g, color.b);
      *canvas = matrix->SwapOnVSync(*canvas);
    } else {
      matrix->Clear();
      matrix->SetPixel(x, y, color.r, color.g, color.b);
      if (direct_delay_us > 0)
        usleep(direct_delay_us);
    }
  }
}

static void ShowVerticalPath(RGBMatrix *matrix, FrameCanvas **canvas,
                             UpdateMode update_mode, int width, int height,
                             int start_position, int column,
                             const Color &color, int hold_frames,
                             int direct_delay_us) {
  const int start_y = WrapPosition(start_position, height);
  for (int step = 0; step < height && !interrupt_received; ++step) {
    const int y = (start_y + step) % height;
    ShowDot(matrix, canvas, update_mode, column, y, color, hold_frames,
            direct_delay_us);
  }
}

static void ShowHorizontalPath(RGBMatrix *matrix, FrameCanvas **canvas,
                               UpdateMode update_mode, int width, int height,
                               int start_position, int row,
                               const Color &color, int hold_frames,
                               int direct_delay_us) {
  const int start_x = WrapPosition(start_position, width);
  for (int step = 0; step < width && !interrupt_received; ++step) {
    const int x = (start_x + step) % width;
    ShowDot(matrix, canvas, update_mode, x, row, color, hold_frames,
            direct_delay_us);
  }
}

static void ShowDiagonalPath(RGBMatrix *matrix, FrameCanvas **canvas,
                             UpdateMode update_mode, int width, int height,
                             int start_position, const Color &color,
                             int hold_frames, int direct_delay_us) {
  const int start_x = WrapPosition(start_position, width);
  for (int step = 0; step < width && !interrupt_received; ++step) {
    const int x = (start_x + step) % width;
    const int y = width > 1 ? x * (height - 1) / (width - 1) : 0;
    ShowDot(matrix, canvas, update_mode, x, y, color, hold_frames,
            direct_delay_us);
  }
}

int main(int argc, char *argv[]) {
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                         &matrix_options, &runtime_opt)) {
    return usage(argv[0]);
  }

  int hold_frames = 1;
  int direct_delay_us = 16667;
  int start_position = 0;
  int vertical_column = -1;
  int horizontal_row = -1;
  UpdateMode update_mode = kSwapOnVSync;
  PathStyle path_style = kPathAll;
  int opt;
  while ((opt = getopt(argc, argv, "f:m:p:s:u:x:y:")) != -1) {
    switch (opt) {
    case 'f':
      hold_frames = std::max(1, atoi(optarg));
      break;
    case 'm':
      if (!ParseUpdateMode(optarg, &update_mode)) {
        fprintf(stderr, "Invalid update mode: %s\n", optarg);
        return usage(argv[0]);
      }
      break;
    case 'p':
      if (!ParsePathStyle(optarg, &path_style)) {
        fprintf(stderr, "Invalid path: %s\n", optarg);
        return usage(argv[0]);
      }
      break;
    case 's':
      start_position = atoi(optarg);
      break;
    case 'u':
      direct_delay_us = std::max(0, atoi(optarg));
      break;
    case 'x':
      vertical_column = atoi(optarg);
      break;
    case 'y':
      horizontal_row = atoi(optarg);
      break;
    default:
      return usage(argv[0]);
    }
  }

  RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
  if (matrix == NULL)
    return usage(argv[0]);

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  FrameCanvas *canvas = update_mode == kSwapOnVSync
      ? matrix->CreateFrameCanvas() : NULL;
  const int width = matrix->width();
  const int height = matrix->height();
  const int selected_vertical_column = vertical_column < 0
      ? width / 2 : WrapPosition(vertical_column, width);
  const int selected_horizontal_row = horizontal_row < 0
      ? height / 2 : WrapPosition(horizontal_row, height);
  const Color vertical_color(255, 0, 0);
  const Color horizontal_color(0, 255, 0);
  const Color diagonal_color(0, 64, 255);

  while (!interrupt_received) {
    if (path_style == kPathAll || path_style == kPathVertical)
      ShowVerticalPath(matrix, &canvas, update_mode, width, height,
                       start_position, selected_vertical_column,
                       vertical_color, hold_frames, direct_delay_us);

    if (path_style == kPathAll || path_style == kPathHorizontal)
      ShowHorizontalPath(matrix, &canvas, update_mode, width, height,
                         start_position, selected_horizontal_row,
                         horizontal_color, hold_frames, direct_delay_us);

    if (path_style == kPathAll || path_style == kPathDiagonal)
      ShowDiagonalPath(matrix, &canvas, update_mode, width, height,
                       start_position, diagonal_color, hold_frames,
                       direct_delay_us);
  }

  delete matrix;
  return 0;
}
