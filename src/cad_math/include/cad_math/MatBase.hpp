//
// Created by rdkgsk on 3/1/26.
//

#ifndef CAD_MATBASE_H
#define CAD_MATBASE_H

#include <cstddef>
#include <cmath>
#include <optional>

#include "Common.hpp"

namespace cadm {
    template <typename T, std::size_t C, typename RowType, typename Derived>
    struct MatRowRef {
        Derived &matrix;
        std::size_t rowIdx;

        constexpr MatRowRef(Derived &mat, const std::size_t idx) noexcept
        : matrix(mat),
          rowIdx(idx) {}

        constexpr T& operator[](std::size_t col) noexcept {
            return matrix(rowIdx, col);
        }

        constexpr const T& operator[](std::size_t col) const noexcept {
            return matrix(rowIdx, col);
        }

        explicit constexpr operator RowType() const noexcept {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k) {
                res[k] = (*this)[k];
            }
            return res;
        }

        constexpr MatRowRef& operator=(const RowType &vec) noexcept {
            for (std::size_t k = 0; k < C; ++k) {
                (*this)[k] = vec[k];
            }
            return *this;
        }

        constexpr void swap(MatRowRef other) noexcept {
            for (std::size_t k = 0; k < C; ++k) {
                std::swap((*this)[k], other[k]);
            }
        }

        // ===== compound assignment operators (modify in-place) =====

        constexpr MatRowRef& operator+=(const RowType &vec) noexcept {
            for (std::size_t k = 0; k < C; ++k) {
                (*this)[k] += vec[k];
            }
            return *this;
        }

        constexpr MatRowRef& operator+=(const MatRowRef &other) noexcept {
            for (std::size_t k = 0; k < C; ++k) {
                (*this)[k] += other[k];
            }
            return *this;
        }

        constexpr MatRowRef& operator-=(const RowType &vec) noexcept {
            for (std::size_t k = 0; k < C; ++k) {
                (*this)[k] -= vec[k];
            }
            return *this;
        }

        constexpr MatRowRef& operator-=(const MatRowRef &other) noexcept {
            for (std::size_t k = 0; k < C; ++k) {
                (*this)[k] -= other[k];
            }
            return *this;
        }

        constexpr MatRowRef& operator*=(T scalar) noexcept {
            for (std::size_t k = 0; k < C; ++k) {
                (*this)[k] *= scalar;
            }
            return *this;
        }

        constexpr MatRowRef& operator/=(T scalar) noexcept {
            for (std::size_t k = 0; k < C; ++k) {
                (*this)[k] /= scalar;
            }
            return *this;
        }

        // ===== binary operators (return new vector, don't modify) =====

