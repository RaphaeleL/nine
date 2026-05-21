#define QOL_IMPLEMENTATION
#define QOL_STRIP_PREFIX
#include "../libs/build.h"

#include "test_helloworld.h"

int main() {
    init_logger(.only=LOG_HINT, .only_set=true, .time=true, .color=true);
    return test_run_all();
}

