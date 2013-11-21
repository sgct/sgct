g++ -O3 -s -DTIXML_USE_STL -c *.cpp -static-libgcc -static-libstdc++
ar -rc libtinyxml2.a *.o
g++ -g -DTIXML_USE_STL -c *.cpp -static-libgcc -static-libstdc++
ar -rc libtinyxml2_d.a *.o
DEL *.o *.obj
pause