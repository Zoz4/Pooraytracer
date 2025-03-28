#pragma once

namespace Pooraytracer {

	// Implemented by PBRT
	template <typename T>
	struct Complex {
		Complex(T re) : re(re), im(0) {}
		Complex(T re, T im) : re(re), im(im) {}

		Complex operator-() const { return { -re, -im }; }

		Complex operator+(Complex z) const { return { re + z.re, im + z.im }; }

		Complex operator-(Complex z) const { return { re - z.re, im - z.im }; }

		Complex operator*(Complex z) const {
			return { re * z.re - im * z.im, re * z.im + im * z.re };
		}

		Complex operator/(Complex z) const {
			T scale = 1 / (z.re * z.re + z.im * z.im);
			return { scale * (re * z.re + im * z.im), scale * (im * z.re - re * z.im) };
		}

		friend Complex operator+(T value, Complex z) {
			return Complex(value) + z;
		}

		friend Complex operator-(T value, Complex z) {
			return Complex(value) - z;
		}

		friend Complex operator*(T value, Complex z) {
			return Complex(value) * z;
		}

		friend Complex operator/(T value, Complex z) {
			return Complex(value) / z;
		}

		T re, im;
	};

	template <typename T>
	T Norm(const Complex<T>& z) {
		return z.re * z.re + z.im * z.im;
	}
	template <typename T>
	T Abs(const Complex<T>& z) {
		return std::sqrt(Norm(z));
	}
	template <typename T>
	Complex<T> Sqrt(const Complex<T>& z) {
		T n = Abs(z), t1 = std::sqrt(T(.5) * (n + std::fabs(z.re))),
			t2 = T(.5) * z.im / t1;

		if (n == 0)
			return 0;

		if (z.re >= 0)
			return { t1, t2 };
		else
			return { std::fabs(t2), std::copysign(t1, z.im) };
	}

	inline constexpr double Clamp(double val, double low, double high) {
		if (val < low)       return low;
		else if (val > high) return high;
		else                 return val;
	}
	template <typename T>
	constexpr T Sqr(T v) { return v * v; }

	inline double CosTheta(const vec3& w) { return w.z; }
	inline double Cos2Theta(const vec3& w) { return Sqr(w.z); }
	inline double Sin2Theta(const vec3& w) { return std::max<double>(0., 1 - Cos2Theta(w)); }
	inline double Tan2Theta(const vec3& w) {
		return Sin2Theta(w) / Cos2Theta(w);
	}
	inline double SinTheta(const vec3& w) { return std::sqrt(Sin2Theta(w)); }
	inline double AbsCosTheta(const vec3& w) { return std::abs(w.z); }
	inline bool IsInf(double v) { return std::isinf(v); }
	inline double CosPhi(const vec3& w) {
		double sinTheta = SinTheta(w);
		return (sinTheta == 0) ? 1 : Clamp(w.x / sinTheta, -1, 1);
	}
	inline double SinPhi(const vec3& w) {
		double sinTheta = SinTheta(w);
		return (sinTheta == 0) ? 0 : Clamp(w.y / sinTheta, -1, 1);
	}
	inline double AbsDot(const vec3& v1, const vec3& v2) { return std::fabs(glm::dot(v1, v2)); }
	inline double Lerp(double x, double a, double b) {
		return (1 - x) * a + x * b;
	}
	inline double LengthSquared(const vec2& v) { return Sqr(v.x) + Sqr(v.y); }
	inline double LengthSquared(const vec3& v) { return Sqr(v.x) + Sqr(v.y) + Sqr(v.z); }


	inline double FrComplex(double cosTheta_i, Complex<double> eta) {
		using Complex = Complex<double>;
		cosTheta_i = Clamp(cosTheta_i, 0, 1);
		// Compute complex $\cos\,\theta_\roman{t}$ for Fresnel equations using Snell's law
		double sin2Theta_i = 1 - Sqr(cosTheta_i);
		Complex sin2Theta_t = sin2Theta_i / Sqr(eta);
		Complex cosTheta_t = Sqrt(1 - sin2Theta_t);

		Complex r_parl = (eta * cosTheta_i - cosTheta_t) / (eta * cosTheta_i + cosTheta_t);
		Complex r_perp = (cosTheta_i - eta * cosTheta_t) / (cosTheta_i + eta * cosTheta_t);
		return (Norm(r_parl) + Norm(r_perp)) / 2;
	}

	inline bool SameHemisphere(const vec3& w, const vec3& wp) {
		return w.z * wp.z > 0;
	}
	inline vec3 FaceForward(const vec3& v, const vec3& n2) {
		return (glm::dot(v, n2) < 0.) ? -v : v;
	}
}