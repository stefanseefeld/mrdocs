//
// This is a derivative work. originally part of the LLVM Project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
//
// Official repository: https://github.com/cppalliance/mrdox
//

#ifndef MRDOX_FUNCTION_HPP
#define MRDOX_FUNCTION_HPP

#include <mrdox/meta/FieldType.hpp>
#include <mrdox/meta/Function.hpp>
#include <mrdox/meta/Symbol.hpp>
#include <mrdox/meta/Template.hpp>
#include <mrdox/meta/Symbols.hpp>
#include <clang/Basic/Specifiers.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/SmallVector.h>
#include <vector>

namespace clang {
namespace mrdox {

/// The string used for unqualified names
using UnqualifiedName = llvm::SmallString<16>;

// TODO: Expand to allow for documenting templating and default args.
// Info for functions.
struct FunctionInfo : SymbolInfo
{
    bool IsMethod = false; // Indicates whether this function is a class method.
    Reference Parent;      // Reference to the parent class decl for this method.
    TypeInfo ReturnType;   // Info about the return type of this function.
    llvm::SmallVector<FieldTypeInfo, 4> Params; // List of parameters.
    // Access level for this method (public, private, protected, none).
    // AS_public is set as default because the bitcode writer requires the enum
    // with value 0 to be used as the default.
    // (AS_public = 0, AS_protected = 1, AS_private = 2, AS_none = 3)
    AccessSpecifier Access = AccessSpecifier::AS_public;

    // Full qualified name of this function, including namespaces and template
    // specializations.
    SmallString<16> FullName;

    // When present, this function is a template or specialization.
    llvm::Optional<TemplateInfo> Template;

    StorageClass storageClass = SC_None;
    RefQualifierKind refQualifier = RQ_None;

    bool isConst : 1 = false;
    bool isConsteval : 1 = false;
    bool isConstexpr : 1 = false;
    bool isInline : 1 = false;
    bool isNoExcept : 1 = false;
    bool isSpecialMember : 1 = false; // dtor, move/copy construct or assign
    bool isTrailingReturn : 1 = false;
    bool isVariadic : 1 = false; // has a C-style "..." variadic
    bool isVirtual : 1 = false;
    bool isVolatile : 1 = false;

    //--------------------------------------------

    static constexpr InfoType type_id = InfoType::IT_function;

    explicit
    FunctionInfo(
        SymbolID id_ = SymbolID())
        : SymbolInfo(InfoType::IT_function, id_)
    {
    }

    void merge(FunctionInfo&& I);
};

//------------------------------------------------

} // mrdox
} // clang

#endif
