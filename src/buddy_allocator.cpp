#include <fmt/format.h>
#include <fmt/ranges.h>

#include <bit>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

struct allocation {
    size_t order;
};

template <>
struct fmt::formatter<allocation> : fmt::formatter<size_t> {
    template <typename FormatContext>
    auto format(allocation v, FormatContext& ctx) const {
        return formatter<size_t>::format(v.order, ctx);
    }
};

constexpr size_t order_of(size_t v) {
    return sizeof(size_t) * CHAR_BIT - 1 - std::countl_zero(v);
}

struct empty_struct {};

template <typename T>
struct binary_tree {
    using value_type = std::conditional_t<std::is_void_v<T>, empty_struct, T>;

    [[no_unique_address]] value_type value;
    std::unique_ptr<binary_tree> left;
    std::unique_ptr<binary_tree> right;
};

template <typename T, typename F>
void visit_preorder(size_t order, const binary_tree<T>& bt, F&& f) {
    if constexpr (std::is_void_v<T>) {
        f(order);
    } else {
        f(order, bt.value);
    }

    if (bt.left) {
        visit_preorder(order + 1, *bt.left, f);
    }

    if (bt.right) {
        visit_preorder(order + 1, *bt.right, f);
    }
}

template <typename T, typename F>
void visit_inorder(size_t order, const binary_tree<T>& bt, F&& f) {
    if (bt.left) {
        visit_inorder(order + 1, *bt.left, f);
    }

    if constexpr (std::is_void_v<T>) {
        f(order);
    } else {
        f(order, bt.value);
    }

    if (bt.right) {
        visit_inorder(order + 1, *bt.right, f);
    }
}

template <typename T, typename F>
void visit_postorder(size_t order, const binary_tree<T>& bt, F&& f) {
    if (bt.left) {
        visit_postorder(order + 1, *bt.left, f);
    }

    if (bt.right) {
        visit_postorder(order + 1, *bt.right, f);
    }

    if constexpr (std::is_void_v<T>) {
        f(order);
    } else {
        f(order, bt.value);
    }
}

template <typename T, typename F>
void visit_preorder(const binary_tree<T>& bt, F&& f) {
    visit_preorder(/*order=*/0, bt, f);
}

template <typename T, typename F>
void visit_inorder(const binary_tree<T>& bt, F&& f) {
    visit_inorder(/*order=*/0, bt, f);
}

template <typename T, typename F>
void visit_postorder(const binary_tree<T>& bt, F&& f) {
    visit_postorder(/*order=*/0, bt, f);
}

template <size_t Min, size_t Max>
struct buddy {
    static constexpr size_t MinOrder = order_of(Min);
    static constexpr size_t MaxOrder = order_of(Max);

    using allocations = binary_tree<void>;
    using allocations_ptr = std::unique_ptr<allocations>;

    buddy(size_t total_sz) {
        assert(total_sz >= Min);
        assert(total_sz <= Max);
        _data.resize(total_sz);

        // _allocs.root = node(free, MaxOrder);
    }

    void destroy() { _allocs = {}; }

    std::byte* base_pointer() noexcept { return _data.data(); }
    std::byte* end_pointer() noexcept { return _data.data() + _data.size(); }

    void* allocate(size_t sz, std::string_view label = {}) {
        size_t next_p2 = std::bit_ceil(sz);
        size_t order = order_of(next_p2);
        order = std::max(order, MinOrder);
        fmt::print("allocate({}: sz={}, o={})\n", label, next_p2, order);
        if (sz > Max) {
            // Cannot allocate more than the max block size.
            return nullptr;
        }

        return do_allocate(_allocs, MaxOrder, base_pointer(), order, label);
    }

    void free(void* ptr, std::string_view label = {}) {
        fmt::print("free({}: p={:p}, [{:p}, {:p}])\n", label, ptr,
                   (void*)base_pointer(), (void*)end_pointer());
        if (ptr < base_pointer() || ptr >= end_pointer()) {
            assert(!"invalid pointer (out of managed memory)");
            return;
        }
        [[maybe_unused]] bool ok = do_free(_allocs, MaxOrder, ptr,
                                           /*block_ptr=*/base_pointer(), label);
        assert(ok);
        if (!ok) {
            fmt::print("could not free pointer {:p}\n", ptr);
        }
    }

