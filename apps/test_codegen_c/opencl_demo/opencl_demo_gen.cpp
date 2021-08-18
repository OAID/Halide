#include "Halide.h"

namespace {

class Opencl_demo : public Halide::Generator<Opencl_demo> {
public:
    Input<Buffer<float>> a{"a", 1};
    Input<Buffer<float>> b{"b", 1};
    Output<Buffer<float>> c{"c", 1};

    void generate() {
        Var x("x");
        c(x) = a(x) + b(x);

        Target target = get_target();
        if (target.has_gpu_feature()) {
            Var xo, xi;
            c.gpu_tile(x, xo,xi, 16);
        }
    }
};
}  // namespace

HALIDE_REGISTER_GENERATOR(Opencl_demo, opencl_demo)
