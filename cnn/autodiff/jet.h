/*  ---------------------------------------------------------------------
  Copyright 2018-2019 Fangjun Kuang
  email: csukuangfj at gmail dot com
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a COPYING file of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>
  -----------------------------------------------------------------  */

#ifndef CNN_AUTODIFF_JET_H_
#define CNN_AUTODIFF_JET_H_

/*
 * autodiff.
 *
 *
 * we take the idea from ceres-solver for automatic differentiation.
 *
 * It is mainly used for verifying the back propagation step.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include "glog/logging.h"

namespace cnn {

template <typename Dtype>
class ArrayWithOp {
 public:
  explicit ArrayWithOp(int n) : v_(n) {}
  int n() const { return static_cast<int>(v_.size()); }

  Dtype operator[](int i) const { return v_[i]; }
  Dtype& operator[](int i) { return v_[i]; }

  bool has_same_shape(const ArrayWithOp& other) const {
    return v_.size() == other.v_.size();
  }
  Dtype at(int i) const { return v_.at(i); }
  Dtype& at(int i) { return v_.at(i); }

 private:
  std::vector<Dtype> v_;
};

template <typename Dtype>
ArrayWithOp<Dtype> operator+(const ArrayWithOp<Dtype>& a,
                             const ArrayWithOp<Dtype>& b) {
  CHECK(a.has_same_shape(b));
  ArrayWithOp<Dtype> c(a.n());
  for (int i = 0; i < a.n(); ++i) {
    c[i] = a[i] + b[i];
  }
  return c;
}

template <typename Dtype>
ArrayWithOp<Dtype> operator-(const ArrayWithOp<Dtype>& a,
                             const ArrayWithOp<Dtype>& b) {
  CHECK(a.has_same_shape(b));
  ArrayWithOp<Dtype> c(a.n());
  for (int i = 0; i < a.n(); ++i) {
    c[i] = a[i] - b[i];
  }
  return c;
}

template <typename Dtype>
ArrayWithOp<Dtype> operator-(const ArrayWithOp<Dtype>& a) {
  ArrayWithOp<Dtype> c(a.n());
  for (int i = 0; i < a.n(); ++i) {
    c[i] = -a[i];
  }
  return c;
}

template <typename Dtype>
ArrayWithOp<Dtype> operator*(const ArrayWithOp<Dtype>& a, double s) {
  ArrayWithOp<Dtype> c(a.n());
  for (int i = 0; i < a.n(); ++i) {
    c[i] = a[i] * s;
  }
  return c;
}

template <typename Dtype>
ArrayWithOp<Dtype> operator*(double s, const ArrayWithOp<Dtype>& a) {
  ArrayWithOp<Dtype> c(a.n());
  for (int i = 0; i < a.n(); ++i) {
    c[i] = a[i] * s;
  }
  return c;
}

template <typename Dtype>
ArrayWithOp<Dtype> operator/(const ArrayWithOp<Dtype>& a, double s) {
  ArrayWithOp<Dtype> c(a.n());
  for (int i = 0; i < a.n(); ++i) {
    c[i] = a[i] / s;
  }
  return c;
}

/**@brief Jet for autodiff (forward mode).
 *
 * This class is based on
 * https://github.com/kashif/ceres-solver/blob/master/include/ceres/jet.h
 *
 * We use the same name `Jet` as in ceres-solver.
 */
struct Dim {
  explicit Dim(int num) : n(num) {}
  int n;
};

template <typename Dtype>
class Jet {
 public:
  using type = Dtype;
  /** @brief The default constructor.
   *
   * Both the value and the gradient are set to 0.
   */
  explicit Jet(Dim dim) : val_(), grad_(dim.n) {}

  /** @brief Conversion from a scalar
   *
   * We set the gradient to 0 since we assume the scalar is
   * a constant and the derivative with respect
   * to a constant is 0.
   *
   * @param val The value of the variable.
   *
   */
  Jet(Dim dim, const Dtype& val) : val_(val), grad_(dim.n) {}

  Jet& operator=(const Dtype& val) {
    val_ = val;
    grad_.assign(grad_.size(), 0);
  }

