////////////////////////////////////////////////////////////////////////
//
// plane_tiler.cpp
//
// Splits a plane into tiles.
//
////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>

using namespace cv;
using namespace std;

typedef Rect PlaneSegment;

class Tile {

  public:
    int id;
    Point tl, tr, br, bl;
    Scalar color;
    static const double EPSILON = 0.001;

    Tile(int id, Point tl, Point tr, Point br, Point bl, Scalar color) {
      this->id = id;
      this->tl = tl; this->tr = tr; this->br = br; this->bl = bl;
      this->color = color;
    }

    Rect getRect() {
      return Rect(tl.x, tl.y, tr.x - tl.x, bl.y - tl.y);
    }

    bool operator<(const Tile& other) const {
      if (tl.y == other.tl.y) { return tl.x < other.tl.x; }
      return tl.y < other.tl.y;
    }

};

const Scalar POINT_COLOR(0, 0, 255);
const Scalar PLANE_COLOR(200, 255, 200);
const Scalar BACKGROUND_COLOR(255, 255, 255);
const int TILE_WIDTH = 100; // pixels
const int TILE_HEIGHT = TILE_WIDTH; // pixels
const int CANVAS_WIDTH = 1200;
const int CANVAS_HEIGHT = 800;
const int EXPAND_WIDTH = 30;
const int EXPAND_HEIGHT = 20;
const int NUM_PLANE_SEGMENTS = 1;

Mat canvas(CANVAS_HEIGHT, CANVAS_WIDTH, CV_8UC3);
PlaneSegment planes[NUM_PLANE_SEGMENTS] = {
  //PlaneSegment(400, 200, 100, 100),
  //PlaneSegment(725, 200, 100, 100)
  PlaneSegment(CANVAS_WIDTH/2-50, CANVAS_HEIGHT/2-50, 100, 100)
};
vector<Tile> tiles;
int tile_id = 0;

Scalar getRandomColor(int low = 100, int high = 200) {
  return Scalar(low + (float)rand()/((float)RAND_MAX/(high-low)),
                low + (float)rand()/((float)RAND_MAX/(high-low)),
                low + (float)rand()/((float)RAND_MAX/(high-low)));
}

void drawPlaneSegment(PlaneSegment& plane) {
  rectangle(canvas, plane, PLANE_COLOR, -1);
}

void drawPlaneSegments() {
  for (int ii = 0; ii < NUM_PLANE_SEGMENTS; ++ii) {
    drawPlaneSegment(planes[ii]);
  }
}

int findClosestTileToPlaneSegment(int x, int y) {
  double closest_distance = 99999999;
  int closest_tile_idx = -1;
  for (int ii = 0; ii < tiles.size(); ++ii) {
    double distance = pow(tiles[ii].tl.x - x, 2.0) + pow(tiles[ii].tl.y - y, 2.0);
    if (distance < closest_distance && tiles[ii].tl.x >= x && tiles[ii].tl.y >= y) {
      closest_distance = distance;
      closest_tile_idx = ii;
    }
  }
  return closest_tile_idx;
}

void drawTiles() {
  sort(tiles.begin(), tiles.end());
  for (int ii = 0; ii < tiles.size(); ++ii) {
    Rect tile_rect = tiles[ii].getRect();
    rectangle(canvas, tile_rect, tiles[ii].color, 3);
    stringstream ss;
    ss << tiles[ii].id << "/" << ii;
    const char* tile_id = ss.str().c_str();
    Size text_size = getTextSize(tile_id, FONT_HERSHEY_COMPLEX_SMALL, 0.8, 1, NULL);
    Point text_location(tile_rect.x + tile_rect.width/2 - text_size.width/2, tile_rect.y + tile_rect.height/2 + text_size.height/2);
    putText(canvas, tile_id, text_location, FONT_HERSHEY_COMPLEX_SMALL, 0.8, tiles[ii].color, 1, CV_AA);
  }
}

bool tileExists(Tile tile) {
  for (int ii = 0; ii < tiles.size(); ii++) {
    if (abs(tiles[ii].tl.x - tile.tl.x) < TILE_WIDTH/2 && abs(tiles[ii].tl.y - tile.tl.y) < TILE_HEIGHT/2) {
      return true;
    }
  }
  return false;
}

void generateTilesForPlaneSegment(PlaneSegment& plane) {
  int closest_tile_idx = findClosestTileToPlaneSegment(plane.x, plane.y);
  int start_x = plane.x;
  int start_y = plane.y;
  if (closest_tile_idx != -1) {
    Tile closest_tile = tiles[closest_tile_idx];
    start_x = closest_tile.tl.x;
    start_y = closest_tile.tl.y;
    while (start_x >= plane.x + TILE_WIDTH) { start_x -= TILE_WIDTH; }
    while (start_y >= plane.y + TILE_HEIGHT) { start_y -= TILE_HEIGHT; }
  }
  for (int x = start_x; x < start_x + plane.width - TILE_WIDTH; x += TILE_WIDTH) {
    for (int y = start_y; y < start_y + plane.height - TILE_HEIGHT; y += TILE_HEIGHT) {
      Point tl(x, y), tr(x + TILE_WIDTH, y), br(x + TILE_WIDTH, y + TILE_HEIGHT), bl(x, y + TILE_HEIGHT);
      Tile tile(tile_id++, tl, tr, br, bl, getRandomColor());
      if (!tileExists(tile)) {
        tiles.push_back(tile);
      }
    }
  }
}

void generateTilesForPlaneSegments() {
  for (int ii = 0; ii < NUM_PLANE_SEGMENTS; ++ii) {
    generateTilesForPlaneSegment(planes[ii]);
  }
}

void expandPlaneSegment(PlaneSegment& plane) {
  plane.x -= EXPAND_WIDTH/2;
  plane.width += EXPAND_WIDTH;
  plane.y -= EXPAND_HEIGHT/2;
  plane.height += EXPAND_HEIGHT;
}

void expandPlaneSegments() {
  for (int ii = 0; ii < NUM_PLANE_SEGMENTS; ++ii) {
    expandPlaneSegment(planes[ii]);
  }
}

void clearCanvas() {
  rectangle(canvas, Point(0, 0), Point(canvas.cols, canvas.rows), BACKGROUND_COLOR, -1);
}

void redraw() {
  canvas = Mat(CANVAS_HEIGHT, CANVAS_WIDTH, CV_8UC3);
  clearCanvas();
  drawPlaneSegments();
  drawTiles();
  imshow("Plane", canvas);
}

void onMouse(int event, int x, int y, int flags, void* param) {
  switch (event) {
  case CV_EVENT_LBUTTONDOWN:
    generateTilesForPlaneSegments();
    redraw();
    break;
  case CV_EVENT_RBUTTONDOWN:
    expandPlaneSegments();
    redraw();
    break;
  }
}

int main(int argc, char *argv[]) {
  // show the image
  clearCanvas();
  redraw();
  namedWindow("Plane", CV_WINDOW_AUTOSIZE);
  moveWindow("Plane", 0, 0);
  imshow("Plane", canvas);

  // set the mouse callback for adding vertices
  setMouseCallback("Plane", onMouse, NULL);
  waitKey(0);

  return 0;
}
