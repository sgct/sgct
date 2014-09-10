#!/bin/bash
# merge libs
echo "Creating $1 directory"
if [ ! -d $1 ];
then
	mkdir $1
fi
cp CMakeLists.txt $1/CMakeLists.txt
cp readme.txt $1/readme.txt
cd $1
#
echo "Copying folders... "
if [ ! -d examples ];
then
	mkdir examples
fi
cp -r ../../src/apps/clustertest examples/clustertest
cp -r ../../src/apps/depthBuffer examples/depthBuffer
cp -r ../../src/apps/example1 examples/example1
cp -r ../../src/apps/example1_opengl3 examples/example1_opengl3
cp -r ../../src/apps/gamepadExample examples/gamepadExample
cp -r ../../src/apps/heightMappingExample examples/heightMappingExample
cp -r ../../src/apps/heightMappingExample_opengl3 examples/heightMappingExample_opengl3
cp -r ../../src/apps/kinectExample examples/kinectExample
cp -r ../../src/apps/MRTExample examples/MRTExample
cp -r ../../src/apps/MRTExample_opengl3 examples/MRTExample_opengl3
cp -r ../../src/apps/osgExample examples/osgExample
cp -r ../../src/apps/osgExampleRTT examples/osgExampleRTT
cp -r ../../src/apps/postFXExample examples/postFXExample
cp -r ../../src/apps/postFXExample_opengl3 examples/postFXExample_opengl3
cp -r ../../src/apps/renderToTexture examples/renderToTexture
cp -r ../../src/apps/sgct_template examples/sgct_template
cp -r ../../src/apps/SGCTRemote examples/SGCTRemote
cp -r ../../src/apps/simpleNavigationExample examples/simpleNavigationExample
cp -r ../../src/apps/simpleNavigationExample_opengl3 examples/simpleNavigationExample_opengl3
cp -r ../../src/apps/simpleShaderExample examples/simpleShaderExample
cp -r ../../src/apps/simpleShaderExample_opengl3 examples/simpleShaderExample_opengl3
#cp -r ../../src/apps/soundExample examples/soundExample
cp -r ../../src/apps/textureExample examples/textureExample
cp -r ../../src/apps/textureExample_opengl3 examples/textureExample_opengl3
cp -r ../../src/apps/trackingExample examples/trackingExample

echo "Copying files... "
cp ../../src/apps/osgExample/airplane.ive examples/osgExampleRTT/airplane.ive
cp ../../src/apps/textureExample/box.png examples/renderToTexture/box.png
cp -r ../../config config
cp -r ../../include include
cp -r ../../readme readme
if [ ! -d lib ];
then
	mkdir lib
fi
cp /usr/local/lib/libsgct_cpp11d.a lib/libsgct_cpp11d.a
cp /usr/local/lib/libsgct_cpp11.a lib/libsgct_cpp11.a
cp /usr/local/lib/libsgctd.a lib/libsgctd.a
cp /usr/local/lib/libsgct.a lib/libsgct.a
if [ ! -d doc ];
then
	mkdir doc
fi
if [ ! -d build ];
then
	mkdir build
