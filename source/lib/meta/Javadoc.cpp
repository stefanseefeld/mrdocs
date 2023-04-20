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

#include <mrdox/meta/Javadoc.hpp>
#include <llvm/Support/Error.h>
#include <llvm/Support/Path.h>

namespace clang {
namespace mrdox {

template<class T>
concept a_Node =
    std::is_copy_constructible_v<T> &&
    std::three_way_comparable<T>;

static_assert(a_Node<Javadoc::Node>);
static_assert(a_Node<Javadoc::Text>);
static_assert(a_Node<Javadoc::StyledText>);
static_assert(a_Node<Javadoc::Block>);
static_assert(a_Node<Javadoc::Paragraph>);
static_assert(a_Node<Javadoc::Param>);
static_assert(a_Node<Javadoc::TParam>);
static_assert(a_Node<Javadoc::Code>);

//------------------------------------------------

Javadoc::
Javadoc(
    List<Block> blocks)
    : blocks_(std::move(blocks))
{
}

bool
Javadoc::
empty() const noexcept
{
    if( ! brief_ &&
        blocks_.empty())
    {
        return true;
    }
    return false;
}

bool
Javadoc::
operator==(
    Javadoc const& other) const noexcept
{
    return blocks_ == other.blocks_;
}

bool
Javadoc::
operator!=(
    Javadoc const& other) const noexcept
{
    return !(*this == other);
}

void
Javadoc::
merge(Javadoc&& other)
{
    // Unconditionally extend the blocks
    // since each decl may have a comment.
    if(other != *this)
        append(blocks_, std::move(other.blocks_));
}

auto
Javadoc::
findBrief() const noexcept ->
    List<Block>::const_iterator
{
    auto it = blocks_.begin();
    auto first = blocks_.end();
    for(;it != blocks_.end(); ++it)
    {
        if(it->kind == Kind::brief)
            return it;
        if( it->kind == Kind::paragraph &&
            first == blocks_.end())
        {
            first = it;
            ++it;
            goto got_first;
        }
    }
    return blocks_.end();
got_first:
    while(it != blocks_.end())
    {
        if(it->kind == Kind::brief)
            return it;
        ++it;
    }
    return first;
}

void
Javadoc::
calculateBrief()
{
return;
    Paragraph* brief = nullptr;
    for(auto& block : blocks_)
    {
        if(block.kind == Kind::brief)
        {
            brief = static_cast<Paragraph*>(&block);
            break;
        }
        if(block.kind == Kind::paragraph && ! brief)
            brief = static_cast<Paragraph*>(&block);
    }
    if(brief != nullptr)
    {
        brief_ = blocks_.extract_first_of<Paragraph>(
            [brief](Block& block)
            {
                return brief == &block;
            });
    }
}

} // mrdox
} // clang
