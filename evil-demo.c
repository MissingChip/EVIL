#include "evil.h"

int main(int argc, char** argv){
    VkExtent2D extent = {680, 420};
    EWindowState window_state;
    eCreateWindow(&extent, &window_state);
    EState state;
    state.width = extent.width;
    state.height = extent.height;
    ezInitialize(&state.instance, &window_state);
}