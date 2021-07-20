#include "Halide.h"
using namespace Halide;

namespace {
class GEMM:public Halide::Generator<GEMM>{
    public:
    Input<Buffer<float>>    input_a{"input_a", 2}; //(dim0,dim1)=(width,height)=(K,M)
    Input<Buffer<float>>    input_b{"input_b", 2}; //(dim0,dim1)=(width,height)=(N,K)
    Output<Buffer<float>>   output{"output", 2};  //(dim0,dim1)=(width,height)=(N,M)



    void generate() {

        Var x("x"), y("y");
        Func prod("prod");
        const Expr K = input_a.width(); 
        RDom k(0, K);

        // Algorithm
        prod(x, y) = 0.0f;
        prod(x, y) += input_a(k, y) * input_b(x, k);
        output(x, y) = prod(x, y);

        // schedule()
        
        prod.update()
            .reorder(x, y, k)
            .vectorize(x, 8);

        // Var xi("xi"), yi("yi"), xii("xii"), yii("yii"), xt("xt"), yt("yt"), xy("xy");
        
        // output.tile(x, y, xi, yi, 16, 32)
        //             .fuse(x, y, xy).parallel(xy)
        //             .split(yi, yi, yii, 4)
        //             .vectorize(xi, 8)
        //             .unroll(xi)
        //             .unroll(yii);

        // prod.compute_at(output, yi)
        //     .vectorize(x, 8).unroll(y);

        // prod.update()
        //     .reorder(x, y, k)
        //     .vectorize(x, 8)
        //     .unroll(x)
        //     .unroll(y)
        //     .unroll(k, 2);
    }

};
} //namespace 
HALIDE_REGISTER_GENERATOR(GEMM, gemm)
