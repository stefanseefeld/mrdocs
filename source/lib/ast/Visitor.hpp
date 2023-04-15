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

//
// This file implements the Mapper piece of the clang-doc tool. It implements
// a RecursiveASTVisitor to look at each declaration and populate the info
// into the internal representation. Each seen declaration is serialized to
// to bitcode and written out to the ExecutionContext as a KV pair where the
// key is the declaration's USR and the value is the serialized bitcode.
//

#ifndef MRDOX_VISITOR_HPP
#define MRDOX_VISITOR_HPP

#include <mrdox/Config.hpp>
#include <mrdox/MetadataFwd.hpp>
#include <mrdox/Reporter.hpp>
#include <clang/Tooling/Execution.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <utility>
#include <unordered_map>

namespace clang {
namespace mrdox {

/** Traverses an AST and stores the nodes in a table.

    An object of this type 
*/
class Visitor
    : public RecursiveASTVisitor<Visitor>
    , public ASTConsumer
{
    struct FileFilter
    {
        llvm::SmallString<0> prefix;
        bool include = true;
    };

    tooling::ExecutionContext& exc_;
    Config const& config_;
    Reporter& R_;
    std::unordered_map<
        clang::SourceLocation::UIntTy,
        FileFilter> fileFilter_;

public:
    Visitor(
        tooling::ExecutionContext& exc,
        Config const& config,
        Reporter& R) noexcept
        : exc_(exc)
        , config_(config)
        , R_(R)
    {
    }

//private:
    void HandleTranslationUnit(ASTContext& Context) override;
    bool VisitNamespaceDecl(NamespaceDecl const* D);
    bool VisitRecordDecl(RecordDecl const* D);
    bool VisitEnumDecl(EnumDecl const* D);
    bool VisitCXXMethodDecl(CXXMethodDecl const* D);
    bool VisitFunctionDecl(FunctionDecl const* D);
    bool VisitTypedefDecl(TypedefDecl const* D);
    bool VisitTypeAliasDecl(TypeAliasDecl const* D);

private:
    template <typename T>
    bool mapDecl(T const* D);

    int
    getLine(
        NamedDecl const* D,
        ASTContext const& Context) const;

    llvm::SmallString<128>
    getFile(
        NamedDecl const* D, 
        ASTContext const& Context,
        StringRef RootDir,
        bool& IsFileInRootDir) const;

    comments::FullComment*
    getComment(
        NamedDecl const* D,
        ASTContext const& Context) const;
};

} // mrdox
} // clang

#endif