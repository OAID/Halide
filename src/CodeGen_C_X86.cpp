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

class CodeGen_C_X86 : public CodeGen_C {
public:

    CodeGen_C_X86(Target,std::ostream &);
protected:
    std::string print_type(Type, AppendSpaceIfNeeded space_option = DoNotAppendSpace) override;
    using CodeGen_C::visit;

    void visit(const Ramp *) override;
    void visit(const Add *) override;
    void visit(const Load *) override;
    void visit(const Store *) override;
    void visit(const Broadcast *) override;

};
CodeGen_C_X86::CodeGen_C_X86(Target t,std::ostream &dest) : CodeGen_C(dest,t,CodeGen_C::OutputKind::CImplementation)
{
   
}
string CodeGen_C_X86::print_type(Type type, AppendSpaceIfNeeded space_option) {
    if(type.bits() == 32 &&type.lanes()==8)
    {
        return "__m256 ";
    }
    return type_to_c_type(type, space_option == AppendSpace);
}

bool fmadd_pass(const Expr &a, const Expr &b,vector<Expr> &result) {
    Type t = a.type();
    internal_assert(b.type() == t);

    if (!(t.is_float() && t.bits() == 32 && t.lanes() >= 4)) {
        return false;
    }
    if(b.node_type()==IRNodeType::Mul)
    {
        const Mul* mul = b.as<Mul>();
        std::vector<Expr> args = {mul->a, mul->b, a};
        result.swap(args);
        return true;
    }
    return false;
}
void CodeGen_C_X86::visit(const Ramp *op) {
}
void CodeGen_C_X86::visit(const Broadcast *op) {
    Type vector_type = op->type.with_lanes(op->lanes);
    string id_value = print_expr(op->value);
    string rhs;
    if (op->lanes > 1) {
        if(print_type(op->type)=="__m256 " )
        {
            rhs = "_mm256_set1_ps("+ id_value + ")";
        }
        else
        {
            rhs = print_type(vector_type) + "_ops::broadcast(" + id_value + ")";
        }
    } else {
        rhs = id_value;
    }
    print_assignment(vector_type, rhs);
}
void CodeGen_C_X86::visit(const Add * op)
{
    if(op->type.bits() == 32 && op->type.lanes()==8)
    {
        vector<Expr> matches;
        if(fmadd_pass(op->a,op->b,matches))
        {
            string sa = print_expr(matches[0]);
            string sb = print_expr(matches[1]);
            string sc = print_expr(matches[2]);
            print_assignment(op->type, "_mm256_fmadd_ps(" + sa + ", " + sb + "," + sc +")");
            return;
        }
        string sa = print_expr(op->a);
        string sb = print_expr(op->b);
        print_assignment(op->type, "_mm256_add_ps(" + sa + ", " + sb+")");
        return;
    }

    visit_binop(op->type, op->a, op->b, "+");
}
void CodeGen_C_X86::visit(const Load *op) {

    string load_intrinsic="_mm256_loadu_ps";

    // TODO: We could replicate the logic in the llvm codegen which decides whether
    // the vector access can be aligned. Doing so would also require introducing
    // aligned type equivalents for all the vector types.
    ostringstream rhs;

    Type t = op->type;
    string name = print_name(op->name);

    // If we're loading a contiguous ramp into a vector, just load the vector
    Expr dense_ramp_base = strided_ramp_base(op->index, 1);
    if (dense_ramp_base.defined() && is_const_one(op->predicate)) {
        internal_assert(t.is_vector());
        string id_ramp_base = print_expr(dense_ramp_base);
        if(target.arch==Target::X86)
        {
            rhs <<  load_intrinsic << "(" << name << "+ " << id_ramp_base << ")";
        }
        else
        {
            rhs << print_type(t) + "_ops::load(" << name << ", " << id_ramp_base << ")";
        }
    } else if (op->index.type().is_vector()) {
        // If index is a vector, gather vector elements.
        internal_assert(t.is_vector());
        string id_index = print_expr(op->index);
        if (is_const_one(op->predicate)) {
            // if((target.arch==Target::X86) ||(target.arch==Target::ARM )){
                const Ramp *ramp_index = op->index.as<Ramp>();
                string id_ramp_base = print_expr(ramp_index->base);
                rhs <<  load_intrinsic << "(" << name << "+ " << id_ramp_base << ")";
            // }
            // else
            // {
            //     rhs << print_type(t) + "_ops::load_gather(" << name << ", " << id_index << ")";
            // }
        } else {
            string id_predicate = print_expr(op->predicate);
            rhs << print_type(t) + "_ops::load_predicated(" << name << ", " << id_index << ", " << id_predicate << ")";
        }
    } else {
        user_assert(is_const_one(op->predicate)) << "Predicated scalar load is not supported by C backend.\n";

        string id_index = print_expr(op->index);
        bool type_cast_needed = !(allocations.contains(op->name) &&
                                  allocations.get(op->name).type.element_of() == t.element_of());
        if (type_cast_needed) {
            const char *const_flag = output_kind == CPlusPlusImplementation ? "const " : "";
            rhs << "((" << const_flag << print_type(t.element_of()) << " *)" << name << ")";
        } else {
            rhs << name;
        }
        rhs << "[" << id_index << "]";
    }
    print_assignment(t, rhs.str());
}

