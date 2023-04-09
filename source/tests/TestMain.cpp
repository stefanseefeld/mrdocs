//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
//
// Official repository: https://github.com/cppalliance/mrdox
//

#include <mrdox/Corpus.hpp>
#include <mrdox/Errors.hpp>
#include <mrdox/Generator.hpp>
#include <clang/Tooling/AllTUsExecution.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/StandaloneExecution.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/ThreadPool.h>
#include <string_view>
#include <vector>

// Each test comes as a pair of files.
// A .cpp file containing valid declarations,
// and a .xml file containing the expected output
// of the XML generator, which must match exactly.

namespace clang {
namespace mrdox {

//------------------------------------------------

/** Compilation database for a single .cpp file.
*/
class SingleFile
    : public tooling::CompilationDatabase
{
    std::vector<tooling::CompileCommand> cc_;

public:
    SingleFile(
        llvm::StringRef dir,
        llvm::StringRef file,
        llvm::StringRef output)
    {
        std::vector<std::string> cmds;
        cmds.emplace_back("clang");
        cmds.emplace_back(file);
        cc_.emplace_back(
            dir,
            file,
            std::move(cmds),
            output);
        cc_.back().Heuristic = "unit test";
    }

    std::vector<tooling::CompileCommand>
    getCompileCommands(
        llvm::StringRef FilePath) const override
    {
        if(! FilePath.equals(cc_.front().Filename))
            return {};
        return { cc_.front() };
    }

    std::vector<std::string>
    getAllFiles() const override
    {
        return { cc_.front().Filename };
    }

    std::vector<tooling::CompileCommand>
    getAllCompileCommands() const override
    {
        return { cc_.front() };
    }
};

//------------------------------------------------

namespace fs = llvm::sys::fs;
namespace path = llvm::sys::path;

template<class F>
bool
visitDirectory(
    llvm::StringRef dirPath,
    Reporter& R,
    F const& f)
{
    std::error_code ec;
    llvm::SmallString<32> dir(dirPath);
    llvm::SmallString<32> out;
    auto const& cdir(dir);

    path::remove_dots(dir, true);
    fs::directory_iterator const end{};
    fs::directory_iterator iter(dir, ec, false);
    if(R.failed("visitDirectory", ec))
        return false;
    while(iter != end)
    {
        if(iter->type() == fs::file_type::directory_file)
        {
            if(! visitDirectory(iter->path(), R, f))
                return false;
        }
        else if(
            iter->type() == fs::file_type::regular_file &&
            path::extension(iter->path()).equals_insensitive(".cpp"))
        {
            out = iter->path();
            path::replace_extension(out, "xml");
            f(cdir, iter->path(), out);
        }
        else
        {
            // we don't handle this type
        }
        iter.increment(ec);
        if(R.failed("fs::directory_iterator::increment", ec))
            return false;
    }
    return true;
}

void
testResult(
    Corpus const& corpus,
    Config const& config,
    llvm::StringRef file,
    llvm::StringRef out,
    Generator const& gen,
    Reporter& R)
{
    std::string xml;
    if(! gen.buildString(xml, corpus, config, R))
    {
        R.testFailed();
        return;
    }
    std::error_code ec;
    fs::file_status stat;
    ec = fs::status(out, stat, false);
    if(ec == std::errc::no_such_file_or_directory)
    {
        // create the xml file and write to it
        llvm::raw_fd_ostream os(out, ec, llvm::sys::fs::OF_None);
        if(! ec)
        {
            os << xml;
        }
        else
        {
            llvm::errs() <<
                "Writing \"" << out << "\" failed: " <<
                ec.message() << "\n";
            R.testFailed();
        }
    }
    else if(! R.failed("fs::status", ec))
    {
        if(stat.type() == fs::file_type::regular_file)
        {
            auto bufferResult = llvm::MemoryBuffer::getFile(out, false);
            if(R.failed("MemoryBuffer::getFile", bufferResult))
                return;
            std::string_view got(bufferResult->get()->getBuffer());
            if(xml != bufferResult->get()->getBuffer())
            {
                llvm::errs() <<
                    "File: \"" << file << "\" failed.\n"
                    "Expected:\n" <<
                    bufferResult->get()->getBuffer() << "\n" <<
                    "Got:\n" <<
                    xml << "\n";
                R.testFailed();
            }
        }
        else
        {
            // VFALCO report that it is not a regular file
        }
    }
}

//------------------------------------------------

int
testMain(int argc, const char** argv)
{
    Config config;
    Reporter R;
    auto const gen = makeXMLGenerator();

    llvm::ThreadPool Pool(llvm::hardware_concurrency(
        tooling::ExecutorConcurrency));
    for(int i = 1; i < argc; ++i)
    {
        visitDirectory(
            llvm::StringRef(argv[i]), R,
        [&](llvm::StringRef dir_,
            llvm::StringRef file_,
            llvm::StringRef out_)
        {
            Pool.async([&config, &R, &gen,
                dir = dir_.str(),
                file = file_.str(),
                out = out_.str()]
            {
                SingleFile db(dir, file, out);
                tooling::StandaloneToolExecutor ex(
                    db, { std::string(file) });
                auto corpus = Corpus::build(ex, config, R);
                if(corpus)
                    testResult(*corpus, config, file, out, *gen, R);
            });
        });
    }
    Pool.wait();
    return R.getExitCode();
}

} // mrdox
} // clang

int
main(int argc, const char** argv)
{
    llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
    return clang::mrdox::testMain(argc, argv);
}
