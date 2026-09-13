#pragma once

#include <cstring>
#include <deque>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include <core/box.hpp>
#include <core/robin_map.hpp>
#include <core/var.hpp>
#include <util/basic_types.hpp>
#include <util/log.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace util::nbt {
enum class tag_type : u8 {
    end        = 0,
    byte       = 1,
    short_     = 2,
    int_       = 3,
    long_      = 4,
    float_     = 5,
    double_    = 6,
    byte_array = 7,
    string     = 8,
    list       = 9,
    compound   = 10,
    int_array  = 11,
    long_array = 12,
};

enum class empty_type : u8 {};
}

namespace std {
    std::ostream& operator<<(ostream& os, const util::nbt::empty_type&) {
        return os << "<empty_type>";
    }
}

namespace util::nbt {
inline constexpr std::string to_string(tag_type value) {
    auto map = [] {
        core::static_int_map<u8, std::string_view, 13> m;
        m.emplace(u8(tag_type::end), "end");
        m.emplace(u8(tag_type::byte), "byte");
        m.emplace(u8(tag_type::short_), "short_");
        m.emplace(u8(tag_type::int_), "int");
        m.emplace(u8(tag_type::long_), "long");
        m.emplace(u8(tag_type::float_), "float");
        m.emplace(u8(tag_type::double_), "double");
        m.emplace(u8(tag_type::byte_array), "byte_array");
        m.emplace(u8(tag_type::string), "string");
        m.emplace(u8(tag_type::list), "list");
        m.emplace(u8(tag_type::compound), "compound");
        m.emplace(u8(tag_type::int_array), "int_array");
        m.emplace(u8(tag_type::long_array), "long_array");
        return m;
    }();
    auto it = map.find(u8(value));
    if (it != map.end()) {
        return std::string(it.value());
    }
    return "unknown(" + std::to_string(int(value)) + ")";
}

template <typename T>
struct list;

struct list_base {
    virtual ~list_base() = default;

    void foreach(this auto&& it, auto value);

    template <typename T>
    list<T>& unsafe_cast();

    template <typename T>
    const list<T>& unsafe_cast() const;

    tag_type type;
};

class compound;

namespace types {
    using byte_array = std::vector<core::byte>;
    using compound   = core::box<compound>;
    using list       = core::box<list_base>;
    using int_array  = std::vector<i32>;
    using long_array = std::vector<i64>;
} // namespace types

namespace details {
    auto type_dispatch(tag_type type, auto&& func, auto&&... args);
}

template <typename... Ts>
struct tag_var : public core::var<Ts...> {
    using core::var<Ts...>::var;

    auto&& at(this auto&& it, const std::string& name);
    std::vector<std::string> keys() const;
    auto find(this auto&& it, const std::string& name);

    tag_type get_tag_type() const {
        return tag_type{this->index()};
    }

    auto&& as_list(this auto&& it) {
        return *fwd(it).template get<types::list>();
    }

    template <typename T>
    auto&& as_list(this auto&& it) {
        auto ok = details::type_dispatch(fwd(it).template get<types::list>()->type, []<typename U>(core::type_t<U>) {
            return core::is_same<U, T>;
        });
        if (!ok) {
            throw std::runtime_error("invalid list type");
        }
        return (*fwd(it).template get<types::list>()).template unsafe_cast<T>();
    }

    template <typename T>
    list<T>* try_as_list(this auto&& it) {
        auto ok = details::type_dispatch(fwd(it).template get<types::list>()->type, []<typename U>(core::type_t<U>) {
            return core::is_same<U, T>;
        });
        if (!ok) {
            return nullptr;
        }
        return &(*fwd(it).template get<types::list>()).template unsafe_cast<T>();
    }
};

using tag_value =
    tag_var<empty_type, i8, i16, i32, i64, f32, f64, types::byte_array, std::string, types::list, types::compound, types::int_array, types::long_array>;

struct tag {
    std::string name;
    tag_value   value;
};

struct compound {
    tag_value& insert_or_replace(const std::string& name, tag_value t) {
        auto [it, inserted] = index.emplace(name, data.size());
        if (inserted) {
            data.push_back(tag{.name = name, .value = core::mov(t)});
        } else {
            data[it->second].value = core::mov(t);
        }
        return data[it->second].value;
    }

    tag_value& operator[](const std::string& name) {
        return insert_or_replace(name, empty_type{});
    }

    auto&& at(this auto&& it, const std::string& name) {
        auto found = it.index.find(name);
        if (found == it.index.end()) {
            throw std::out_of_range("Key '" + name + "' was not found in compound");
        }
        return fwd(it).data[found->second].value;
    }

