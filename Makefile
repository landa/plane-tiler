all:
	g++ plane_tiler.cpp -o plane_tiler `pkg-config --libs opencv`