  /** @brief Construct with a value and its derivative.
   *
   * @param val the value of variable
   * @param i the position to set the derivative
   * @param derivative the derivative of the variable
   */
  Jet(Dim dim, const Dtype& val, int i, Dtype derivative = 1)
      : val_(val), grad_(dim.n) {
    grad_.at(i) = derivative;
  }

  /** @brief Set the value and its derivative.
   *
   * @param val the value of the variable
   * @param i the position to set the derivative
   * @param derviative the derivative of the variable
   */
  void set(const Dtype& val, int i, Dtype derivative = 1) {
    val_ = val;
    grad_.assign(grad_.size(), 0);
    grad_.at(i) = derivative;
  }

  /** @brief For debug purpose only.
   * @return a string representation of the jet.
   */
  std::string to_string() const {
    std::ostringstream ss;
    ss << "[" << val_ << ", (";
    std::string sep;
    for (int i = 0; i < grad_.n(); ++i) {
      ss << sep << grad_[i];
      sep = ", ";
    }
    ss << ")]";
    ss << "\n";
    return ss.str();
  }

  Jet& operator+=(const Jet& f) { return *this = *this + f; }
  Jet& operator-=(const Jet& f) { return *this = *this - f; }
  Jet& operator*=(const Jet& f) { return *this = *this * f; }
  Jet& operator/=(const Jet& f) { return *this = *this / f; }

  Jet& operator+=(const Dtype& s) { return *this = *this + s; }
  Jet& operator-=(const Dtype& s) { return *this = *this - s; }
  Jet& operator*=(const Dtype& s) { return *this = *this * s; }
  Jet& operator/=(const Dtype& s) { return *this = *this / s; }

  /** @brief Get the value of the jet.
   *
   * @return the value of the jet.
   */
  operator Dtype() const { return val_; }

  bool has_same_shape(const Jet& other) const {
    return grad_.has_same_shape(other.grad_);
  }

  Dim dim() const { return Dim(grad_.n()); }

  Dtype val_;                //!< value
  ArrayWithOp<Dtype> grad_;  //!< gradient
};

template <typename Dtype>
std::ostream& operator<<(std::ostream& os, const Jet<Dtype>& f) {
  os << f.to_string();
  return os;
}

/** @brief Negate a jet.
 *
 * @param f the input jet
 * @return a jet with (-value, -gradient) of the input jet
 */
template <typename Dtype>
Jet<Dtype> operator-(const Jet<Dtype>& f) {
  Jet<Dtype> res(f.dim());
  res.val_ = -f.val_;
  res.grad_ = -f.grad_;
  return res;
}

//----------------------------------------
//  scalars
//  +, -, *, /
//----------------------------------------

/** @brief Jet + scalar
 *
 * @param f the input jet
 * @param s the scalar
 *
 * @note the gradient is not changed and only the value is
 * increased by `s`.
 *
 * @return a jet with (s + value, gradient)
 */
template <typename Dtype>
Jet<Dtype> operator+(const Jet<Dtype>& f, Dtype s) {
  Jet<Dtype> res(f);
  res.val_ += s;
  return res;
}

/** @brief scalar + jet
 *
 * @param s the scalar
 * @param f the in jet
 *
 * @note the gradient is not changed and only the value
 * is increased by `s`.
 *
 * @return a jet with (s + value, gradient)
 */
template <typename Dtype>
Jet<Dtype> operator+(Dtype s, const Jet<Dtype>& f) {
  Jet<Dtype> res(f);
  res.val_ += s;
  return res;
}

/** @brief jet - scalar
 *
 * @param f the input jet
 * @param s the scalar
 *
 * @note the gradient is not changed
 *
 * @return a jet with (val - s, gradient)
 *
 */
template <typename Dtype>
Jet<Dtype> operator-(const Jet<Dtype>& f, Dtype s) {
  Jet<Dtype> res(f);
  res.val_ -= s;
  return res;
}

/** @brief scalar - jet
 *
 * @param s the scalar
 * @param f the input jet
 *
 * @note the gradient is negated
 *
 * @return a jet with (s - value, -gradient)
 */
template <typename Dtype>
Jet<Dtype> operator-(Dtype s, const Jet<Dtype>& f) {
  Jet<Dtype> res(f.dim());
  res.val_ = s - f.val_;
  res.grad_ = -f.grad_;
  return res;
}

