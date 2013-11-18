#!/bin/bash
# merge libs
echo "Copying folders... "
if [ ! -d examples ];
then
	mkdir examples
fi
cp -r ../../src/apps/clustertest examples/clustertest
cp -r ../../src/apps/example1 examples/example1
cp -r ../../src/apps/example1_opengl3 examples/example1_opengl3
cp -r ../../src/apps/gamepadExample examples/gamepadExample
cp -r ../../src/apps/heightMappingExample examples/heightMappingExample
cp -r ../../src/apps/heightMappingExample_opengl3 examples/heightMappingExample_opengl3
cp -r ../../src/apps/kinectExample examples/kinectExample
cp -r ../../src/apps/osgExample examples/osgExample
cp -r ../../src/apps/osgExampleRTT examples/osgExampleRTT
cp -r ../../src/apps/postFXExample examples/postFXExample
cp -r ../../src/apps/postFXExample_opengl3 examples/postFXExample_opengl3
cp -r ../../src/apps/renderToTexture examples/renderToTexture
cp -r ../../src/apps/sgct_template examples/sgct_template
cp -r ../../src/apps/SGCTRemote examples/SGCTRemote
cp -r ../../src/apps/simpleNavigationExample examples/simpleNavigationExample
cp -r ../../src/apps/simpleShaderExample examples/simpleShaderExample
cp -r ../../src/apps/simpleShaderExample_opengl3 examples/simpleShaderExample_opengl3
cp -r ../../src/apps/soundExample examples/soundExample
cp -r ../../src/apps/textureExample examples/textureExample
cp -r ../../src/apps/textureExample_opengl3 examples/textureExample_opengl3
cp -r ../../src/apps/trackingExample examples/trackingExample

echo "Copying files... "
cp ../../src/apps/osgExample/airplane.ive examples/osgExampleRTT/airplane.ive
cp ../../src/apps/textureExample/box.png examples/renderToTexture/box.png

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

echo "Done."
