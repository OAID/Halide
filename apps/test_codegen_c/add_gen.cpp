#include "Halide.h"
using namespace Halide;

class Addone:public Halide::Generator<Addone>{
    public:
    Input<Buffer<float>> input{"input", 2};
    Input<float> offset{"offset"};
    Output<Buffer<float>> output{"output", 2};

    Var x, y;
    void generate() {

        output(x,y)=input(x,y)+offset;
    }
    void schedule() {
        output.vectorize(x, 4).parallel(y);
    }
};
HALIDE_REGISTER_GENERATOR(Addone, add)