/** @brief jet * scalar
 *
 * @param f the input jet
 * @param s the scalar
 *
 * @note the gradient is scaled by s
 *
 * @return a jet with (s * value, s * gradient)
 */
template <typename Dtype>
Jet<Dtype> operator*(const Jet<Dtype>& f, Dtype s) {
  Jet<Dtype> res(f.dim());
  res.val_ = f.val_ * s;
  res.grad_ = f.grad_ * s;
  return res;
}

/** @brief scalar * jet
 *
 * @param s the scalar
 * @param f the input jet
 *
 * @note the gradient is scaled by s
 *
 * @return a jet with (s * value, s * gradient)
 *
 */
template <typename Dtype>
Jet<Dtype> operator*(Dtype s, const Jet<Dtype>& f) {
  Jet<Dtype> res(f.dim());
  res.val_ = f.val_ * s;
  res.grad_ = f.grad_ * s;
  return res;
}

/** @brief jet / s
 *
 * @param f the input jet
 * @param s the scalar
 *
 * @note the gradient is scaled by (1/s)
 *
 * @return a jet with (value/s, gradient/s)
 */
template <typename Dtype>
Jet<Dtype> operator/(const Jet<Dtype>& f, Dtype s) {
  Jet<Dtype> res(f.dim());
  res.val_ = f.val_ / s;
  res.grad_ = f.grad_ / s;
  return res;
}

/** @brief \f$\frac{s}{jet}\f$
 * \f[
 * \frac{s}{x + \epsilon\cdot g} = \frac{s(x - \epsilon\cdot g)}{(x +
 * \epsilon\cdot g)(x - \epsilon\cdot g)} = \frac{s \cdot x - \epsilon\cdot s
 * g}{x^2 - \epsilon^2\cdot g^2}
 * \f]
 *
 * Since \f$\epsilon^2 == 0\f$, we have
 *
 * \f[
 * \frac{s}{x + \epsilon \cdot g} = \frac{s \cdot x - \epsilon \cdot sg}{x^2}
 * = \frac{s}{x} - \epsilon \frac{sg}{x^2}
 * \f]
 *
 * Therefore, we should return a jet with \f$(\frac{s}{\mathrm{value}},
 * \frac{s}{\mathrm{value}^2}\cdot \mathrm{gradient})\f$
 *
 * @return a jet with \f$(\frac{s}{\mathrm{value}},
 * \frac{s}{\mathrm{value}^2}\cdot \mathrm{gradient})\f$
 */
template <typename Dtype>
Jet<Dtype> operator/(Dtype s, const Jet<Dtype>& f) {
  Jet<Dtype> res(f.dim());
  res.val_ = s / f.val_;
  res.grad_ = -s * f.grad_ / (f.val_ * f.val_);
  return res;
}

/** @brief jet + jet
 *
 * @param f the first jet
 * @param g the second jet
 *
 * @return (f.val + g.val, f.grad + g.grad)
 */
template <typename Dtype>
Jet<Dtype> operator+(const Jet<Dtype>& f, const Jet<Dtype>& g) {
  Jet<Dtype> res(f.dim());
  res.val_ = f.val_ + g.val_;
  res.grad_ = f.grad_ + g.grad_;  // it will check the shape internally
  return res;
}

/** @brief jet - jet
 *
 * @param f the first jet
 * @param g the second jet
 *
 * @return (f.val - g.val, f.grad - g.grad)
 */
template <typename Dtype>
Jet<Dtype> operator-(const Jet<Dtype>& f, const Jet<Dtype>& g) {
  Jet<Dtype> res(f.dim());
  res.val_ = f.val_ - g.val_;
  res.grad_ = f.grad_ - g.grad_;
  return res;
}
/** @bref jet * jet
 *
 * @param f the first jet
 * @param g the second jet
 *
 * \f[
 * (x + \epsilon f_x)\cdot(y + \epsilon f_y) =
 * xy + \epsilon (x\cdot f_y + y\cdot f_x) + \epsilon^2 (f_x \cdot f_y)
 * \f]
 *
 * Since \f$\epsilon^2 == 0\f$, we have
 *
 * \f[
 * (x + \epsilon f_x)\cdot(y + \epsilon f_y) =
 * xy + \epsilon (x\cdot f_y + y\cdot f_x)
 * \f]
 *
 * So we should return a jet \f$(\mathrm{f.val} \times \mathrm{g.val},
 * x\times\mathrm{g.grad} + y\times\mathrm{f.grad})\f$
 *
 * @return a jet \f$(\mathrm{f.val} \times\mathrm{g.val},
 * \mathrm{f.val}\times\mathrm{g.grad} + \mathrm{g.val}\times\mathrm{f.grad})\f$
 */
