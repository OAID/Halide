#include "CodeGen_C.h"
#include<iostream>
#include<string.h>
#include "IROperator.h" //strided_ramp_base
namespace Halide {
namespace Internal {

using std::string;
using std::vector;
using std::ostringstream;
using std::ostream;
#if defined(WITH_X86)
namespace{

class CodeGen_C_RISCV : public CodeGen_C {
public:

    CodeGen_C_RISCV(Target,std::ostream &);
protected:
    // std::string print_type(Type, AppendSpaceIfNeeded space_option = DoNotAppendSpace) override;
    using CodeGen_C::visit;

    // void visit(const Ramp *) override;
    // void visit(const Add *) override;
    // void visit(const Load *) override;
    // void visit(const Store *) override;
    // void visit(const Broadcast *) override;

};
CodeGen_C_RISCV::CodeGen_C_RISCV(Target t,std::ostream &dest) : CodeGen_C(dest,t,CodeGen_C::OutputKind::CImplementation) {
}
// string CodeGen_C_RISCV::print_type(Type type, AppendSpaceIfNeeded space_option) {
//     return type_to_c_type(type, space_option == AppendSpace);
// }


// void CodeGen_C_RISCV::visit(const Ramp *op) {
// }
// void CodeGen_C_RISCV::visit(const Broadcast *op) {
// }
// void CodeGen_C_RISCV::visit(const Add * op) {
// }
// void CodeGen_C_RISCV::visit(const Load *op) {
// }
// void CodeGen_C_RISCV::visit(const Store *op) {
// }
} // namespace

std::unique_ptr<CodeGen_C> new_CodeGen_C_RISCV(const Target &target, std::ostream & dest) {
    return std::make_unique<CodeGen_C_RISCV>(target,dest);
}
#else  // WITH_X86

std::unique_ptr<CodeGen_C> new_CodeGen_C_RISCV(const Target &target, std::ostream & dest) {
    user_error << "x86 not enabled for this build of Halide.\n";
    return nullptr;
}

#endif  // WITH_X86

} // namespace Internal
} // namespace Halide
