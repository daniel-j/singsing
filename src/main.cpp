
#include "app.hpp"

int main() {
    App app;
    int r = app.init();
    if (r != 0) return r;
    return app.launch();
}