template <typename Dtype>
Jet<Dtype> operator*(const Jet<Dtype>& f, const Jet<Dtype>& g) {
  Jet<Dtype> res(f.dim());
  res.val_ = f.val_ * g.val_;
  res.grad_ = f.val_ * g.grad_ + g.val_ * f.grad_;
  return res;
}

/** @brief \frac{jet}{jet}
 *
 * @param f the first jet (numerator)
 * @param g the second jet (denominator)
 *
 * \f[
 * \frac{x + \epsilon \cdot f_x}{y + \epsilon \cdot f_y}
 * =\frac{(x+\epsilon \cdot f_x)(y - \epsilon \cdot f_y)}{(y+\epsilon\cdot
 * f_y)(y-\epsilon\cdot f_y)}
 * = \frac{x\cdot y + \epsilon(y\cdot f_x - x\cdot f_y) - \epsilon^2 f_x
 * f_y}{y^2 - \epsilon^2 f_y^2}
 * \f]
 *
 * Since \f$\epsilon^2 == 0\f$, we have
 * \f[
 * \frac{x + \epsilon \cdot f_x}{y + \epsilon \cdot f_y}
 * = \frac{x\cdot y + \epsilon(y\cdot f_x - x\cdot f_y)}{y^2}
 * = \frac{x}{y} + \epsilon (\frac{f_x}{y} - \frac{x \cdot f_y}{y^2})
 * \f]
 *
 * @return a jet \f$(\frac{\mathrm{f.val}}{\mathrm{g.val}},
 * \frac{\mathrm{f.grad}}{\mathrm{g.val}} -
 * \frac{\mathrm{f.val}\times\mathrm{g.grad}}{\mathrm{g.val}^2})\f$
 */
template <typename Dtype>
Jet<Dtype> operator/(const Jet<Dtype>& f, const Jet<Dtype>& g) {
  Jet<Dtype> res(f.dim());
  res.val_ = f.val_ / g.val_;
  res.grad_ = f.grad_ / g.val_ - (f.val_ * g.grad_) / (g.val_ * g.val_);
  return res;
}

/** @brief compare two jets: jet1 == jet2
 *
 * @param f the first jet
 * @param g the second jet
 *
 * @warning We only consider the value; the gradient of the jet
 * is not considered.
 *
 * @return true if the values of the two jet are equal; false otherwise
 */
template <typename Dtype>
bool operator==(const Jet<Dtype>& f, const Jet<Dtype>& g) {
  CHECK(f.has_same_shape(g));
  return f.val_ == g.val_;
}

/** @brief Compare two jets: jet1 != jet2
 *
 * @param f the first jet
 * @param g the second jet
 *
 * @warning We only consider the value; the gradient of the jet
 * is not considered.
 *
 * @return true if the values of the two jet are **NOT** equal; false otherwise
 */
template <typename Dtype>
bool operator!=(const Jet<Dtype>& f, const Jet<Dtype>& g) {
  CHECK(f.has_same_shape(g));
  return f.val_ != g.val_;
}

/** @brief Compare two jets: jet1 < jet2
 *
 * @param f the first jet
 * @param g the second jet
 *
 * @warning We only consider the value; the gradient of the jet
 * is not considered.
 *
 * @return true if f.val < g.val; false otherwise
 */
template <typename Dtype>
bool operator<(const Jet<Dtype>& f, const Jet<Dtype>& g) {
  CHECK(f.has_same_shape(g));
  return f.val_ < g.val_;
}