    void print(void* ptr = nullptr) {
        fmt::print("{:p}, [\n", ptr);
        if (_allocs) {
            visit_preorder(*_allocs, [](size_t order) {
                fmt::print("{:>{}}- order={}\n", "", order, MaxOrder - order);
            });
        }
        fmt::print("]\n");
    }

   private:
    void* do_allocate(allocations_ptr& allocs, size_t allocs_order,
                      std::byte* ptr, size_t order, std::string_view label) {
        assert(allocs_order >= MinOrder);

        size_t next_order = allocs_order - 1;
        if (allocs) {
            if (allocs_order <= order) {
                return nullptr;
            }
            bool try_new_only = (allocs_order == order + 1);

            if (!try_new_only || !allocs->left) {
                // fmt::print("do_allocate( left, {:p})\n", (void*)ptr);
                auto* p =
                    do_allocate(allocs->left, next_order, ptr, order, label);
                if (p) return p;
            }

            if (!try_new_only || !allocs->right) {
                ptr += 1 << next_order;
                // fmt::print("do_allocate(right, {:p})\n", (void*)ptr);
                return do_allocate(allocs->right, next_order, ptr, order,
                                   label);
            }
            return nullptr;
        }

        // fmt::print("do_allocate( new , {:p})\n", (void*)ptr);
        // Completely free node, create.
        allocs = std::make_unique<allocations>();
        if (allocs_order == order) {
            return ptr;
        }
        // fmt::print("do_allocate(split, {:p})\n", (void*)ptr);
        auto* p = do_allocate(allocs->left, next_order, ptr, order, label);
        assert(p);  // Must succeed, because we have just created it.
        return p;
    }

    bool do_free(allocations_ptr& allocs, size_t next_order, void* ptr,
                 std::byte* block_ptr, std::string_view label) {
        std::byte* end_block_ptr = block_ptr + (1 << next_order);
        std::byte* mid_block_ptr = block_ptr + (1 << (next_order - 1));
        fmt::print("do_free({}: o={}, p={:p}, bp=[{:p}, {:p}, {:p}])\n", label,
                   next_order, (void*)ptr, (void*)block_ptr,
                   (void*)mid_block_ptr, (void*)end_block_ptr);
        if (!allocs) return false;

        assert(ptr >= block_ptr);
        bool destroyed = false;
        if (!allocs->left && !allocs->right) {
            // We got to the leaf.
            fmt::print("do_free(leaf)\n");
            destroyed = true;
        } else if (ptr < mid_block_ptr) {
            // Go left.
            fmt::print("do_free(left)\n", label, next_order, (void*)ptr,
                       (void*)block_ptr);

            destroyed =
                do_free(allocs->left, next_order - 1, ptr, block_ptr, label);
        } else {
            // Go right.
            fmt::print("do_free(right)\n", label, next_order, (void*)ptr,
                       (void*)block_ptr);
            destroyed = do_free(allocs->right, next_order - 1, ptr,
                                mid_block_ptr, label);
        }

        if (destroyed) {
            if (!allocs->left && !allocs->right) {
                fmt::print("do_free(destroy)\n");
                allocs = {};
            }
        }
        return destroyed;
    }

   private:
    allocations_ptr _allocs;
    std::vector<std::byte> _data;
};

int main() {
    buddy<(1 << 16), (1 << 20)> mem(1 << 20);
    mem.print(mem.base_pointer());

    auto* a = mem.allocate(34'000, "A");
    // mem.print(a);
    auto* b = mem.allocate(66'000, "B");
    // mem.print(b);
    auto* c = mem.allocate(35'000, "C");
    // mem.print(c);
    auto* d = mem.allocate(67'000, "D");
    mem.print(d);

    mem.free(b, "B");
    mem.print(b);
    mem.free(d, "D");
    mem.print(d);
    mem.free(a, "A");
    mem.print(a);
    mem.free(c, "C");
    mem.print(c);

    mem.destroy();
    mem.print();
}
