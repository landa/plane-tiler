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
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

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
const int TILE_WIDTH = 20; // pixels
const int TILE_HEIGHT = TILE_WIDTH; // pixels
const int CANVAS_WIDTH = 1200;
const int CANVAS_HEIGHT = 800;
const int EXPAND_WIDTH = 30;
const int EXPAND_HEIGHT = 20;
const int NUM_PLANE_SEGMENTS = 1;

Mat canvas(CANVAS_HEIGHT, CANVAS_WIDTH, CV_8UC3);
vector<vector<Point> > planes;
vector<Tile> tiles;
int tile_id = 0;

Scalar getRandomColor(int low = 100, int high = 200) {
  return Scalar(low + (float)rand()/((float)RAND_MAX/(high-low)),
                low + (float)rand()/((float)RAND_MAX/(high-low)),
                low + (float)rand()/((float)RAND_MAX/(high-low)));
}

void drawPlaneSegments() {
  drawContours(canvas, planes, -1, PLANE_COLOR, CV_FILLED);
}

void drawTiles() {
  sort(tiles.begin(), tiles.end());
  for (int ii = 0; ii < tiles.size(); ++ii) {
    Rect tile_rect = tiles[ii].getRect();
    rectangle(canvas, tile_rect, tiles[ii].color, 1);
    stringstream ss;
    ss << tiles[ii].id << "/" << ii;
    const char* tile_id = ss.str().c_str();
    Size text_size = getTextSize(tile_id, FONT_HERSHEY_COMPLEX_SMALL, 0.8, 1, NULL);
    Point text_location(tile_rect.x + tile_rect.width/2 - text_size.width/2, tile_rect.y + tile_rect.height/2 + text_size.height/2);
    // putText(canvas, tile_id, text_location, FONT_HERSHEY_COMPLEX_SMALL, 0.8, tiles[ii].color, 1, CV_AA);
  }
}

bool tileExists(Tile tile) {
  for (int ii = 0; ii < tiles.size(); ii++) {
    if (abs(tiles[ii].tl.x - tile.tl.x) < TILE_WIDTH/4 && abs(tiles[ii].tl.y - tile.tl.y) < TILE_HEIGHT/4) {
      return true;
    }
  }
  return false;
}

void generateTilesForPlaneSegment(vector<Point> plane) {
  // 1. Get the bounding box of the plane segment
  Rect bbox = boundingRect(plane);

  // 2. Adjust the bounding box to the existing tiles
  if (tiles.size() > 0) {
    //    -> Find a tile that's inside the plane segment
    Tile* inner_tile = &tiles[0]; // for now this is just the 0th tile
    //    -> Move up from it until you're outside of the plane segment, and expand the BB up by the overflow
    int h = inner_tile->tl.y;
    while (h > bbox.y) { h -= TILE_HEIGHT; }
    int overflow_y = bbox.y - h;
    //    -> Move left from it until you're outside of the plane segment, and expand the BB left by the overflow
    int w = inner_tile->tl.x;
    while (w > bbox.x) { w -= TILE_WIDTH; }
    int overflow_x = bbox.x - w;
    // expand the bb
    bbox.x -= overflow_x;
    bbox.y -= overflow_y;
  }
  // TODO: rotate the contour to align with the existing tiles

  // 3. Overlay a grid onto the plane segment starting from the top left corner
  //    -> Add a tile for each grid square
  for (int w = bbox.x; w < bbox.x + bbox.width; w += TILE_WIDTH) {
    for (int h = bbox.y; h < bbox.y + bbox.height; h += TILE_HEIGHT) {
      Point tl(w, h), tr(w + TILE_WIDTH, h);
      Point bl(w, h + TILE_HEIGHT), br(w + TILE_WIDTH, h + TILE_HEIGHT);
      Point center(w + TILE_WIDTH/2, h + TILE_HEIGHT/2);
      Tile t(tile_id++, tl, tr, br, bl, getRandomColor());
      double center_to_edge = pointPolygonTest(plane, center, true);
      bool is_inside = center_to_edge >= 0;
      if (!tileExists(t) && (is_inside || abs(center_to_edge) <= TILE_WIDTH/2)) {
        tiles.push_back(t);
      }
    }
  }
}

void generateTilesForPlaneSegments() {
  for (int ii = 0; ii < NUM_PLANE_SEGMENTS; ++ii) {
    generateTilesForPlaneSegment(planes[ii]);
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

int clicked = 0;

void onMouse(int event, int x, int y, int flags, void* param) {
  switch (event) {
  case CV_EVENT_LBUTTONDOWN:
    generateTilesForPlaneSegments();
    redraw();
    break;
  case CV_EVENT_RBUTTONDOWN:
    planes.clear();
    vector<Point> plane_segment;
    if (clicked++ == 0) {
      plane_segment.push_back(Point(266,260));
      plane_segment.push_back(Point(303,158));
      plane_segment.push_back(Point(591,230));
      plane_segment.push_back(Point(810,288));
      plane_segment.push_back(Point(944,386));
      plane_segment.push_back(Point(425,613));
      plane_segment.push_back(Point(387,420));
    } else {
      plane_segment.push_back(Point(166,160));
      plane_segment.push_back(Point(303,158));
      plane_segment.push_back(Point(591,230));
      plane_segment.push_back(Point(610,288));
      plane_segment.push_back(Point(684,486));
      plane_segment.push_back(Point(225,613));
      plane_segment.push_back(Point(187,420));
    }
    planes.push_back(plane_segment);
    redraw();
    break;
  }
}

int main(int argc, char *argv[]) {
  vector<Point> plane_segment;
  plane_segment.push_back(Point(366,360));
  plane_segment.push_back(Point(403,258));
  plane_segment.push_back(Point(591,230));
  plane_segment.push_back(Point(710,288));
  plane_segment.push_back(Point(643,374));
  plane_segment.push_back(Point(544,386));
  plane_segment.push_back(Point(497,465));
  plane_segment.push_back(Point(425,513));
  plane_segment.push_back(Point(387,420));
  planes.push_back(plane_segment);

  // show the image
  clearCanvas();
  redraw();
  namedWindow("Plane", CV_WINDOW_AUTOSIZE);
  imshow("Plane", canvas);

  // set the mouse callback for adding vertices
  setMouseCallback("Plane", onMouse, NULL);
  waitKey(0);

  return 0;
}
