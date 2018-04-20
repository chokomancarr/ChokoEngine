CXX = g++ 
CXXFLAGS = -O0 -g -std=c++11 -I../src/ -I/usr/local/include/freetype2/ -D CHOKO_LAIT -D PLATFORM_LNX -D GLEW_STATIC
LDFLAGS = -Wl,-rpath='$$ORIGIN' -L/usr/lib -L/usr/local/lib -static-libstdc++
LIBS = -L../lib -lpthread -lm -lGL -lGLEW -lglfw -ljpeg -lfreetype -lz

OBJS = obj/main.o obj/Gromacs.o hdr.o lodepng.o obj/AudioEngine.o obj/ChokoLait.o obj/Editor.o obj/EditorShortcuts.o obj/Engine.o obj/RenderEngine.o obj/SceneObjects.o obj/Types.o obj/Water.o obj/asset/animation.o obj/asset/animclip.o obj/asset/assetmanager.o obj/asset/assetobject.o obj/asset/audioclip.o obj/asset/cameffect.o obj/asset/compute.o obj/asset/cubemap.o obj/asset/material.o obj/asset/mesh.o obj/asset/resources.o obj/asset/shader.o obj/asset/img/background.o obj/asset/img/rendertexture.o obj/asset/img/texture.o obj/asset/img/texture3d.o obj/asset/img/videotexture.o obj/core/audio.o obj/core/bbox.o obj/core/debug.o obj/core/display.o obj/core/font.o obj/core/input.o obj/core/io.o obj/core/mvp.o obj/core/time.o obj/core/ui.o obj/scene/object.o obj/scene/scene.o obj/scene/sceneobject.o obj/scene/transform.o obj/scene/comp/animator.o obj/scene/comp/armature.o obj/scene/comp/arrayrenderer.o obj/scene/comp/audiolistener.o obj/scene/comp/audiosource.o obj/scene/comp/camera.o obj/scene/comp/component.o obj/scene/comp/ik.o obj/scene/comp/light.o obj/scene/comp/meshfilter.o obj/scene/comp/meshrenderer.o obj/scene/comp/particlesystem.o obj/scene/comp/reflprobe.o obj/scene/comp/reflquad.o obj/scene/comp/scenescript.o obj/scene/comp/skinnedmeshrenderer.o obj/scene/comp/texturerenderer.o obj/scene/comp/voxelrenderer.o obj/utils/color.o obj/utils/fft.o obj/utils/net.o obj/utils/precedurals.o obj/utils/random.o obj/utils/rect.o obj/utils/serial.o obj/utils/stream.o obj/utils/strext.o obj/utils/xml.o

HEADERS = AssetObjects.h AudioEngine.h ChokoLait.h Defines.h Editor.h Engine.h SceneObjects.h SceneScriptResolver.h Water.h asset/animation.h asset/animclip.h asset/assetmanager.h asset/assetobject.h asset/audioclip.h asset/cameffect.h asset/compute.h asset/cubemap.h asset/material.h asset/mesh.h asset/resources.h asset/shader.h asset/img/background.h asset/img/rendertexture.h asset/img/texture.h asset/img/texture3d.h asset/img/videotexture.h core/audio.h core/bbox.h core/debug.h core/display.h core/font.h core/input.h core/io.h core/math.h core/mvp.h core/time.h core/ui.h scene/object.h scene/scene.h scene/sceneobject.h scene/transform.h scene/comp/animator.h scene/comp/armature.h scene/comp/arrayrenderer.h scene/comp/audiolistener.h scene/comp/audiosource.h scene/comp/camera.h scene/comp/component.h scene/comp/ik.h scene/comp/light.h scene/comp/meshfilter.h scene/comp/meshrenderer.h scene/comp/particlesystem.h scene/comp/reflprobe.h scene/comp/reflquad.h scene/comp/scenescript.h scene/comp/skinnedmeshrenderer.h scene/comp/texturerenderer.h scene/comp/voxelrenderer.h utils/color.h utils/fft.h utils/ifaddrs.h utils/net.h utils/precedurals.h utils/ptrext.h utils/random.h utils/rect.h utils/serial.h utils/stream.h utils/strext.h utils/xml.h

../build/.out: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

obj/AudioEngine.o: AudioEngine.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ AudioEngine.cpp

obj/ChokoLait.o: ChokoLait.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ ChokoLait.cpp

obj/Editor.o: Editor.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ Editor.cpp

obj/EditorShortcuts.o: EditorShortcuts.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ EditorShortcuts.cpp

obj/Engine.o: Engine.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ Engine.cpp

obj/main.o: main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ main.cpp

obj/RenderEngine.o: RenderEngine.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ RenderEngine.cpp

obj/SceneObjects.o: SceneObjects.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ SceneObjects.cpp

obj/Types.o: Types.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ Types.cpp

obj/Water.o: Water.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ Water.cpp