    // TODO: use iterator
    tag_value* find(this auto&& it, const std::string& name) {
        auto found = it.index.find(name);
        if (found == it.index.end()) {
            return nullptr;
        }
        return &fwd(it).data[found->second].value;
    }

    std::vector<std::string> keys() const {
        // TODO: non copy iterator
        std::vector<std::string> result;
        result.reserve(data.size());
        for (auto&& v : data) {
            result.push_back(v.name);
        }
        return result;
    }

    std::deque<tag> data;
    std::map<std::string, u32> index;
};

template <typename... Ts>
auto&& tag_var<Ts...>::at(this auto&& it, const std::string& name) {
    if (!it.is_type(core::type<types::compound>)) {
        throw std::runtime_error("tag not a compound");
    }

    return fwd(it).template get<types::compound>()->at(name);
}

template <typename... Ts>
auto tag_var<Ts...>::find(this auto&& it, const std::string& name) {
    if (!it.is_type(core::type<types::compound>)) {
        throw std::runtime_error("tag not a compound");
    }

    return fwd(it).template get<types::compound>()->find(name);
}

template <typename... Ts>
std::vector<std::string> tag_var<Ts...>::keys() const {
    if (!this->is_type(core::type<types::compound>)) {
        throw std::runtime_error("tag not a compound");
    }
    return this->template get<types::compound>()->keys();
}

template <typename T>
struct list : public list_base {
    std::vector<T> data;
};

template <>
struct list<empty_type> : public list_base {
};

namespace details {
template <core::integral T>
T read_num(std::span<const core::byte>& data) {
    if constexpr (sizeof(T) == 1) {
        auto result = (T)data.front();
        data = data.subspan(1);
        return result;
    }
    else {
        T result;
        std::memcpy(&result, data.data(), sizeof(result));
        result = std::byteswap(result);
        data = data.subspan(sizeof(result));
        return result;
    }
}

template <core::floating_point T>
T read_num(std::span<const core::byte>& data) {
    if constexpr (sizeof(T) == 4) {
        u32 result;
        std::memcpy(&result, data.data(), sizeof(result));
        result = std::byteswap(result);
        data = data.subspan(sizeof(result));
        f32 res;
        std::memcpy(&res, &result, sizeof(res));
        return res;
    }
    else {
        u64 result;
        std::memcpy(&result, data.data(), sizeof(result));
        result = std::byteswap(result);
        data = data.subspan(sizeof(result));
        f64 res;
        std::memcpy(&res, &result, sizeof(res));
        return res;
    }
}

std::string read_string(u16 len, std::span<const core::byte>& data) {
    std::string result;
    if (len == 0) {
        return result;
    }
    result.resize(len);

    // TODO: check sizes
    std::memcpy(result.data(), data.data(), len);
    data = data.subspan(len);

    return result;
}

std::string read_name(std::span<const core::byte>& data) {
    auto name_len = read_num<u16>(data);
    auto name = read_string(name_len, data);
    return name;
}

auto type_dispatch(tag_type type, auto&& func, auto&&... args) {
    switch (type) {
    case tag_type::byte: return fwd(func)(core::type<i8>, fwd(args)...);
    case tag_type::short_: return fwd(func)(core::type<i16>, fwd(args)...);
    case tag_type::int_: return fwd(func)(core::type<i32>, fwd(args)...);
    case tag_type::long_: return fwd(func)(core::type<i64>, fwd(args)...);
    case tag_type::float_: return fwd(func)(core::type<f32>, fwd(args)...);
    case tag_type::double_: return fwd(func)(core::type<f64>, fwd(args)...);
    case tag_type::byte_array: return fwd(func)(core::type<std::vector<core::byte>>, fwd(args)...);
    case tag_type::string: return fwd(func)(core::type<std::string>, fwd(args)...);
    case tag_type::list: return fwd(func)(core::type<core::box<list_base>>, fwd(args)...);
    case tag_type::compound: return fwd(func)(core::type<core::box<compound>>, fwd(args)...);
    case tag_type::int_array: return fwd(func)(core::type<std::vector<i32>>, fwd(args)...);
    case tag_type::long_array: return fwd(func)(core::type<std::vector<i64>>, fwd(args)...);
    case tag_type::end: return fwd(func)(core::type<empty_type>, fwd(args)...);
    }
    __builtin_unreachable();
}
} // namespace details

template <typename T>
list<T>& list_base::unsafe_cast() {
    return *static_cast<list<T>*>(this);
}

template <typename T>
const list<T>& list_base::unsafe_cast() const {
    return *static_cast<list<T>*>(this);
}

void list_base::foreach (this auto&& it, auto func) {
    details::type_dispatch(
        it.type,
        core::overloaded{
            [&]<typename T>(core::type_t<T>) {
                for (auto&& e : it.template unsafe_cast<T>().data) {
                    func(e);
                }
            },
            [](core::type_t<empty_type>) {},
        }
    );
}

namespace details {
template <typename T>
T parse_value_static(std::span<const core::byte>& data);

template <typename T>
void parse_list(std::span<const core::byte>& data, list<T>& list, u32 size) {
    if constexpr (core::is_same<T, empty_type>) {
    } else {
        list.data.reserve(size);
        for (u32 i = 0; i < size; ++i) {
            list.data.push_back(parse_value_static<T>(data));
        }
    }
}

//static int indent2 = -2;

core::box<compound> parse_compound(std::span<const core::byte>& data) {
    auto result = core::boxed<compound>();

    //indent2 += 2;
    while (true) {
        if (data.empty()) {
            break;
        }

        auto type = (tag_type)read_num<i8>(data);
        //glog().warn("{}parse type: {}", std::string(std::max(0, indent2), ' '), to_string(type));
        if (type == tag_type::end) {
            break;
        }

        auto name  = read_name(data);
        //glog().warn("{}parse name: {}", std::string(std::max(0, indent2), ' '), name);
        auto value = type_dispatch(
            type, []<typename T>(core::type_t<T>, std::span<const core::byte>& data) { return tag_value{core::type<T>, parse_value_static<T>(data)}; }, data
        );
        result->data.push_back(tag{.name = core::mov(name), .value = core::mov(value)});
        result->index.emplace(result->data.back().name, u32(result->data.size() - 1));
    }
    //indent2 -= 2;

    return core::mov(result);
}

template <typename T>
T parse_value_static(std::span<const core::byte>& data) {
    if constexpr (core::any_of<T, i8, i16, i32, i64, f32, f64>) {
        return read_num<T>(data);
    } else if constexpr (core::is_same<T, std::vector<core::byte>>) {
        auto                    len = read_num<u32>(data);
        std::vector<core::byte> bytes(len);
        std::memcpy(bytes.data(), data.data(), len);
        data = data.subspan(len);
        return std::move(bytes);
    } else if constexpr (core::is_same<T, std::string>) {
        return read_name(data);
    } else if constexpr (core::is_same<T, core::box<list_base>>) {
        auto element_type  = (tag_type)read_num<u8>(data);
        auto element_count = read_num<u32>(data);
        //glog().warn("{}list type: {}", std::string(std::max(0, indent2), ' '), to_string(element_type));
        //glog().warn("{}list size: {}", std::string(std::max(0, indent2), ' '), element_count);
        return type_dispatch(element_type, [&]<typename U>(core::type_t<U>, std::span<const core::byte>& data, u32 size) -> core::box<list_base> {
            auto result = core::boxed<list<U>>();
            parse_list(data, *result, size);
            result->type = element_type;
            return core::mov(result);
        }, data, element_count);
    } else if constexpr (core::is_same<T, core::box<compound>>) {
        return parse_compound(data);
    } else if constexpr (core::is_same<T, std::vector<i32>>) {
        auto len = read_num<u32>(data);
        std::vector<i32> array(len);
        for (auto& e: array) {
            e = read_num<i32>(data);
        }
        return array;
    } else if constexpr (core::is_same<T, std::vector<i64>>) {
        auto len = read_num<u32>(data);
        std::vector<i64> array(len);
        for (auto& e: array) {
            e = read_num<i64>(data);
        }
        return array;
    }

    __builtin_unreachable();
}

std::string to_str(i8 value) {
    return std::string(1, "0123456789abcdef"[u8(value) >> 4]) + std::string(1, "0123456789abcdef"[u8(value) & 0xf]);
}

void print_compound(const core::box<compound>& compound, std::string& out, size_t indent = 0);

void print_list(const core::box<list_base>& list, std::string& out, size_t indent) {
    //glog().warn("list: {}", list.get());
    list->foreach (
        core::overloaded{
            [&](const i8& v) { out += std::string(indent, ' ') + to_str(v) + '\n'; },
            [&](const i16& v) { out += std::string(indent, ' ') + std::to_string(v) + '\n'; },
            [&](const i32& v) { out += std::string(indent, ' ') + std::to_string(v) + '\n'; },
            [&](const i64& v) { out += std::string(indent, ' ') + std::to_string(v) + '\n'; },
            [&](const f32& v) { out += std::string(indent, ' ') + std::to_string(v) + '\n'; },
            [&](const f64& v) { out += std::string(indent, ' ') + std::to_string(v) + '\n'; },
            [&](const std::string& v) { out += std::string(indent, ' ') + v + '\n'; },
            [&](const core::box<list_base>& v) { print_list(v, out, indent + 2); },
            [&](const core::box<compound>& v) { print_compound(v, out, indent + 2); },
            [&](const std::vector<core::byte>& v) {
                out += std::string(indent, ' ');
                for (auto b : v) {
                    out += to_str(i8(b));
                    out += ", ";
                }
                if (!v.empty()) {
                    out.resize(out.size() - 2);
                }
                out += '\n';
            },
            [&](const std::vector<i32>& v) {
                for (auto b : v) {
                    out += std::string(indent, ' ');
                    out += std::to_string(b);
                    out += ",\n";
                }
                if (!v.empty()) {
                    out.resize(out.size() - 2);
                }
                out += '\n';
            },
            [&](const std::vector<i64>& v) {
                for (auto b : v) {
                    out += std::string(indent, ' ');
                    out += std::to_string(b);
                    out += ",\n";
                }
                if (!v.empty()) {
                    out.resize(out.size() - 2);
                }
                out += '\n';
            },
            [&](const empty_type&) {
                out += std::string(indent, ' ') + "<empty_type>\n";
            }
        }
    );
}

void print_compound(const core::box<compound>& comp, std::string& out, size_t indent) {
    //out += std::string(indent, ' ');
    indent += 2;
    for (auto&& tag : comp->data) {
        core::visit(
            tag.value,
            core::overloaded{
                [&](const i8& v) { out += std::string(indent, ' ') + "Byte(\"" + tag.name + "\"): " + to_str(v) + "\n"; },
                [&](const i16& v) { out += std::string(indent, ' ') + "Short(\"" + tag.name + "\"): " + std::to_string(v) + "\n"; },
                [&](const i32& v) { out += std::string(indent, ' ') + "Int(\"" + tag.name + "\"): " + std::to_string(v) + "\n"; },
                [&](const i64& v) { out += std::string(indent, ' ') + "Long(\"" + tag.name + "\"): " + std::to_string(v) + "\n"; },
                [&](const f32& v) { out += std::string(indent, ' ') + "Float(\"" + tag.name + "\"): " + std::to_string(v) + "\n"; },
                [&](const f64& v) { out += std::string(indent, ' ') + "Double(\"" + tag.name + "\"): " + std::to_string(v) + "\n"; },
                [&](const std::string& v) { out += std::string(indent, ' ') + "String(\"" + tag.name + "\"):" + v + "\n"; },
                [&](const core::box<list_base>& v) {
                    out += std::string(indent, ' ') + "List\"(" + tag.name + "\"):\n";
                    print_list(v, out, indent + 2);
                },
                [&](const core::box<compound>& v) {
                    out += std::string(indent, ' ') + "Compound\"(" + tag.name + "\"):\n";
                    print_compound(v, out, indent + 2);
                },
                [&](const std::vector<core::byte>& v) {
                    out += std::string(indent, ' ') + "ByteArray\"(" + tag.name + "\"):\n";
                    out += std::string(indent + 2, ' ');
                    for (auto b : v) {
                        out += to_str(i8(b));
                        out += ", ";
                    }
                    if (!v.empty()) {
                        out.resize(out.size() - 2);
                    }
                    out += '\n';
                },
                [&](const std::vector<core::i32>& v) {
                    out += std::string(indent, ' ') + "IntArray\"(" + tag.name + "\"):\n";
                    for (auto b : v) {
                        out += std::string(indent + 2, ' ');
                        out += std::to_string(b);
                        out += ",\n";
                    }
                    if (!v.empty()) {
                        out.resize(out.size() - 2);
                    }
                    out += '\n';
                },
                [&](const std::vector<core::i64>& v) {
                    out += std::string(indent, ' ') + "LongArray\"(" + tag.name + "\"):\n";
                    for (auto b : v) {
                        out += std::string(indent + 2, ' ');
                        out += std::to_string(b);
                        out += ",\n";
                    }
                    if (!v.empty()) {
                        out.resize(out.size() - 2);
                    }
                    out += '\n';
                },
                [&](const empty_type&) {
                    out += std::string(indent, ' ') + "<empty_type>\n";
                }
            }
        );
    }
}
} // namespace details

auto parse(std::span<const core::byte> data) {
    return details::parse_compound(data);
}

void print(const core::box<compound>& compound) {
    std::string out;
    details::print_compound(compound, out);
    glog().info("{}", out);
}
} // namespace util::nbt

#undef fwd
