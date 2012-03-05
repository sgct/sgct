g++ -g -DTIXML_USE_STL -c *.cpp
ar -rc libtinyxml.a *.o
cp ./libtinyxml.a /usr/local/tinyxml/staticLib/
rm -rf *.so *.a *.o
pause