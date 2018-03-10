#Makefile for Linux

CXX = g++
CXXFLAGS = -g -I/usr/local/include/freetype2/ -D CHOKO_LAIT
LIBS = -L../lib -lpthread -lm -lGL -lGLEW -lglfw -ljpeg -lfreetype -lz

OBJS = main.o Water.o Animation.o AssetObjects.o AudioEngine.o ChokoLait.o Engine.o Editor.o hdr.o lodepng.o Networking.o RenderEngine.o SceneObjects.o SceneScriptResolver.o Shader.o Types.o

HEADERS = Water.h AssetObjects.h Engine.h AudioEngine.h hdr.h lodepng.h ChokoLait.h Defines.h SceneObjects.h Editor.h SceneScriptResolver.h

../build/.out: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LIBS)

main.o: main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -DGLEW_STATIC -o $@ main.cpp

Water.o: Water.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -DGLEW_STATIC -o $@ Water.cpp

Animation.o: Animation.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -DGLEW_STATIC -o $@ Animation.cpp

AssetObjects.o: AssetObjects.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -DGLEW_STATIC -o $@ AssetObjects.cpp

ChokoLait.o: ChokoLait.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -DGLEW_STATIC -o $@ ChokoLait.cpp

Engine.o: Engine.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -DGLEW_STATIC -o $@ Engine.cpp

SceneObjects.o: SceneObjects.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -DGLEW_STATIC -o $@ SceneObjects.cpp

Shader.o: Shader.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -DGLEW_STATIC -o $@ Shader.cpp

clean:
	rm -f *.o
	rm -f ../build/.out
