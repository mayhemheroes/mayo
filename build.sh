mkdir mayo-build && cd mayo-build
CASCADE_INC_DIR=`dpkg -L libocct-foundation-dev | grep -i "Standard_Version.hxx" | sed "s/\/Standard_Version.hxx//i"`
CASCADE_LIB_DIR=`dpkg -L libocct-foundation-dev | grep -i "libTKernel.so" | sed "s/\/libTKernel.so//i"`
qmake .. CASCADE_INC_DIR=$CASCADE_INC_DIR CASCADE_LIB_DIR=$CASCADE_LIB_DIR
make -j8