        constexpr RowType operator+(const RowType &vec) const noexcept {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k) {
                res[k] = (*this)[k] + vec[k];
            }
            return res;
        }

        constexpr RowType operator+(const MatRowRef &other) const noexcept {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k) {
                res[k] = (*this)[k] + other[k];
            }
            return res;
        }

        constexpr RowType operator-(const RowType &vec) const noexcept {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k) {
                res[k] = (*this)[k] - vec[k];
            }
            return res;
        }

        constexpr RowType operator-(const MatRowRef &other) const noexcept {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k) {
                res[k] = (*this)[k] - other[k];
            }
            return res;
        }

        constexpr RowType operator*(T scalar) const noexcept {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k) {
                res[k] = (*this)[k] * scalar;
            }
            return res;
        }

        friend constexpr RowType operator*(T scalar, const MatRowRef &row) noexcept {
            return row * scalar;
        }

        constexpr RowType operator/(T scalar) const noexcept {
            RowType res{};
            for (std::size_t k = 0; k < C; ++k) {
                res[k] = (*this)[k] / scalar;
            }
            return res;
        }
    };

    template <typename T, std::size_t R, std::size_t C,
              typename RowType,
              typename ColType,
              template <typename, std::size_t, std::size_t> class MatrixT,
              typename Derived>
    struct MatBase {
        using ValueType = T;

        constexpr ColType& col(const std::size_t i) noexcept {
            return static_cast<Derived*>(this)->columns[i];
        }

        constexpr const ColType& col(const std::size_t i) const noexcept {
            return static_cast<const Derived*>(this)->columns[i];
        }

        constexpr RowType row(const std::size_t i) const noexcept {
            RowType res{};
            for (std::size_t j = 0; j < C; ++j) {
                res[j] = (*this)(i, j);
            }
            return res;
        }

        constexpr T& operator()(const std::size_t row, const std::size_t col) noexcept {
            return this->col(col)[row];
        }

        constexpr const T& operator()(const std::size_t row, const std::size_t col) const noexcept {
            return this->col(col)[row];
        }

        friend constexpr Derived operator-(Derived lhs) {
            for (std::size_t i = 0; i < C; ++i) {
                lhs.col(i) = -lhs.col(i);
            }
            return lhs;
        }

        friend constexpr bool operator==(const Derived &lhs, const Derived &rhs) {
            for (std::size_t i = 0; i < C; ++i) {
                if (lhs.col(i) != rhs.col(i)) {
                    return false;
                }
            }
            return true;
        }

        friend constexpr Derived operator+(Derived lhs, const Derived &rhs) {
            for (std::size_t i = 0; i < C; ++i) {
                lhs.col(i) += rhs.col(i);
            }
            return lhs;
        }

        friend constexpr Derived operator-(Derived lhs, const Derived &rhs) {
            for (std::size_t i = 0; i < C; ++i) {
                lhs.col(i) -= rhs.col(i);
            }
            return lhs;
        }

        template <typename OtherDerived, typename OtherCol, typename OtherRow, std::size_t OtherC>
        constexpr auto operator*(const MatBase<T, C, OtherC, OtherCol, OtherRow, MatrixT, OtherDerived> &rhs) const {
            using ResultType = MatrixT<T, R, OtherC>;
            ResultType m{};

            for (std::size_t i = 0; i < R; ++i) {
                const auto r = this->row(i);
                for (std::size_t j = 0; j < OtherC; ++j) {
                    m(i, j) = r.dot(rhs.col(j));
                }
            }
            return m;
        }

        constexpr ColType operator*(const RowType &v) const noexcept {
            ColType res = this->col(0) * v[0];
            for (std::size_t j = 1; j < C; ++j) {
                res += this->col(j) * v[j];
            }
            return res;
        }

        constexpr void transpose() requires (R == C) {
            for (std::size_t i = 0; i < R; ++i) {
                for (std::size_t j = i + 1; j < C; ++j) {
                    std::swap((*this)(i, j), (*this)(j, i));
                }
            }
        }

        [[nodiscard]] constexpr auto transposed() const {
            using ResultType = MatrixT<T, C, R>;
            ResultType result{};
            for (std::size_t i = 0; i < R; ++i) {
                result.col(i) = this->row(i);
            }
            return result;
        }

        /// @brief Modified Gram-Schmidt orthonormalization of columns
        [[nodiscard]] Derived orthonormalized() const requires (R == C) {
            // https://www.math.uci.edu/~ttrogdon/105A/html/Lecture23.html
            Derived v = *static_cast<const Derived*>(this);
            for (std::size_t j = 0; j < C; ++j) {
                const auto qj = v.col(j).normalized();
                v.col(j) = qj;
                for (std::size_t k = j + 1; k < C; ++k) {
                    v.col(k) -= qj.dot(v.col(k)) * qj;
                }
            }
            return v;
        }

        [[nodiscard]] constexpr Derived normalizedColumns() const noexcept {
            Derived result = *static_cast<const Derived*>(this);
            for (std::size_t i = 0; i < C; ++i) {
                result.col(i) = result.col(i).normalized();
            }
            return result;
        }

        constexpr auto makeRowRef(std::size_t rowIdx) noexcept {
            return MatRowRef<T, C, RowType, Derived>(static_cast<Derived&>(*this), rowIdx);
        }

        [[nodiscard]] std::size_t findPivotGepp(const std::size_t i) const {
            std::size_t pivot = i;
            for (std::size_t j = i + 1; j < R; ++j) {
                if (std::abs((*this)(j, i)) > std::abs((*this)(pivot, i))) {
                    pivot = j;
                }
            }
            return pivot;
        }

        void swapRows(const std::size_t i, const std::size_t j) {
            auto tempRowI = makeRowRef(i);
            auto tempRowPivot = makeRowRef(j);
            tempRowI.swap(tempRowPivot);
        }

        [[deprecated("inverse is not needed for this project")]]
        [[nodiscard]] constexpr std::optional<Derived> inversedSafe() const requires (R == C) {
            Derived temp = *static_cast<const Derived*>(this);
            Derived inv = Derived::identity();

            for (std::size_t i = 0; i < R; ++i) {
                std::size_t pivot = temp.findPivotGepp(i);

                // check if the matrix is singular
                if (std::abs(temp(pivot, i)) < gc_eps) {
                    return std::nullopt;
                }

                if (pivot != i) {
                    temp.swapRows(i, pivot);
                    inv.swapRows(i, pivot);
                }

                // normalize row i
                T div = temp(i, i);
                auto tempRowI = temp.makeRowRef(i);
                auto invRowI = inv.makeRowRef(i);
                tempRowI /= div;
                invRowI /= div;

                // eliminate other rows
                for (std::size_t j = 0; j < R; ++j) {
                    if (i != j) {
                        T mul = temp(j, i);
                        temp.makeRowRef(j) -= tempRowI * mul;
                        inv.makeRowRef(j) -= invRowI * mul;
                    }
                }
            }

            return inv;
        }

        [[deprecated("inverse is not needed for this project")]]
        [[nodiscard]] constexpr Derived inversed() const requires (R == C) {
            // ReSharper disable once CppDeprecatedEntity
            if (const auto safeInverse = inversedSafe()) {
                return safeInverse.value();
            }
            return Derived::identity();
        }
    };
}

#endif //CAD_MATBASE_H
