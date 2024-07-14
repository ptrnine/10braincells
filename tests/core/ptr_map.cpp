#include <catch2/catch_test_macros.hpp>

#include <core/robin_map.hpp>
#include <core/ranges/zip.hpp>
#include <util/log.hpp>

using namespace core;

TEST_CASE("static_ptr_map") {
    int* o = nullptr;
    auto p = o + 16;

    SECTION("sequential") {
        static_ptr_map<int*, std::string, 4> m;
        static_assert(m.capacity() == 4);

        for (size_t i = 0; i < m.capacity(); ++i)
            m.emplace(p + i, std::to_string(i));

        size_t i = 0;
        for (auto&& [ptr, v] : m) {
            CHECK(ptr == p + i);
            CHECK(v == std::to_string(i));
            ++i;
        }
        CHECK(i == m.capacity());
    }

    SECTION("collisions") {
        static_ptr_map<int*, std::string, 4> m;
        for (size_t i = 0; i < m.capacity(); ++i)
            /* Make index collisions */
            m.emplace(p + i * m.capacity(), std::to_string(i * m.capacity()));

        size_t i = 0;
        for (auto&& [ptr, v] : m) {
            CHECK(ptr == p + i);
            CHECK(v == std::to_string(i));
            i += m.capacity();
        }
        CHECK(i == m.capacity() * m.capacity());
    }

    SECTION("robin hood shifts") {
        static_ptr_map<int*, std::string, 4> m;
        auto get_keys = [](auto&& m) {
            array<const int*, 5> res;
            for (size_t i = 0; i < m.raw_data().size(); ++i)
                res[i] = m.raw_data()[i].key();
            return res;
        };
        auto get_dists = [](auto&& m) {
            array<size_t, 5> res;
            for (size_t i = 0; i < m.raw_data().size(); ++i)
                res[i] = m.raw_data()[i].distance();
            return res;
        };

        CHECK(m.size() == 0);
        CHECK(m.empty());

        m.emplace(p + 3, "1");
        CHECK(get_dists(m) == array<size_t, 5>{0, 0, 0, 1, 1});
        CHECK(get_keys(m) == array<const int*, 5>{o, o, o, p + 3, o});
        CHECK(m.size() == 1);
        CHECK(!m.empty());

        m.emplace(p + 2, "2");
        CHECK(get_dists(m) == array<size_t, 5>{0, 0, 1, 1, 1});
        CHECK(get_keys(m) == array<const int*, 5>{o, o, p + 2, p + 3, o});
        CHECK(m.size() == 2);
        CHECK(!m.empty());

        m.emplace(p + 6, "3");
        CHECK(get_dists(m) == array<size_t, 5>{2, 0, 1, 2, 1});
        CHECK(get_keys(m) == array<const int*, 5>{p + 3, o, p + 2, p + 6, o});
        CHECK(m.size() == 3);
        CHECK(!m.empty());

        m.emplace(p + 10, "0");
        CHECK(get_dists(m) == array<size_t, 5>{3, 3, 1, 2, 1});
        CHECK(get_keys(m) == array<const int*, 5>{p + 10, p + 3, p + 2, p + 6, o});
        CHECK(m.size() == 4);
        CHECK(!m.empty());

        CHECK(m.erase(p + 10));
        CHECK(get_dists(m) == array<size_t, 5>{2, 0, 1, 2, 1});
        CHECK(get_keys(m) == array<const int*, 5>{p + 3, o, p + 2, p + 6, o});
        CHECK(m.size() == 3);
        CHECK(!m.empty());

        CHECK(m.erase(p + 6));
        CHECK(get_dists(m) == array<size_t, 5>{0, 0, 1, 1, 1});
        CHECK(get_keys(m) == array<const int*, 5>{o, o, p + 2, p + 3, o});
        CHECK(m.size() == 2);
        CHECK(!m.empty());

        CHECK(m.erase(p + 2));
        CHECK(get_dists(m) == array<size_t, 5>{0, 0, 0, 1, 1});
        CHECK(get_keys(m) == array<const int*, 5>{o, o, o, p + 3, o});
        CHECK(m.size() == 1);
        CHECK(!m.empty());

        CHECK(m.erase(p + 3));
        CHECK(get_dists(m) == array<size_t, 5>{0, 0, 0, 0, 1});
        CHECK(get_keys(m) == array<const int*, 5>{o, o, o, o, o});
        CHECK(m.size() == 0);
        CHECK(m.empty());
    }

    SECTION("overflow") {
        static_ptr_map<int*, std::string, 4> m;
        m[p] = "0";
        m[p + 1] = "1";
        m[p + 2] = "2";
        m[p + 3] = "3";

        bool ok = false;
        try {
            m[p + 4] = "4";
        }
        catch (const robin_map_overflow&) {
            ok = true;
        }
        CHECK(ok);
    }

    SECTION("find") {
        static_ptr_map<int*, std::string, 4> m;
        m.emplace(p + 3, "1");
        m.emplace(p + 2, "2");
        m.emplace(p + 6, "3");
        m.emplace(p + 10, "0");

        auto f1 = m.find(p + 3);
        auto f2 = m.find(p + 2);
        auto f3 = m.find(p + 6);
        auto f4 = m.find(p + 10);

        CHECK(f1 != m.end());
        CHECK(f2 != m.end());
        CHECK(f3 != m.end());
        CHECK(f4 != m.end());

        CHECK(f1.key() == p + 3);
        CHECK(f1.value() == "1");
        CHECK(f2.key() == p + 2);
        CHECK(f2.value() == "2");
        CHECK(f3.key() == p + 6);
        CHECK(f3.value() == "3");
        CHECK(f4.key() == p + 10);
        CHECK(f4.value() == "0");

        f1.value() = "11";
        f2.value() = "22";
        f3.value() = "33";
        f4.value() = "00";

        f1 = m.find(p + 3);
        f2 = m.find(p + 2);
        f3 = m.find(p + 6);
        f4 = m.find(p + 10);

        CHECK(f1.value() == "11");
        CHECK(f2.value() == "22");
        CHECK(f3.value() == "33");
        CHECK(f4.value() == "00");
    }

    SECTION("at") {
        static_ptr_map<int*, std::string, 4> m;
        m.emplace(p + 3, "1");
        m.emplace(p + 2, "2");
        m.emplace(p + 6, "3");
        m.emplace(p + 10, "0");

        bool ok = false;
        try {
            auto& v = m.at(p + 14);
            v = "dffdsfs";
        }
        catch (const robin_map_key_not_found&) {
            ok = true;
        }
        CHECK(ok);
    }

    SECTION("replace") {
        static_ptr_map<int*, std::string, 4> m;
        auto [i1, inserted1] = m.emplace(p + 3, "1");
        auto [i2, inserted2] = m.emplace(p + 2, "2");
        auto [i3, inserted3] = m.emplace(p + 6, "3");
        auto [i4, inserted4] = m.emplace(p + 10, "0");
        CHECK(inserted1);
        CHECK(inserted2);
        CHECK(inserted3);
        CHECK(inserted4);

        auto [i5, inserted5] = m.emplace(p + 3, "-1");
        auto [i6, inserted6] = m.emplace(p + 2, "-1");
        auto [i7, inserted7] = m.emplace(p + 6, "-1");
        auto [i8, inserted8] = m.emplace(p + 10, "-1");
        CHECK(!inserted5);
        CHECK(!inserted6);
        CHECK(!inserted7);
        CHECK(!inserted8);
        CHECK(i5.value() == "1");
        CHECK(i6.value() == "2");
        CHECK(i7.value() == "3");
        CHECK(i8.value() == "0");

        auto [i9, inserted9] = m.insert_or_assign(p + 3, "i9");
        auto [i10, inserted10] = m.insert_or_assign(p + 2, "i10");
        m[p + 6] = "i11";
        m[p + 10] = "i12";
        CHECK(!inserted9);
        CHECK(!inserted10);
        CHECK(i9.value() == "i9");
        CHECK(i10.value() == "i10");
        CHECK(m[p + 6] == "i11");
        CHECK(m[p + 10] == "i12");
    }

    SECTION("clear") {
        static_ptr_map<int*, std::string, 4> m;
        m.emplace(p + 3, "1");
        m.emplace(p + 2, "2");
        m.emplace(p + 6, "3");
        m.emplace(p + 10, "0");

        CHECK(m.size() == 4);
        m.clear();
        CHECK(m.size() == 0);

        auto f1 = m.find(p + 3);
        auto f2 = m.find(p + 2);
        auto f3 = m.find(p + 6);
        auto f4 = m.find(p + 10);

        CHECK(f1 == m.end());
        CHECK(f2 == m.end());
        CHECK(f3 == m.end());
        CHECK(f4 == m.end());
    }
}