void CodeGen_C_X86::visit(const Store *op) {
    string store_intrinsic="_mm256_storeu_ps";

    Type t = op->value.type();

    if (inside_atomic_mutex_node) {
        user_assert(t.is_scalar())
            << "The vectorized atomic operation for the store" << op->name
            << " is lowered into a mutex lock, which does not support vectorization.\n";
    }

    // Issue atomic store if we are in the designated producer.
    if (emit_atomic_stores) {
        stream << "#if defined(_OPENMP)\n";
        stream << "#pragma omp atomic\n";
        stream << "#else\n";
        stream << "#error \"Atomic stores in the C backend are only supported in compilers that support OpenMP.\"\n";
        stream << "#endif\n";
    }

    string id_value = print_expr(op->value);
    string name = print_name(op->name);

    // TODO: We could replicate the logic in the llvm codegen which decides whether
    // the vector access can be aligned. Doing so would also require introducing
    // aligned type equivalents for all the vector types.

    // If we're writing a contiguous ramp, just store the vector.
    Expr dense_ramp_base = strided_ramp_base(op->index, 1);
    if (dense_ramp_base.defined() && is_const_one(op->predicate)) {
        internal_assert(op->value.type().is_vector());
        string id_ramp_base = print_expr(dense_ramp_base);
        // if(target.arch==Target::X86)
        // {
            stream << get_indent() << store_intrinsic<< "(" << name << " + " << id_ramp_base << ", "<< id_value<<");\n";
        // }
        // else
        // {
        //     stream << get_indent() << print_type(t) + "_ops::store(" << id_value << ", " << name << ", " << id_ramp_base << ");\n";
        // }
        
    } else if (op->index.type().is_vector()) {
        // If index is a vector, scatter vector elements.
        internal_assert(t.is_vector());
        string id_index = print_expr(op->index);
        if (is_const_one(op->predicate)) {
            //  if((target.arch==Target::X86) ||(target.arch==Target::ARM))
            //  {
                const Ramp *ramp_index = op->index.as<Ramp>();
                string id_ramp_base = print_expr(ramp_index->base);
                stream << get_indent() << store_intrinsic<< "(" << name << " + " << id_ramp_base << ", "<< id_value<<");\n";
            //  }
            //  else
            //  {
            //      stream << get_indent() << print_type(t) + "_ops::store_scatter(" << id_value << ", " << name << ", " << id_index << ");\n";
            //  }

        } else {
            string id_predicate = print_expr(op->predicate);
            stream << get_indent() << print_type(t) + "_ops::store_predicated(" << id_value << ", " << name << ", " << id_index << ", " << id_predicate << ");\n";
        }
    } else {
        user_assert(is_const_one(op->predicate)) << "Predicated scalar store is not supported by C backend.\n";

        bool type_cast_needed =
            t.is_handle() ||
            !allocations.contains(op->name) ||
            allocations.get(op->name).type != t;

        string id_index = print_expr(op->index);
        stream << get_indent();
        if (type_cast_needed) {
            stream << "((" << print_type(t) << " *)" << name << ")";
        } else {
            stream << name;
        }
        stream << "[" << id_index << "] = " << id_value << ";\n";
    }
    cache.clear();
}
} // namespace

std::unique_ptr<CodeGen_C> new_CodeGen_C_X86(const Target &target, std::ostream & dest) {
    return std::make_unique<CodeGen_C_X86>(target,dest);
}
#else  // WITH_X86

std::unique_ptr<CodeGen_C> new_CodeGen_C_X86(const Target &target, std::ostream & dest) {
    user_error << "x86 not enabled for this build of Halide.\n";
    return nullptr;
}

#endif  // WITH_X86

} // namespace Internal
} // namespace Halide
