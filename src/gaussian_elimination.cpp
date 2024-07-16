#include <fmt/format.h>

#include <array>
#include <cassert>
#include <limits>
#include <vector>

template <typename T>
using matrix = std::vector<std::vector<T>>;

template <typename T>
void print(const matrix<T>& m) {
    fmt::print("[\n");
    for (const auto& r : m) {
        for (const auto& c : r) {
            fmt::print("{}, ", c);
        }
        fmt::print("\n");
    }
    fmt::print("]\n");
}

template <typename T>
size_t find_argmax_abs(const matrix<T>& A, size_t r_min, size_t col) {
    const auto row_count = A.size();
    assert(r_min < row_count);

    size_t i_max = r_min;
    T max = A[r_min][col];
    for (size_t i = i_max + 1; i < row_count; ++i) {
        using std::abs;
        if (abs(A[i][col]) > abs(A[i_max][col])) {
            // New max found
            i_max = i;
        }
    }
    return i_max;
}

template <typename T>
void gaussian_elimination(matrix<T>& A) {
    const auto row_count = A.size();
    if (A.empty()) return;
    const auto col_count = A[0].size();
    size_t h = 0;  // row
    size_t k = 0;  // col

    while (h < row_count && k < col_count) {
        /* Find the k-th pivot: */
        // size_t i_max := argmax (i = h ... m, abs(A[i, k]))
        size_t i_max = find_argmax_abs(A, h, k);
        fmt::print("h = {}, k = {}, pivot = {}, A[pivot][k] = {}\n", h, k,
                   i_max, A[i_max][k]);
        if (A[i_max][k] == 0) {
            /* No pivot in this column, pass to next column */
            ++k;
        } else {
            // swap rows(h, i_max)
            using std::swap;
            swap(A[h], A[i_max]);

            /* Do for all rows below pivot: */
            for (size_t i = h + 1; i < row_count; ++i) {
                auto f = A[i][k] / A[h][k];
                /* Fill with zeros the lower part of pivot column: */
                A[i][k] = 0;
                /* Do for all remaining elements in current row: */
                for (size_t j = k + 1; j < col_count; ++j) {
                    A[i][j] -= A[h][j] * f;
                }
            }

            /* Increase pivot row and column */
            ++h;
            ++k;
        }
        print(A);
        fflush(stdout);
    }
}

int main() {
    matrix<int> m1 = {{
        {1, 0, 4, 2},
        {1, 2, 6, 2},
        {2, 0, 8, 8},
        {2, 1, 9, 4},
    }};

    print(m1);
    gaussian_elimination(m1);
    print(m1);
}
