#pragma once

namespace core {
template <typename T1, typename T2>
concept same_as = __is_same(T1, T2) && __is_same(T2, T1);
}
