#pragma once

namespace core
{
template <typename T>
concept trivial = __is_trivial(T);
}