obj/asset/animation.o: asset/animation.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/animation.cpp

obj/asset/animclip.o: asset/animclip.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/animclip.cpp

obj/asset/assetmanager.o: asset/assetmanager.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/assetmanager.cpp

obj/asset/assetobject.o: asset/assetobject.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/assetobject.cpp

obj/asset/audioclip.o: asset/audioclip.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/audioclip.cpp

obj/asset/cameffect.o: asset/cameffect.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/cameffect.cpp

obj/asset/compute.o: asset/compute.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/compute.cpp

obj/asset/cubemap.o: asset/cubemap.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/cubemap.cpp

obj/asset/material.o: asset/material.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/material.cpp

obj/asset/mesh.o: asset/mesh.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/mesh.cpp

obj/asset/resources.o: asset/resources.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/resources.cpp

obj/asset/shader.o: asset/shader.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/shader.cpp

obj/asset/img/background.o: asset/img/background.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/img/background.cpp

obj/asset/img/rendertexture.o: asset/img/rendertexture.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/img/rendertexture.cpp

obj/asset/img/texture.o: asset/img/texture.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/img/texture.cpp

obj/asset/img/texture3d.o: asset/img/texture3d.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/img/texture3d.cpp

obj/asset/img/videotexture.o: asset/img/videotexture.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ asset/img/videotexture.cpp

obj/core/audio.o: core/audio.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ core/audio.cpp

obj/core/bbox.o: core/bbox.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ core/bbox.cpp

obj/core/debug.o: core/debug.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ core/debug.cpp

obj/core/display.o: core/display.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ core/display.cpp

obj/core/font.o: core/font.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ core/font.cpp

obj/core/input.o: core/input.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ core/input.cpp

obj/core/io.o: core/io.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ core/io.cpp

obj/core/mvp.o: core/mvp.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ core/mvp.cpp

obj/core/time.o: core/time.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ core/time.cpp

obj/core/ui.o: core/ui.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ core/ui.cpp

obj/scene/object.o: scene/object.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/object.cpp

obj/scene/scene.o: scene/scene.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/scene.cpp

obj/scene/sceneobject.o: scene/sceneobject.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/sceneobject.cpp

obj/scene/transform.o: scene/transform.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/transform.cpp

obj/scene/comp/animator.o: scene/comp/animator.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/animator.cpp

obj/scene/comp/armature.o: scene/comp/armature.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/armature.cpp

obj/scene/comp/arrayrenderer.o: scene/comp/arrayrenderer.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/arrayrenderer.cpp

obj/scene/comp/audiolistener.o: scene/comp/audiolistener.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/audiolistener.cpp

obj/scene/comp/audiosource.o: scene/comp/audiosource.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/audiosource.cpp

obj/scene/comp/camera.o: scene/comp/camera.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/camera.cpp

obj/scene/comp/component.o: scene/comp/component.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/component.cpp

obj/scene/comp/ik.o: scene/comp/ik.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/ik.cpp

obj/scene/comp/light.o: scene/comp/light.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/light.cpp

obj/scene/comp/meshfilter.o: scene/comp/meshfilter.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/meshfilter.cpp

obj/scene/comp/meshrenderer.o: scene/comp/meshrenderer.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/meshrenderer.cpp

obj/scene/comp/particlesystem.o: scene/comp/particlesystem.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/particlesystem.cpp

obj/scene/comp/reflprobe.o: scene/comp/reflprobe.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/reflprobe.cpp

obj/scene/comp/reflquad.o: scene/comp/reflquad.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/reflquad.cpp

obj/scene/comp/scenescript.o: scene/comp/scenescript.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/scenescript.cpp

obj/scene/comp/skinnedmeshrenderer.o: scene/comp/skinnedmeshrenderer.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/skinnedmeshrenderer.cpp

obj/scene/comp/texturerenderer.o: scene/comp/texturerenderer.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/texturerenderer.cpp

obj/scene/comp/voxelrenderer.o: scene/comp/voxelrenderer.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ scene/comp/voxelrenderer.cpp

obj/utils/color.o: utils/color.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ utils/color.cpp

obj/utils/fft.o: utils/fft.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ utils/fft.cpp

obj/utils/net.o: utils/net.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ utils/net.cpp

obj/utils/precedurals.o: utils/precedurals.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ utils/precedurals.cpp

obj/utils/random.o: utils/random.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ utils/random.cpp

obj/utils/rect.o: utils/rect.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ utils/rect.cpp

obj/utils/serial.o: utils/serial.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ utils/serial.cpp

obj/utils/stream.o: utils/stream.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ utils/stream.cpp

obj/utils/strext.o: utils/strext.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ utils/strext.cpp

obj/utils/xml.o: utils/xml.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ utils/xml.cpp

obj/Gromacs.o: Gromacs.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ Gromacs.cpp

clean:
	find ./obj -name '*.o' -exec rm {} \;
	rm -f ../build/.out