fi
#
cd build
echo "Running Cmake..."
cmake -G Xcode -DCMAKE_BUILD_TYPE=Release -D SGCT_EXAMPLES_OSG=On -D SGCT_PLACE_TARGETS_IN_SOURCE_TREE=On ../
echo "Building.."
xcodebuild -target ALL_BUILD -configuration Release clean
xcodebuild -target ALL_BUILD -configuration Release
cd ..
rm -rf build
rm -f CMakeLists.txt
#cd build_cpp11
#xcodebuild -target ALL_BUILD -configuration Release clean
#xcodebuild -target ALL_BUILD -configuration Release
#cd ..
cp examples/clustertest/user_cmake/CMakeLists.txt examples/clustertest/CMakeLists.txt
rm -rf examples/clustertest/user_cmake
#
cp examples/depthBuffer/user_cmake/CMakeLists.txt examples/depthBuffer/CMakeLists.txt
rm -rf examples/depthBuffer/user_cmake
#
cp examples/example1/user_cmake/CMakeLists.txt examples/example1/CMakeLists.txt
rm -rf examples/example1/user_cmake
#
cp examples/example1_opengl3/user_cmake/CMakeLists.txt examples/example1_opengl3/CMakeLists.txt
rm -rf examples/example1_opengl3/user_cmake
#
cp examples/gamepadExample/user_cmake/CMakeLists.txt examples/gamepadExample/CMakeLists.txt
rm -rf examples/gamepadExample/user_cmake
#
cp examples/heightMappingExample/user_cmake/CMakeLists.txt examples/heightMappingExample/CMakeLists.txt
rm -rf examples/heightMappingExample/user_cmake
#
cp examples/heightMappingExample_opengl3/user_cmake/CMakeLists.txt examples/heightMappingExample_opengl3/CMakeLists.txt
rm -rf examples/heightMappingExample_opengl3/user_cmake
#
cp examples/kinectExample/user_cmake/CMakeLists.txt examples/kinectExample/CMakeLists.txt
rm -rf examples/kinectExample/user_cmake
#
cp examples/MRTExample/user_cmake/CMakeLists.txt examples/MRTExample/CMakeLists.txt
rm -rf examples/MRTExample/user_cmake
#
cp examples/MRTExample_opengl3/user_cmake/CMakeLists.txt examples/MRTExample_opengl3/CMakeLists.txt
rm -rf examples/MRTExample_opengl3/user_cmake
#
cp examples/osgExample/user_cmake/CMakeLists.txt examples/osgExample/CMakeLists.txt
rm -rf examples/osgExample/user_cmake
#
cp examples/osgExampleRTT/user_cmake/CMakeLists.txt examples/osgExampleRTT/CMakeLists.txt
rm -rf examples/osgExampleRTT/user_cmake
#
cp examples/postFXExample/user_cmake/CMakeLists.txt examples/postFXExample/CMakeLists.txt
rm -rf examples/postFXExample/user_cmake
#
cp examples/postFXExample_opengl3/user_cmake/CMakeLists.txt examples/postFXExample_opengl3/CMakeLists.txt
rm -rf examples/postFXExample_opengl3/user_cmake
#
cp examples/renderToTexture/user_cmake/CMakeLists.txt examples/renderToTexture/CMakeLists.txt
rm -rf examples/renderToTexture/user_cmake
#
cp examples/sgct_template/user_cmake/CMakeLists.txt examples/sgct_template/CMakeLists.txt
rm -rf examples/sgct_template/user_cmake
#
cp examples/SGCTRemote/user_cmake/CMakeLists.txt examples/SGCTRemote/CMakeLists.txt
rm -rf examples/SGCTRemote/user_cmake
#
cp examples/simpleNavigationExample/user_cmake/CMakeLists.txt examples/simpleNavigationExample/CMakeLists.txt
rm -rf examples/simpleNavigationExample/user_cmake
#
cp examples/simpleNavigationExample_opengl3/user_cmake/CMakeLists.txt examples/simpleNavigationExample_opengl3/CMakeLists.txt
rm -rf examples/simpleNavigationExample_opengl3/user_cmake
#
cp examples/simpleShaderExample/user_cmake/CMakeLists.txt examples/simpleShaderExample/CMakeLists.txt
rm -rf examples/simpleShaderExample/user_cmake
#
cp examples/simpleShaderExample_opengl3/user_cmake/CMakeLists.txt examples/simpleShaderExample_opengl3/CMakeLists.txt
rm -rf examples/simpleShaderExample_opengl3/user_cmake
#
cp examples/textureExample/user_cmake/CMakeLists.txt examples/textureExample/CMakeLists.txt
rm -rf examples/textureExample/user_cmake
#
cp examples/textureExample_opengl3/user_cmake/CMakeLists.txt examples/textureExample_opengl3/CMakeLists.txt
rm -rf examples/textureExample_opengl3/user_cmake
#
cp examples/trackingExample/user_cmake/CMakeLists.txt examples/trackingExample/CMakeLists.txt
rm -rf examples/trackingExample/user_cmake

#
echo "Removing unwanted files..."
find . -name '*.layout' -type f -delete
find . -name '*.depend' -type f -delete
find . -name '*.cbp' -type f -delete
find . -name '*.sln' -type f -delete
find . -name '*.vcxproj' -type f -delete
find . -name '*.vcproj' -type f -delete
find . -name '*.suo' -type f -delete
find . -name '*.bat' -type f -delete
find . -name '*.DS_Store' -type f -delete
find ./ -name ".svn" | xargs rm -Rf
find ./ -name "obj" | xargs rm -Rf
echo "Done!"
