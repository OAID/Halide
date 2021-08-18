#ifndef HALIDE_CODEGEN_TARGETS_H
#define HALIDE_CODEGEN_TARGETS_H

/** \file
 * Provides constructors for code generators for various targets.
 */

#include <memory>

namespace Halide {

struct Target;

namespace Internal {

class CodeGen_Posix;

/** Construct CodeGen object for a variety of targets. */
std::unique_ptr<CodeGen_Posix> new_CodeGen_ARM(const Target &target);
std::unique_ptr<CodeGen_Posix> new_CodeGen_Hexagon(const Target &target);
std::unique_ptr<CodeGen_Posix> new_CodeGen_MIPS(const Target &target);
std::unique_ptr<CodeGen_Posix> new_CodeGen_PowerPC(const Target &target);
std::unique_ptr<CodeGen_Posix> new_CodeGen_RISCV(const Target &target);
std::unique_ptr<CodeGen_Posix> new_CodeGen_X86(const Target &target);
std::unique_ptr<CodeGen_Posix> new_CodeGen_WebAssembly(const Target &target);

class CodeGen_C;

std::unique_ptr<CodeGen_C> new_CodeGen_C_X86(const Target &target,std::ostream &dest);
std::unique_ptr<CodeGen_C> new_CodeGen_C_ARM(const Target &target,std::ostream &dest);
std::unique_ptr<CodeGen_C> new_CodeGen_C_RISCV(const Target &target,std::ostream &dest);

}  // namespace Internal
}  // namespace Halide

#endif
