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

#include <mrdox/Config.hpp>
#include <llvm/Support/YAMLParser.h>
#include <llvm/Support/YAMLTraits.h>

template<>
struct llvm::yaml::MappingTraits<
    clang::mrdox::Config::filter>
{
    static void mapping(
        IO &io, clang::mrdox::Config::filter &f)
    {
        io.mapOptional("include", f.include);
        io.mapOptional("exclude", f.exclude);
    }
};

template<>
struct llvm::yaml::MappingTraits<
    clang::mrdox::Config>
{
    static void mapping(
        IO &io, clang::mrdox::Config &f)
    {
        io.mapOptional("namespaces",   f.namespaces);
        io.mapOptional("files",        f.files);
        io.mapOptional("entities",     f.entities);
        io.mapOptional("project-name", f.ProjectName);
        io.mapOptional("public-only",  f.PublicOnly);
        io.mapOptional("output-dir",   f.OutDirectory);
    }
};

//------------------------------------------------

namespace clang {
namespace mrdox {

Config::
Config()
{
    if (auto p = ::getenv("MRDOX_SOURCE_ROOT"))
        SourceRoot = p;
    else
    {
        llvm::SmallString<128> SourceRootDir;
        llvm::sys::fs::current_path(SourceRootDir);
        SourceRoot = SourceRootDir.c_str();
    }

    if (auto p = ::getenv("MRDOX_REPOSITORY_URL"))
        RepositoryUrl.emplace(p);
    else if (auto q = ::getenv("DRONE_REMOTE_URL"))
        RepositoryUrl.emplace(q);
}

bool
Config::
shouldSkipFile(
    llvm::StringRef filePath) const noexcept
{
    return false;
}

std::error_code
Config::
load(
    const std::string & name,
    const std::source_location & loc)
{
    auto ct = llvm::MemoryBuffer::getFile(SourceRoot + "/" + name);
    if (!ct)
        return ct.getError();

    llvm::yaml::Input yin{**ct};

    yin >> *this;
    if ( yin.error() )
        return yin.error();

    return {};
}

} // mrdox
} // clang