/** @brief Compare two jets: jet1 <= jet2
 *
 * @param f the first jet
 * @param g the second jet
 *
 * @warning We only consider the value; the gradient of the jet
 * is not considered.
 *
 * @return true if f.val <= g.val; false otherwise
 */
template <typename Dtype>
bool operator<=(const Jet<Dtype>& f, const Jet<Dtype>& g) {
  CHECK(f.has_same_shape(g));
  return f.val_ <= g.val_;
}

/** @brief Compare two jets: jet1 > jet2
 *
 * @param f the first jet
 * @param g the second jet
 *
 * @warning We only consider the value; the gradient of the jet
 * is not considered.
 *
 * @return true if f.val > g.val; false otherwise
 */
template <typename Dtype>
bool operator>(const Jet<Dtype>& f, const Jet<Dtype>& g) {
  return f.val_ > g.val_;
}

/** @brief Compare two jets: jet1 >= jet2
 *
 * @param f the first jet
 * @param g the second jet
 *
 * @warning We only consider the value; the gradient of the jet
 * is not considered.
 *
 * @return true if f.val >= g.val; false otherwise
 */
template <typename Dtype>
bool operator>=(const Jet<Dtype>& f, const Jet<Dtype>& g) {
  CHECK(f.has_same_shape(g));
  return f.val_ >= g.val_;
}

//--------------------
//  math functions
//      std::exp
//      std::log
//      std::max
//      std::sqrt
//--------------------
using std::max;

/** @brief max(jet1, jet2)
 *
 * @param f the first jet
 * @param g the second jet
 *
 * @warning It returns by value!
 *
 * @return g if f.val < g.val; f otherwise
 */
template <typename Dtype>
Jet<Dtype> max(const Jet<Dtype>& f, const Jet<Dtype>& g) {
  return (f < g) ? g : f;
}

using std::exp;

/** @brief exp(jet)
 *
 * Assume \f$f = x + \epsilon \cdot f_x\f$, then the derivative
 * of \f$\mathrm{e}^f\f$ with respect to x is \f$\mathrm{e}^f \cdot f_x\f$.
 *
 * Thus, we should return a jet (exp(value), exp(value)*grad)
 *
 * @param f the input jet
 *
 * @return a jet with (exp(value), exp(value)*grad)
 */
template <typename Dtype>
Jet<Dtype> exp(const Jet<Dtype>& f) {
  Jet<Dtype> res(f.dim());

  auto s = exp(f.val_);
  res.val_ = s;
  res.grad_ = s * f.grad_;

  return res;
}

using std::log;

/** @brief log(jet)
 *
 * Assume \f$f = x + \epsilon \cdot f_x\f$, then the derivative
 * of \f$\mathrm{log}(f)\f$ with respect to x is \f$\frac{1}{f} \cdot f_x\f$.
 *
 * Thus, we should return a jet (log(value),
 * \f$\frac{1}{\mathrm{value}}\f$*grad)
 *
 * @return a jet (log(value),
 * \f$\frac{1}{\mathrm{value}}\f$*grad)
 */
template <typename Dtype>
Jet<Dtype> log(const Jet<Dtype>& f) {
  Jet<Dtype> res(f.dim());

  res.val_ = log(f.val_);
  res.grad_ = f.grad_ / f.val_;

  return res;
}

using std::sqrt;

/** @brief sqrt(jet)
 *
 * Assume \f$f = x + \epsilon \cdot f_x\f$, then the derivative
 * of \f$\sqrt{f}\f$ with respect to x is \f$\frac{1}{2\sqrt{f}} \cdot f_x\f$.
 *
 * Thus, we should return a jet \f$(\sqrt{\mathrm{value}},
 * \frac{1}{\sqrt{\mathrm{value}}}\f$*grad)
 *
 * @return a jet \f$(\sqrt{\mathrm{value}},
 * \frac{1}{\sqrt{\mathrm{value}}}\f$*grad)
 */
template <typename Dtype>
Jet<Dtype> sqrt(const Jet<Dtype>& f) {
  Jet<Dtype> res(f.dim());

  res.val_ = sqrt(f.val_);
  res.grad_ = f.grad_ / (Dtype(2) * res.val_);

  return res;
}

}  // namespace cnn
#endif  // CNN_AUTODIFF_JET_H_
