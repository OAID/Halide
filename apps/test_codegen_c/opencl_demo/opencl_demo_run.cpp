#include "HalideBuffer.h"
#include "HalideRuntime.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "HalideRuntimeOpenCL.h"


#include "opencl_demo.h"
using namespace Halide::Runtime;


int main(int argc, char **argv) {
    printf("main here\n");

    const auto *interface = halide_opencl_device_interface();
    assert(interface->compute_capability != nullptr);
    int major, minor;
    int err = interface->compute_capability(nullptr, &major, &minor);
    if (err != 0 || (major == 1 && minor < 2)) {
        printf("[SKIP] OpenCl %d.%d is less than required 1.2.\n", major, minor);
        return 0;
    }
    const int n=100000;
    
    int i;
    Buffer<float> a(n);
    Buffer<float> b(n);
    Buffer<float> c(n);
    for( i = 0; i < n; i++ )
    {
        a(i) = sinf(i)*sinf(i);
        b(i) = cosf(i)*cosf(i);
    }
    // Explicitly copy data to the GPU.
    a.set_host_dirty();
    b.set_host_dirty();

    opencl_demo(a,b,c);

    c.copy_to_host();
    float sum = 0;
    for(i=0; i<n; i++)
        sum += c(i);
    printf("final result: %f\n", sum/n);
    printf("Success!\n");

    return 0;
}
