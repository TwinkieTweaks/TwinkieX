#include "pch.h"
#include "Math.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include <cmath>
#include <format>
#include <limits>
#include <random>
#include <string>
#include <type_traits>

namespace Gecko::Exports::Math
{
	static void SetScriptException(const char* Message)
	{
		if (auto Ctx = asGetActiveContext()) Ctx->SetException(Message);
	}

	// Vectors (and quat) as arrays of scalars

	template<typename V> struct Traits;
	template<> struct Traits<vec2> { using Scalar = float;    static constexpr int Size = 2; };
	template<> struct Traits<vec3> { using Scalar = float;    static constexpr int Size = 3; };
	template<> struct Traits<vec4> { using Scalar = float;    static constexpr int Size = 4; };
	template<> struct Traits<int2> { using Scalar = int32_t;  static constexpr int Size = 2; };
	template<> struct Traits<int3> { using Scalar = int32_t;  static constexpr int Size = 3; };
	template<> struct Traits<nat2> { using Scalar = uint32_t; static constexpr int Size = 2; };
	template<> struct Traits<nat3> { using Scalar = uint32_t; static constexpr int Size = 3; };
	template<> struct Traits<quat> { using Scalar = float;    static constexpr int Size = 4; };

	template<typename V> using ScalarOf = typename Traits<V>::Scalar;
	template<typename V> constexpr int SizeOf = Traits<V>::Size;

	// The types are made of consecutive scalars, so they can be walked like arrays
	template<typename V> static ScalarOf<V>* Elems(V& Vec) { return reinterpret_cast<ScalarOf<V>*>(&Vec); }
	template<typename V> static const ScalarOf<V>* Elems(const V& Vec) { return reinterpret_cast<const ScalarOf<V>*>(&Vec); }

	enum class Op { Add, Sub, Mul, Div };

	// Returns false when an integer is divided by zero
	template<Op O, typename S>
	static bool Apply(S& A, S B)
	{
		if constexpr (std::is_integral_v<S>)
		{
			using U = std::make_unsigned_t<S>;

			// Wraps around on overflow, like the script's own integer math
			if constexpr (O == Op::Add) A = S(U(A) + U(B));
			else if constexpr (O == Op::Sub) A = S(U(A) - U(B));
			else if constexpr (O == Op::Mul) A = S(U(A) * U(B));
			else
			{
				if (B == 0) return false;

				// INT_MIN / -1 overflows, which traps on x86
				if constexpr (std::is_signed_v<S>)
				{
					if (B == S(-1))
					{
						A = S(U(0) - U(A));
						return true;
					}
				}
				A = S(A / B);
			}
		}
		else
		{
			if constexpr (O == Op::Add) A += B;
			else if constexpr (O == Op::Sub) A -= B;
			else if constexpr (O == Op::Mul) A *= B;
			else A /= B;
		}
		return true;
	}

	// Self op Other
	template<typename V, Op O>
	static V OpVec(const V& Other, const V& Self)
	{
		V Result = Self;
		for (int i = 0; i < SizeOf<V>; i++)
		{
			if (not Apply<O>(Elems(Result)[i], Elems(Other)[i]))
			{
				SetScriptException("Divide by zero");
				return Self;
			}
		}
		return Result;
	}

	// Self op Other
	template<typename V, Op O>
	static V OpScalar(ScalarOf<V> Other, const V& Self)
	{
		V Result = Self;
		for (int i = 0; i < SizeOf<V>; i++)
		{
			if (not Apply<O>(Elems(Result)[i], Other))
			{
				SetScriptException("Divide by zero");
				return Self;
			}
		}
		return Result;
	}

	// Other op Self
	template<typename V, Op O>
	static V OpScalarReversed(ScalarOf<V> Other, const V& Self)
	{
		V Result = Self;
		for (int i = 0; i < SizeOf<V>; i++)
		{
			ScalarOf<V> Value = Other;
			if (not Apply<O>(Value, Elems(Self)[i]))
			{
				SetScriptException("Divide by zero");
				return Self;
			}
			Elems(Result)[i] = Value;
		}
		return Result;
	}

	template<typename V, Op O>
	static V& OpAssignVec(const V& Other, V& Self)
	{
		Self = OpVec<V, O>(Other, Self);
		return Self;
	}

	template<typename V, Op O>
	static V& OpAssignScalar(ScalarOf<V> Other, V& Self)
	{
		Self = OpScalar<V, O>(Other, Self);
		return Self;
	}

	template<typename V>
	static ScalarOf<V> Index(int Idx, const V& Self)
	{
		if (Idx < 0 or Idx >= SizeOf<V>)
		{
			SetScriptException("Index out of range");
			return 0;
		}
		return Elems(Self)[Idx];
	}

	// Compares any of the types float by float (or int by int)
	template<typename T>
	static bool Equals(const T& Other, const T& Self)
	{
		using S = std::conditional_t<std::is_same_v<T, int2> or std::is_same_v<T, int3>, int32_t, std::conditional_t<std::is_same_v<T, nat2> or std::is_same_v<T, nat3>, uint32_t, float>>;
		auto A = reinterpret_cast<const S*>(&Self);
		auto B = reinterpret_cast<const S*>(&Other);
		for (size_t i = 0; i < sizeof(T) / sizeof(S); i++)
		{
			if (A[i] != B[i]) return false;
		}
		return true;
	}

	template<typename V>
	static std::string ToString(const V& Self)
	{
		std::string Out = "<";
		for (int i = 0; i < SizeOf<V>; i++)
		{
			if (i) Out += ", ";
			Out += std::format("{}", Elems(Self)[i]);
		}
		return Out + ">";
	}

	template<typename V>
	static float Dot(const V& A, const V& B)
	{
		float Sum = 0.0f;
		for (int i = 0; i < SizeOf<V>; i++) Sum += Elems(A)[i] * Elems(B)[i];
		return Sum;
	}

	template<typename V> static float LengthSquared(const V& Self) { return Dot(Self, Self); }
	template<typename V> static float Length(const V& Self) { return std::sqrt(LengthSquared(Self)); }

	// A zero vector stays zero instead of becoming NaN
	template<typename V>
	static V Normalized(const V& Self)
	{
		float Len = Length(Self);
		if (Len == 0.0f) return Self;
		return OpScalar<V, Op::Div>(Len, Self);
	}

	template<typename V> static V Sum(const V& A, const V& B) { return OpVec<V, Op::Add>(B, A); }
	template<typename V> static V Difference(const V& A, const V& B) { return OpVec<V, Op::Sub>(B, A); }
	template<typename V> static V Scaled(const V& A, float B) { return OpScalar<V, Op::Mul>(B, A); }

	static vec3 Cross(const vec3& A, const vec3& B)
	{
		return { A.y * B.z - A.z * B.y, A.z * B.x - A.x * B.z, A.x * B.y - A.y * B.x };
	}

	// Constructors write into memory the script engine allocated

	template<typename T> static void ConstructZero(T* Self) { *Self = T{}; }
	template<typename T> static void ConstructCopy(const T& Other, T* Self) { *Self = Other; }

	template<typename V>
	static void ConstructScalar(ScalarOf<V> Value, V* Self)
	{
		for (int i = 0; i < SizeOf<V>; i++) Elems(*Self)[i] = Value;
	}

	// ConstructFrom<vec3, float, float, float> is void(float, float, float, vec3*)
	template<typename T, typename... Args>
	static void ConstructFrom(Args... Values, T* Self)
	{
		*Self = T{ Values... };
	}

	static void Vec3ConstructXY(const vec2& XY, float Z, vec3* Self) { *Self = { XY.x, XY.y, Z }; }
	static void Vec4ConstructXYZ(const vec3& XYZ, float W, vec4* Self) { *Self = { XYZ.x, XYZ.y, XYZ.z, W }; }
	static void Vec4ConstructXYZW(const vec2& XY, const vec2& ZW, vec4* Self) { *Self = { XY.x, XY.y, ZW.x, ZW.y }; }
	static void Int3ConstructXY(const int2& XY, int32_t Z, int3* Self) { *Self = { XY.x, XY.y, Z }; }
	static void Nat3ConstructXY(const nat2& XY, uint32_t Z, nat3* Self) { *Self = { XY.x, XY.y, Z }; }

	static vec2 Vec3XY(const vec3& Self) { return { Self.x, Self.y }; }
	static vec2 Vec3YZ(const vec3& Self) { return { Self.y, Self.z }; }
	static vec2 Vec4XY(const vec4& Self) { return { Self.x, Self.y }; }
	static vec3 Vec4XYZ(const vec4& Self) { return { Self.x, Self.y, Self.z }; }
	static vec2 Vec4YZ(const vec4& Self) { return { Self.y, Self.z }; }
	static vec3 Vec4YZW(const vec4& Self) { return { Self.y, Self.z, Self.w }; }
	static vec2 Vec4ZW(const vec4& Self) { return { Self.z, Self.w }; }
	static int2 Int3XY(const int3& Self) { return { Self.x, Self.y }; }
	static nat2 Nat3XY(const nat3& Self) { return { Self.x, Self.y }; }

	// Matrices

	static mat3 Mat3Identity() { return { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } }; }
	static mat4 Mat4Identity() { return { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } }; }

	static vec3 Mat3MulVec3(const vec3& V, const mat3& Self) { return Sum(Sum(Scaled(Self.x, V.x), Scaled(Self.y, V.y)), Scaled(Self.z, V.z)); }
	static vec3 Mat3MulVec2(const vec2& V, const mat3& Self) { return Mat3MulVec3({ V.x, V.y, 1.0f }, Self); }
	static vec3 Vec3MulMat3(const vec3& V, const mat3& Self) { return { Dot(V, Self.x), Dot(V, Self.y), Dot(V, Self.z) }; }
	static vec3 Vec2MulMat3(const vec2& V, const mat3& Self) { return Vec3MulMat3({ V.x, V.y, 1.0f }, Self); }

	// Self * Other
	static mat3 Mat3MulMat3(const mat3& Other, const mat3& Self) { return { Mat3MulVec3(Other.x, Self), Mat3MulVec3(Other.y, Self), Mat3MulVec3(Other.z, Self) }; }
	static mat3& Mat3MulAssign(const mat3& Other, mat3& Self) { Self = Mat3MulMat3(Other, Self); return Self; }

	static vec4 Mat4MulVec4(const vec4& V, const mat4& Self) { return Sum(Sum(Scaled(Self.x, V.x), Scaled(Self.y, V.y)), Sum(Scaled(Self.z, V.z), Scaled(Self.t, V.w))); }
	static vec4 Mat4MulVec3(const vec3& V, const mat4& Self) { return Mat4MulVec4({ V.x, V.y, V.z, 1.0f }, Self); }
	static vec4 Vec4MulMat4(const vec4& V, const mat4& Self) { return { Dot(V, Self.x), Dot(V, Self.y), Dot(V, Self.z), Dot(V, Self.t) }; }
	static vec4 Vec3MulMat4(const vec3& V, const mat4& Self) { return Vec4MulMat4({ V.x, V.y, V.z, 1.0f }, Self); }

	// Self * Other
	static mat4 Mat4MulMat4(const mat4& Other, const mat4& Self) { return { Mat4MulVec4(Other.x, Self), Mat4MulVec4(Other.y, Self), Mat4MulVec4(Other.z, Self), Mat4MulVec4(Other.t, Self) }; }
	static mat4& Mat4MulAssign(const mat4& Other, mat4& Self) { Self = Mat4MulMat4(Other, Self); return Self; }

	static void Mat3ConstructIdentity(mat3* Self) { *Self = Mat3Identity(); }
	static void Mat3ConstructIso3(const iso3& Iso, mat3* Self) { *Self = { { Iso.xx, Iso.xy, 0 }, { Iso.yx, Iso.yy, 0 }, { Iso.tx, Iso.ty, 1 } }; }
	static void Mat3ConstructVecs(const vec3& V0, const vec3& V1, const vec3& V2, mat3* Self) { *Self = { V0, V1, V2 }; }
	static void Mat3ConstructMat4(const mat4& M, mat3* Self) { *Self = { Vec4XYZ(M.x), Vec4XYZ(M.y), Vec4XYZ(M.z) }; }

	static void Mat4ConstructIdentity(mat4* Self) { *Self = Mat4Identity(); }
	static void Mat4ConstructIso4(const iso4& Iso, mat4* Self) { *Self = { { Iso.xx, Iso.xy, Iso.xz, 0 }, { Iso.yx, Iso.yy, Iso.yz, 0 }, { Iso.zx, Iso.zy, Iso.zz, 0 }, { Iso.tx, Iso.ty, Iso.tz, 1 } }; }
	static void Mat4ConstructVecs(const vec4& V0, const vec4& V1, const vec4& V2, const vec4& V3, mat4* Self) { *Self = { V0, V1, V2, V3 }; }
	static void Mat4ConstructMat3(const mat3& M, mat4* Self) { *Self = { { M.x.x, M.x.y, M.x.z, 0 }, { M.y.x, M.y.y, M.y.z, 0 }, { M.z.x, M.z.y, M.z.z, 0 }, { 0, 0, 0, 1 } }; }

	static void Iso3ConstructIdentity(iso3* Self) { *Self = { 1, 0, 0, 1, 0, 0 }; }
	static void Iso3ConstructMat3(const mat3& M, iso3* Self) { *Self = { M.x.x, M.x.y, M.y.x, M.y.y, M.z.x, M.z.y }; }
	static void Iso3Translate(float X, float Y, iso3& Self) { Self.tx += X; Self.ty += Y; }

	static void Iso4ConstructIdentity(iso4* Self) { *Self = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 }; }
	static void Iso4ConstructMat4(const mat4& M, iso4* Self) { *Self = { M.x.x, M.x.y, M.x.z, M.y.x, M.y.y, M.y.z, M.z.x, M.z.y, M.z.z, M.t.x, M.t.y, M.t.z }; }
	static void Iso4Translate(float X, float Y, float Z, iso4& Self) { Self.tx += X; Self.ty += Y; Self.tz += Z; }

	// Getters such as mat4.xy and iso4.tz
	template<typename M, float M::*Field>
	static float GetField(const M& Self) { return Self.*Field; }

	template<typename M, typename V, V M::*Column, float V::*Row>
	static float GetElem(const M& Self) { return (Self.*Column).*Row; }

	// The inverses and transposes work on the flat floats, so they don't depend on rows vs. columns
	static mat3 Mat3Inverse(const mat3& Self)
	{
		auto M = [&](int Col, int Row) { return reinterpret_cast<const float*>(&Self)[Col * 3 + Row]; };
		float OneOverDet = 1.0f / (
			M(0, 0) * (M(1, 1) * M(2, 2) - M(2, 1) * M(1, 2)) -
			M(1, 0) * (M(0, 1) * M(2, 2) - M(2, 1) * M(0, 2)) +
			M(2, 0) * (M(0, 1) * M(1, 2) - M(1, 1) * M(0, 2)));

		mat3 Result;
		auto R = [&](int Col, int Row) -> float& { return reinterpret_cast<float*>(&Result)[Col * 3 + Row]; };
		R(0, 0) = +(M(1, 1) * M(2, 2) - M(2, 1) * M(1, 2)) * OneOverDet;
		R(1, 0) = -(M(1, 0) * M(2, 2) - M(2, 0) * M(1, 2)) * OneOverDet;
		R(2, 0) = +(M(1, 0) * M(2, 1) - M(2, 0) * M(1, 1)) * OneOverDet;
		R(0, 1) = -(M(0, 1) * M(2, 2) - M(2, 1) * M(0, 2)) * OneOverDet;
		R(1, 1) = +(M(0, 0) * M(2, 2) - M(2, 0) * M(0, 2)) * OneOverDet;
		R(2, 1) = -(M(0, 0) * M(2, 1) - M(2, 0) * M(0, 1)) * OneOverDet;
		R(0, 2) = +(M(0, 1) * M(1, 2) - M(1, 1) * M(0, 2)) * OneOverDet;
		R(1, 2) = -(M(0, 0) * M(1, 2) - M(1, 0) * M(0, 2)) * OneOverDet;
		R(2, 2) = +(M(0, 0) * M(1, 1) - M(1, 0) * M(0, 1)) * OneOverDet;
		return Result;
	}

	static mat4 Mat4Inverse(const mat4& Self)
	{
		const float* m = reinterpret_cast<const float*>(&Self);
		float inv[16];

		inv[0]  =  m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
		inv[4]  = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
		inv[8]  =  m[4] * m[9]  * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
		inv[12] = -m[4] * m[9]  * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
		inv[1]  = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
		inv[5]  =  m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
		inv[9]  = -m[0] * m[9]  * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
		inv[13] =  m[0] * m[9]  * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
		inv[2]  =  m[1] * m[6]  * m[15] - m[1] * m[7]  * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7]  - m[13] * m[3] * m[6];
		inv[6]  = -m[0] * m[6]  * m[15] + m[0] * m[7]  * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7]  + m[12] * m[3] * m[6];
		inv[10] =  m[0] * m[5]  * m[15] - m[0] * m[7]  * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7]  - m[12] * m[3] * m[5];
		inv[14] = -m[0] * m[5]  * m[14] + m[0] * m[6]  * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6]  + m[12] * m[2] * m[5];
		inv[3]  = -m[1] * m[6]  * m[11] + m[1] * m[7]  * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9]  * m[2] * m[7]  + m[9]  * m[3] * m[6];
		inv[7]  =  m[0] * m[6]  * m[11] - m[0] * m[7]  * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8]  * m[2] * m[7]  - m[8]  * m[3] * m[6];
		inv[11] = -m[0] * m[5]  * m[11] + m[0] * m[7]  * m[9]  + m[4] * m[1] * m[11] - m[4] * m[3] * m[9]  - m[8]  * m[1] * m[7]  + m[8]  * m[3] * m[5];
		inv[15] =  m[0] * m[5]  * m[10] - m[0] * m[6]  * m[9]  - m[4] * m[1] * m[10] + m[4] * m[2] * m[9]  + m[8]  * m[1] * m[6]  - m[8]  * m[2] * m[5];

		float OneOverDet = 1.0f / (m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12]);

		mat4 Result;
		float* r = reinterpret_cast<float*>(&Result);
		for (int i = 0; i < 16; i++) r[i] = inv[i] * OneOverDet;
		return Result;
	}

	template<typename M, int N>
	static M Transpose(const M& Self)
	{
		M Result;
		auto m = reinterpret_cast<const float*>(&Self);
		auto r = reinterpret_cast<float*>(&Result);
		for (int Col = 0; Col < N; Col++)
			for (int Row = 0; Row < N; Row++)
				r[Col * N + Row] = m[Row * N + Col];
		return Result;
	}

	// The mat3:: and mat4:: functions, matching glm's

	static mat3 Mat3Translate(const vec2& V) { mat3 M = Mat3Identity(); M.z = { V.x, V.y, 1.0f }; return M; }
	static mat3 Mat3Scale(const vec2& V) { return { { V.x, 0, 0 }, { 0, V.y, 0 }, { 0, 0, 1 } }; }
	static mat3 Mat3ScaleUniform(float S) { return Mat3Scale({ S, S }); }

	static mat3 Mat3Rotate(float Angle)
	{
		float C = std::cos(Angle), S = std::sin(Angle);
		return { { C, S, 0 }, { -S, C, 0 }, { 0, 0, 1 } };
	}

	static mat4 Mat4Translate(const vec3& V) { mat4 M = Mat4Identity(); M.t = { V.x, V.y, V.z, 1.0f }; return M; }
	static mat4 Mat4Scale(const vec3& V) { return { { V.x, 0, 0, 0 }, { 0, V.y, 0, 0 }, { 0, 0, V.z, 0 }, { 0, 0, 0, 1 } }; }
	static mat4 Mat4ScaleUniform(float S) { return Mat4Scale({ S, S, S }); }

	static mat4 Mat4Rotate(float Angle, const vec3& Dir)
	{
		float C = std::cos(Angle), S = std::sin(Angle);
		vec3 Axis = Normalized(Dir);
		vec3 Temp = Scaled(Axis, 1.0f - C);

		mat4 M = Mat4Identity();
		M.x = { C + Temp.x * Axis.x,          Temp.x * Axis.y + S * Axis.z, Temp.x * Axis.z - S * Axis.y, 0 };
		M.y = { Temp.y * Axis.x - S * Axis.z, C + Temp.y * Axis.y,          Temp.y * Axis.z + S * Axis.x, 0 };
		M.z = { Temp.z * Axis.x + S * Axis.y, Temp.z * Axis.y - S * Axis.x, C + Temp.z * Axis.z,          0 };
		return M;
	}

	// Right-handed, with depth from -1 to 1 (glm's default)
	static mat4 Mat4Perspective(float YFov, float Aspect, float NearZ, float FarZ)
	{
		float TanHalfFov = std::tan(YFov / 2.0f);

		mat4 M = {};
		M.x.x = 1.0f / (Aspect * TanHalfFov);
		M.y.y = 1.0f / TanHalfFov;
		M.z.z = -(FarZ + NearZ) / (FarZ - NearZ);
		M.z.w = -1.0f;
		M.t.z = -(2.0f * FarZ * NearZ) / (FarZ - NearZ);
		return M;
	}

	// Right-handed (glm's default)
	static mat4 Mat4LookAt(const vec3& Eye, const vec3& Center, const vec3& Up)
	{
		vec3 F = Normalized(Difference(Center, Eye));
		vec3 S = Normalized(Cross(F, Up));
		vec3 U = Cross(S, F);

		mat4 M = Mat4Identity();
		M.x = { S.x, U.x, -F.x, 0 };
		M.y = { S.y, U.y, -F.y, 0 };
		M.z = { S.z, U.z, -F.z, 0 };
		M.t = { -Dot(S, Eye), -Dot(U, Eye), Dot(F, Eye), 1 };
		return M;
	}

	// Quaternions, matching glm's

	static void QuatConstructIdentity(quat* Self) { *Self = { 0, 0, 0, 1 }; }

	// Pitch (x), yaw (y) and roll (z), in radians
	static void QuatConstructEuler(const vec3& Euler, quat* Self)
	{
		float CX = std::cos(Euler.x * 0.5f), CY = std::cos(Euler.y * 0.5f), CZ = std::cos(Euler.z * 0.5f);
		float SX = std::sin(Euler.x * 0.5f), SY = std::sin(Euler.y * 0.5f), SZ = std::sin(Euler.z * 0.5f);

		Self->w = CX * CY * CZ + SX * SY * SZ;
		Self->x = SX * CY * CZ - CX * SY * SZ;
		Self->y = CX * SY * CZ + SX * CY * SZ;
		Self->z = CX * CY * SZ - SX * SY * CZ;
	}

	static void QuatConstructAxisAngle(const vec3& Axis, float Angle, quat* Self)
	{
		float S = std::sin(Angle * 0.5f);
		*Self = { Axis.x * S, Axis.y * S, Axis.z * S, std::cos(Angle * 0.5f) };
	}

	static void QuatConstructMat3(const mat3& M, quat* Self)
	{
		float FourXSquaredMinus1 = M.x.x - M.y.y - M.z.z;
		float FourYSquaredMinus1 = M.y.y - M.x.x - M.z.z;
		float FourZSquaredMinus1 = M.z.z - M.x.x - M.y.y;
		float FourWSquaredMinus1 = M.x.x + M.y.y + M.z.z;

		int BiggestIndex = 0;
		float FourBiggestSquaredMinus1 = FourWSquaredMinus1;
		if (FourXSquaredMinus1 > FourBiggestSquaredMinus1) { FourBiggestSquaredMinus1 = FourXSquaredMinus1; BiggestIndex = 1; }
		if (FourYSquaredMinus1 > FourBiggestSquaredMinus1) { FourBiggestSquaredMinus1 = FourYSquaredMinus1; BiggestIndex = 2; }
		if (FourZSquaredMinus1 > FourBiggestSquaredMinus1) { FourBiggestSquaredMinus1 = FourZSquaredMinus1; BiggestIndex = 3; }

		float BiggestVal = std::sqrt(FourBiggestSquaredMinus1 + 1.0f) * 0.5f;
		float Mult = 0.25f / BiggestVal;

		switch (BiggestIndex)
		{
		case 0: *Self = { (M.y.z - M.z.y) * Mult, (M.z.x - M.x.z) * Mult, (M.x.y - M.y.x) * Mult, BiggestVal }; break;
		case 1: *Self = { BiggestVal, (M.x.y + M.y.x) * Mult, (M.z.x + M.x.z) * Mult, (M.y.z - M.z.y) * Mult }; break;
		case 2: *Self = { (M.x.y + M.y.x) * Mult, BiggestVal, (M.y.z + M.z.y) * Mult, (M.z.x - M.x.z) * Mult }; break;
		default: *Self = { (M.z.x + M.x.z) * Mult, (M.y.z + M.z.y) * Mult, BiggestVal, (M.x.y - M.y.x) * Mult }; break;
		}
	}

	static void QuatConstructMat4(const mat4& M, quat* Self)
	{
		mat3 Rotation;
		Mat3ConstructMat4(M, &Rotation);
		QuatConstructMat3(Rotation, Self);
	}

	static quat QuatMul(const quat& P, const quat& Q)
	{
		return {
			P.w * Q.x + P.x * Q.w + P.y * Q.z - P.z * Q.y,
			P.w * Q.y + P.y * Q.w + P.z * Q.x - P.x * Q.z,
			P.w * Q.z + P.z * Q.w + P.x * Q.y - P.y * Q.x,
			P.w * Q.w - P.x * Q.x - P.y * Q.y - P.z * Q.z
		};
	}

	static quat QuatInverse(const quat& Self)
	{
		float LenSq = LengthSquared(Self);
		return { -Self.x / LenSq, -Self.y / LenSq, -Self.z / LenSq, Self.w / LenSq };
	}

	static vec3 QuatRotate(const quat& Q, const vec3& V)
	{
		vec3 QuatVector = { Q.x, Q.y, Q.z };
		vec3 UV = Cross(QuatVector, V);
		vec3 UUV = Cross(QuatVector, UV);
		return Sum(V, Scaled(Sum(Scaled(UV, Q.w), UUV), 2.0f));
	}

	static quat QuatMulQuat(const quat& Other, const quat& Self) { return QuatMul(Self, Other); }
	static quat QuatMulQuatReversed(const quat& Other, const quat& Self) { return QuatMul(Other, Self); }
	static vec3 QuatMulVec3(const vec3& V, const quat& Self) { return QuatRotate(Self, V); }
	static vec3 Vec3MulQuat(const vec3& V, const quat& Self) { return QuatRotate(QuatInverse(Self), V); }
	static vec3 QuatXYZ(const quat& Self) { return { Self.x, Self.y, Self.z }; }

	static quat QuatNormalized(const quat& Self)
	{
		float Len = Length(Self);
		if (Len <= 0.0f) return { 0, 0, 0, 1 };
		return OpScalar<quat, Op::Div>(Len, Self);
	}

	static float QuatAngle(const quat& Self)
	{
		constexpr float CosOneOverTwo = 0.877582561890372716130286068203503191f;
		constexpr float Pi = 3.14159265358979323846f;
		if (std::abs(Self.w) > CosOneOverTwo)
		{
			float A = std::asin(std::sqrt(Self.x * Self.x + Self.y * Self.y + Self.z * Self.z)) * 2.0f;
			return Self.w < 0.0f ? Pi * 2.0f - A : A;
		}
		return std::acos(Self.w) * 2.0f;
	}

	static vec3 QuatAxis(const quat& Self)
	{
		float Tmp1 = 1.0f - Self.w * Self.w;
		if (Tmp1 <= 0.0f) return { 0, 0, 1 };
		float Tmp2 = 1.0f / std::sqrt(Tmp1);
		return { Self.x * Tmp2, Self.y * Tmp2, Self.z * Tmp2 };
	}

	// Pitch (x), yaw (y) and roll (z), in radians
	static vec3 QuatEuler(const quat& Q)
	{
		float PitchY = 2.0f * (Q.y * Q.z + Q.w * Q.x);
		float PitchX = Q.w * Q.w - Q.x * Q.x - Q.y * Q.y + Q.z * Q.z;
		float Pitch = (PitchY == 0.0f and PitchX == 0.0f) ? 2.0f * std::atan2(Q.x, Q.w) : std::atan2(PitchY, PitchX);

		float SinYaw = -2.0f * (Q.x * Q.z - Q.w * Q.y);
		float Yaw = std::asin(SinYaw < -1.0f ? -1.0f : (SinYaw > 1.0f ? 1.0f : SinYaw));

		float Roll = std::atan2(2.0f * (Q.x * Q.y + Q.w * Q.z), Q.w * Q.w + Q.x * Q.x - Q.y * Q.y - Q.z * Q.z);

		return { Pitch, Yaw, Roll };
	}

	// The Math:: functions

	template<typename V>
	static V Lerp(const V& Min, const V& Max, float X) { return Sum(Min, Scaled(Difference(Max, Min), X)); }

	template<typename V>
	static float Distance2(const V& A, const V& B) { return LengthSquared(Difference(A, B)); }

	template<typename V>
	static float Distance(const V& A, const V& B) { return Length(Difference(A, B)); }

	// Unlike glm's, A and B don't have to be normalized
	template<typename V>
	static float Angle(const V& A, const V& B)
	{
		float Lengths = Length(A) * Length(B);
		if (Lengths == 0.0f) return 0.0f;
		float Cos = Dot(A, B) / Lengths;
		return std::acos(Cos < -1.0f ? -1.0f : (Cos > 1.0f ? 1.0f : Cos));
	}

	// Takes the shortest path
	static quat Slerp(const quat& A, const quat& B, float X)
	{
		quat Z = B;
		float CosTheta = Dot(A, B);
		if (CosTheta < 0.0f)
		{
			Z = Scaled(B, -1.0f);
			CosTheta = -CosTheta;
		}

		// Too close for sin(angle) to be safe to divide by
		if (CosTheta > 1.0f - 1.1920929e-7f)
		{
			return Lerp(A, Z, X);
		}

		float Theta = std::acos(CosTheta);
		return Scaled(Sum(Scaled(A, std::sin((1.0f - X) * Theta)), Scaled(Z, std::sin(X * Theta))), 1.0f / std::sin(Theta));
	}

	// Registration

	static std::vector<GeckoExport> s_Exports;

#define MATH_CONSTRUCT(Class_, Decl_, Fn_) GECKO_EXPORT_BEHAVIOUR(s_Exports, "", Class_, asBEHAVE_CONSTRUCT, Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)
#define MATH_METHOD(Class_, Decl_, Fn_)    GECKO_EXPORT_METHOD(s_Exports, "", Class_, Decl_, asFUNCTION(Fn_), asCALL_CDECL_OBJLAST)
#define MATH_PROP(Class_, Decl_, Offset_)  GECKO_EXPORT_PROP(s_Exports, "", Class_, Decl_, Offset_)
#define MATH_FN(Ns_, Decl_, Fn_)           GECKO_EXPORT_FN(s_Exports, Ns_, Decl_, Fn_)

	// Everything vec2, vec3, vec4, int2, int3, nat2 and nat3 have in common. T is the type, S its scalar's script name.
#define MATH_VECTOR(T, S) \
	MATH_CONSTRUCT(#T, "void f()", ConstructZero<T>); \
	MATH_CONSTRUCT(#T, "void f(" #S " scalar)", ConstructScalar<T>); \
	MATH_CONSTRUCT(#T, "void f(const " #T "&in)", ConstructCopy<T>); \
	MATH_METHOD(#T, #S " opIndex(int) const", Index<T>); \
	MATH_METHOD(#T, #T " opAdd(const " #T "&in) const", (OpVec<T, Op::Add>)); \
	MATH_METHOD(#T, #T " opSub(const " #T "&in) const", (OpVec<T, Op::Sub>)); \
	MATH_METHOD(#T, #T " opMul(const " #T "&in) const", (OpVec<T, Op::Mul>)); \
	MATH_METHOD(#T, #T " opDiv(const " #T "&in) const", (OpVec<T, Op::Div>)); \
	MATH_METHOD(#T, #T " opAdd(" #S ") const", (OpScalar<T, Op::Add>)); \
	MATH_METHOD(#T, #T " opSub(" #S ") const", (OpScalar<T, Op::Sub>)); \
	MATH_METHOD(#T, #T " opMul(" #S ") const", (OpScalar<T, Op::Mul>)); \
	MATH_METHOD(#T, #T " opDiv(" #S ") const", (OpScalar<T, Op::Div>)); \
	MATH_METHOD(#T, #T " opAdd_r(" #S ") const", (OpScalarReversed<T, Op::Add>)); \
	MATH_METHOD(#T, #T " opSub_r(" #S ") const", (OpScalarReversed<T, Op::Sub>)); \
	MATH_METHOD(#T, #T " opMul_r(" #S ") const", (OpScalarReversed<T, Op::Mul>)); \
	MATH_METHOD(#T, #T " opDiv_r(" #S ") const", (OpScalarReversed<T, Op::Div>)); \
	MATH_METHOD(#T, #T "& opAddAssign(const " #T "&in)", (OpAssignVec<T, Op::Add>)); \
	MATH_METHOD(#T, #T "& opSubAssign(const " #T "&in)", (OpAssignVec<T, Op::Sub>)); \
	MATH_METHOD(#T, #T "& opMulAssign(const " #T "&in)", (OpAssignVec<T, Op::Mul>)); \
	MATH_METHOD(#T, #T "& opDivAssign(const " #T "&in)", (OpAssignVec<T, Op::Div>)); \
	MATH_METHOD(#T, #T "& opAddAssign(" #S ")", (OpAssignScalar<T, Op::Add>)); \
	MATH_METHOD(#T, #T "& opSubAssign(" #S ")", (OpAssignScalar<T, Op::Sub>)); \
	MATH_METHOD(#T, #T "& opMulAssign(" #S ")", (OpAssignScalar<T, Op::Mul>)); \
	MATH_METHOD(#T, #T "& opDivAssign(" #S ")", (OpAssignScalar<T, Op::Div>)); \
	MATH_METHOD(#T, "bool opEquals(const " #T "&in) const", Equals<T>); \
	MATH_METHOD(#T, "string ToString() const", ToString<T>)

	// What only the float vectors have
#define MATH_FLOAT_VECTOR(T) \
	MATH_METHOD(#T, "float Length() const", Length<T>); \
	MATH_METHOD(#T, "float LengthSquared() const", LengthSquared<T>); \
	MATH_METHOD(#T, #T " Normalized() const", Normalized<T>)

	/*
	!CompatMeta
	for     Global
	with    float x
	with    float y
	being   prop
	of      vec2
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    float x
	with    float y
	with    float z
	being   prop
	of      vec3
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    float x
	with    float y
	with    float z
	with    float w
	being   prop
	of      vec4
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    int x
	with    int y
	being   prop
	of      int2
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    int x
	with    int y
	with    int z
	being   prop
	of      int3
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    uint x
	with    uint y
	being   prop
	of      nat2
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    uint x
	with    uint y
	with    uint z
	being   prop
	of      nat3
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    float x
	with    float y
	with    float z
	with    float w
	being   prop
	of      quat
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_PROP("vec2", "float x", asOFFSET(vec2, x));
	MATH_PROP("vec2", "float y", asOFFSET(vec2, y));
	MATH_PROP("vec3", "float x", asOFFSET(vec3, x));
	MATH_PROP("vec3", "float y", asOFFSET(vec3, y));
	MATH_PROP("vec3", "float z", asOFFSET(vec3, z));
	MATH_PROP("vec4", "float x", asOFFSET(vec4, x));
	MATH_PROP("vec4", "float y", asOFFSET(vec4, y));
	MATH_PROP("vec4", "float z", asOFFSET(vec4, z));
	MATH_PROP("vec4", "float w", asOFFSET(vec4, w));
	MATH_PROP("int2", "int x", asOFFSET(int2, x));
	MATH_PROP("int2", "int y", asOFFSET(int2, y));
	MATH_PROP("int3", "int x", asOFFSET(int3, x));
	MATH_PROP("int3", "int y", asOFFSET(int3, y));
	MATH_PROP("int3", "int z", asOFFSET(int3, z));
	MATH_PROP("nat2", "uint x", asOFFSET(nat2, x));
	MATH_PROP("nat2", "uint y", asOFFSET(nat2, y));
	MATH_PROP("nat3", "uint x", asOFFSET(nat3, x));
	MATH_PROP("nat3", "uint y", asOFFSET(nat3, y));
	MATH_PROP("nat3", "uint z", asOFFSET(nat3, z));
	MATH_PROP("quat", "float x", asOFFSET(quat, x));
	MATH_PROP("quat", "float y", asOFFSET(quat, y));
	MATH_PROP("quat", "float z", asOFFSET(quat, z));
	MATH_PROP("quat", "float w", asOFFSET(quat, w));

	/*
	!CompatMeta
	for     Global
	with    ~vec2()
	with    vec2()
	with    vec2(float scalar)
	with    vec2(float x, float y)
	with    vec2(const vec2&in)
	being   behavior
	of      vec2
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    float opIndex(int) const
	with    vec2 opAdd(const vec2&in) const
	with    vec2 opSub(const vec2&in) const
	with    vec2 opMul(const vec2&in) const
	with    vec2 opDiv(const vec2&in) const
	with    vec2 opAdd(float) const
	with    vec2 opSub(float) const
	with    vec2 opMul(float) const
	with    vec2 opDiv(float) const
	with    vec2 opAdd_r(float) const
	with    vec2 opSub_r(float) const
	with    vec2 opMul_r(float) const
	with    vec2 opDiv_r(float) const
	with    vec2& opAddAssign(const vec2&in)
	with    vec2& opSubAssign(const vec2&in)
	with    vec2& opMulAssign(const vec2&in)
	with    vec2& opDivAssign(const vec2&in)
	with    vec2& opAddAssign(float)
	with    vec2& opSubAssign(float)
	with    vec2& opMulAssign(float)
	with    vec2& opDivAssign(float)
	with    bool opEquals(const vec2&in)
	with    float Length() const
	with    float LengthSquared() const
	with    vec2 Normalized() const
	with    string ToString() const
	being   method
	of      vec2
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_VECTOR(vec2, float);
	MATH_FLOAT_VECTOR(vec2);
	MATH_CONSTRUCT("vec2", "void f(float x, float y)", (ConstructFrom<vec2, float, float>));

	/*
	!CompatMeta
	for     Global
	with    ~vec3()
	with    vec3()
	with    vec3(float scalar)
	with    vec3(float x, float y, float z)
	with    vec3(const vec2&in xy, float z)
	with    vec3(const vec3&in)
	being   behavior
	of      vec3
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    float opIndex(int) const
	with    vec3 opAdd(const vec3&in) const
	with    vec3 opSub(const vec3&in) const
	with    vec3 opMul(const vec3&in) const
	with    vec3 opDiv(const vec3&in) const
	with    vec3 opAdd(float) const
	with    vec3 opSub(float) const
	with    vec3 opMul(float) const
	with    vec3 opDiv(float) const
	with    vec3 opAdd_r(float) const
	with    vec3 opSub_r(float) const
	with    vec3 opMul_r(float) const
	with    vec3 opDiv_r(float) const
	with    vec3& opAddAssign(const vec3&in)
	with    vec3& opSubAssign(const vec3&in)
	with    vec3& opMulAssign(const vec3&in)
	with    vec3& opDivAssign(const vec3&in)
	with    vec3& opAddAssign(float)
	with    vec3& opSubAssign(float)
	with    vec3& opMulAssign(float)
	with    vec3& opDivAssign(float)
	with    bool opEquals(const vec3&in)
	with    float Length() const
	with    float LengthSquared() const
	with    vec3 Normalized() const
	with    vec2 get_xy() const
	with    vec2 get_yz() const
	with    string ToString() const
	being   method
	of      vec3
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_VECTOR(vec3, float);
	MATH_FLOAT_VECTOR(vec3);
	MATH_CONSTRUCT("vec3", "void f(float x, float y, float z)", (ConstructFrom<vec3, float, float, float>));
	MATH_CONSTRUCT("vec3", "void f(const vec2&in xy, float z)", Vec3ConstructXY);
	MATH_METHOD("vec3", "vec2 get_xy() const property", Vec3XY);
	MATH_METHOD("vec3", "vec2 get_yz() const property", Vec3YZ);

	/*
	!CompatMeta
	for     Global
	with    ~vec4()
	with    vec4()
	with    vec4(float scalar)
	with    vec4(float x, float y, float z, float w)
	with    vec4(const vec3&in xyz, float w)
	with    vec4(const vec2&in xy, const vec2&in zw)
	with    vec4(const vec4&in)
	being   behavior
	of      vec4
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    float opIndex(int) const
	with    vec4 opAdd(const vec4&in) const
	with    vec4 opSub(const vec4&in) const
	with    vec4 opMul(const vec4&in) const
	with    vec4 opDiv(const vec4&in) const
	with    vec4 opAdd(float) const
	with    vec4 opSub(float) const
	with    vec4 opMul(float) const
	with    vec4 opDiv(float) const
	with    vec4 opAdd_r(float) const
	with    vec4 opSub_r(float) const
	with    vec4 opMul_r(float) const
	with    vec4 opDiv_r(float) const
	with    vec4& opAddAssign(const vec4&in)
	with    vec4& opSubAssign(const vec4&in)
	with    vec4& opMulAssign(const vec4&in)
	with    vec4& opDivAssign(const vec4&in)
	with    vec4& opAddAssign(float)
	with    vec4& opSubAssign(float)
	with    vec4& opMulAssign(float)
	with    vec4& opDivAssign(float)
	with    bool opEquals(const vec4&in)
	with    float Length() const
	with    float LengthSquared() const
	with    vec4 Normalized() const
	with    vec2 get_xy() const
	with    vec3 get_xyz() const
	with    vec2 get_yz() const
	with    vec3 get_yzw() const
	with    vec2 get_zw() const
	with    string ToString() const
	being   method
	of      vec4
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_VECTOR(vec4, float);
	MATH_FLOAT_VECTOR(vec4);
	MATH_CONSTRUCT("vec4", "void f(float x, float y, float z, float w)", (ConstructFrom<vec4, float, float, float, float>));
	MATH_CONSTRUCT("vec4", "void f(const vec3&in xyz, float w)", Vec4ConstructXYZ);
	MATH_CONSTRUCT("vec4", "void f(const vec2&in xy, const vec2&in zw)", Vec4ConstructXYZW);
	MATH_METHOD("vec4", "vec2 get_xy() const property", Vec4XY);
	MATH_METHOD("vec4", "vec3 get_xyz() const property", Vec4XYZ);
	MATH_METHOD("vec4", "vec2 get_yz() const property", Vec4YZ);
	MATH_METHOD("vec4", "vec3 get_yzw() const property", Vec4YZW);
	MATH_METHOD("vec4", "vec2 get_zw() const property", Vec4ZW);

	/*
	!CompatMeta
	for     Global
	with    ~int2()
	with    int2()
	with    int2(int scalar)
	with    int2(int x, int y)
	with    int2(const int2&in)
	being   behavior
	of      int2
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    int opIndex(int) const
	with    int2 opAdd(const int2&in) const
	with    int2 opSub(const int2&in) const
	with    int2 opMul(const int2&in) const
	with    int2 opDiv(const int2&in) const
	with    int2 opAdd(int) const
	with    int2 opSub(int) const
	with    int2 opMul(int) const
	with    int2 opDiv(int) const
	with    int2 opAdd_r(int) const
	with    int2 opSub_r(int) const
	with    int2 opMul_r(int) const
	with    int2 opDiv_r(int) const
	with    int2& opAddAssign(const int2&in)
	with    int2& opSubAssign(const int2&in)
	with    int2& opMulAssign(const int2&in)
	with    int2& opDivAssign(const int2&in)
	with    int2& opAddAssign(int)
	with    int2& opSubAssign(int)
	with    int2& opMulAssign(int)
	with    int2& opDivAssign(int)
	with    bool opEquals(const int2&in)
	with    string ToString() const
	being   method
	of      int2
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_VECTOR(int2, int);
	MATH_CONSTRUCT("int2", "void f(int x, int y)", (ConstructFrom<int2, int32_t, int32_t>));

	/*
	!CompatMeta
	for     Global
	with    ~int3()
	with    int3()
	with    int3(int scalar)
	with    int3(int x, int y, int z)
	with    int3(const int2&in xy, int z)
	with    int3(const int3&in)
	being   behavior
	of      int3
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    int opIndex(int) const
	with    int3 opAdd(const int3&in) const
	with    int3 opSub(const int3&in) const
	with    int3 opMul(const int3&in) const
	with    int3 opDiv(const int3&in) const
	with    int3 opAdd(int) const
	with    int3 opSub(int) const
	with    int3 opMul(int) const
	with    int3 opDiv(int) const
	with    int3 opAdd_r(int) const
	with    int3 opSub_r(int) const
	with    int3 opMul_r(int) const
	with    int3 opDiv_r(int) const
	with    int3& opAddAssign(const int3&in)
	with    int3& opSubAssign(const int3&in)
	with    int3& opMulAssign(const int3&in)
	with    int3& opDivAssign(const int3&in)
	with    int3& opAddAssign(int)
	with    int3& opSubAssign(int)
	with    int3& opMulAssign(int)
	with    int3& opDivAssign(int)
	with    bool opEquals(const int3&in)
	with    int2 get_xy() const
	with    string ToString() const
	being   method
	of      int3
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_VECTOR(int3, int);
	MATH_CONSTRUCT("int3", "void f(int x, int y, int z)", (ConstructFrom<int3, int32_t, int32_t, int32_t>));
	MATH_CONSTRUCT("int3", "void f(const int2&in xy, int z)", Int3ConstructXY);
	MATH_METHOD("int3", "int2 get_xy() const property", Int3XY);

	/*
	!CompatMeta
	for     Global
	with    ~nat2()
	with    nat2()
	with    nat2(uint scalar)
	with    nat2(uint x, uint y)
	with    nat2(const nat2&in)
	being   behavior
	of      nat2
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    uint opIndex(int) const
	with    nat2 opAdd(const nat2&in) const
	with    nat2 opSub(const nat2&in) const
	with    nat2 opMul(const nat2&in) const
	with    nat2 opDiv(const nat2&in) const
	with    nat2 opAdd(uint) const
	with    nat2 opSub(uint) const
	with    nat2 opMul(uint) const
	with    nat2 opDiv(uint) const
	with    nat2 opAdd_r(uint) const
	with    nat2 opSub_r(uint) const
	with    nat2 opMul_r(uint) const
	with    nat2 opDiv_r(uint) const
	with    nat2& opAddAssign(const nat2&in)
	with    nat2& opSubAssign(const nat2&in)
	with    nat2& opMulAssign(const nat2&in)
	with    nat2& opDivAssign(const nat2&in)
	with    nat2& opAddAssign(uint)
	with    nat2& opSubAssign(uint)
	with    nat2& opMulAssign(uint)
	with    nat2& opDivAssign(uint)
	with    bool opEquals(const nat2&in)
	with    string ToString() const
	being   method
	of      nat2
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_VECTOR(nat2, uint);
	MATH_CONSTRUCT("nat2", "void f(uint x, uint y)", (ConstructFrom<nat2, uint32_t, uint32_t>));

	/*
	!CompatMeta
	for     Global
	with    ~nat3()
	with    nat3()
	with    nat3(uint scalar)
	with    nat3(uint x, uint y, uint z)
	with    nat3(const nat3&in)
	being   behavior
	of      nat3
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    uint opIndex(int) const
	with    nat3 opAdd(const nat3&in) const
	with    nat3 opSub(const nat3&in) const
	with    nat3 opMul(const nat3&in) const
	with    nat3 opDiv(const nat3&in) const
	with    nat3 opAdd(uint) const
	with    nat3 opSub(uint) const
	with    nat3 opMul(uint) const
	with    nat3 opDiv(uint) const
	with    nat3 opAdd_r(uint) const
	with    nat3 opSub_r(uint) const
	with    nat3 opMul_r(uint) const
	with    nat3 opDiv_r(uint) const
	with    nat3& opAddAssign(const nat3&in)
	with    nat3& opSubAssign(const nat3&in)
	with    nat3& opMulAssign(const nat3&in)
	with    nat3& opDivAssign(const nat3&in)
	with    nat3& opAddAssign(uint)
	with    nat3& opSubAssign(uint)
	with    nat3& opMulAssign(uint)
	with    nat3& opDivAssign(uint)
	with    bool opEquals(const nat3&in)
	with    nat2 get_xy() const
	with    string ToString() const
	being   method
	of      nat3
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    nat3(const nat3&in xy, uint z)
	as      nat3(const nat2&in xy, uint z)
	being   behavior
	of      nat3
	declare partial
	reason  Takes a nat2 for xy, since a nat3 there looks like a mistake
	since   v0.1.1
	!CompatMeta
	*/
	MATH_VECTOR(nat3, uint);
	MATH_CONSTRUCT("nat3", "void f(uint x, uint y, uint z)", (ConstructFrom<nat3, uint32_t, uint32_t, uint32_t>));
	MATH_CONSTRUCT("nat3", "void f(const nat2&in xy, uint z)", Nat3ConstructXY);
	MATH_METHOD("nat3", "nat2 get_xy() const property", Nat3XY);

	/*
	!CompatMeta
	for     Global
	with    ~iso3()
	with    iso3()
	with    iso3(const mat3&in)
	with    iso3(const iso3&in)
	being   behavior
	of      iso3
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    bool opEquals(const iso3&in)
	with    float get_xx() const
	with    float get_xy() const
	with    float get_yx() const
	with    float get_yy() const
	with    float get_tx() const
	with    float get_ty() const
	with    void Translate(float x, float y)
	being   method
	of      iso3
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_CONSTRUCT("iso3", "void f()", Iso3ConstructIdentity);
	MATH_CONSTRUCT("iso3", "void f(const mat3&in)", Iso3ConstructMat3);
	MATH_CONSTRUCT("iso3", "void f(const iso3&in)", ConstructCopy<iso3>);
	MATH_METHOD("iso3", "bool opEquals(const iso3&in) const", Equals<iso3>);
	MATH_METHOD("iso3", "float get_xx() const property", (GetField<iso3, &iso3::xx>));
	MATH_METHOD("iso3", "float get_xy() const property", (GetField<iso3, &iso3::xy>));
	MATH_METHOD("iso3", "float get_yx() const property", (GetField<iso3, &iso3::yx>));
	MATH_METHOD("iso3", "float get_yy() const property", (GetField<iso3, &iso3::yy>));
	MATH_METHOD("iso3", "float get_tx() const property", (GetField<iso3, &iso3::tx>));
	MATH_METHOD("iso3", "float get_ty() const property", (GetField<iso3, &iso3::ty>));
	MATH_METHOD("iso3", "void Translate(float x, float y)", Iso3Translate);

	/*
	!CompatMeta
	for     Global
	with    ~mat3()
	with    mat3()
	with    mat3(const iso3&in)
	with    mat3(const vec3&in v0, const vec3&in v1, const vec3&in v2)
	with    mat3(const mat4&in)
	with    mat3(const mat3&in)
	being   behavior
	of      mat3
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    bool opEquals(const mat3&in)
	with    float get_xx() const
	with    float get_xy() const
	with    float get_xz() const
	with    float get_yx() const
	with    float get_yy() const
	with    float get_yz() const
	with    float get_zx() const
	with    float get_zy() const
	with    float get_zz() const
	with    mat3 opMul(const mat3&in) const
	with    mat3& opMulAssign(const mat3&in)
	with    vec3 opMul(const vec2&in) const
	with    vec3 opMul(const vec3&in) const
	with    vec3 opMul_r(const vec2&in)
	with    vec3 opMul_r(const vec3&in)
	being   method
	of      mat3
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_CONSTRUCT("mat3", "void f()", Mat3ConstructIdentity);
	MATH_CONSTRUCT("mat3", "void f(const iso3&in)", Mat3ConstructIso3);
	MATH_CONSTRUCT("mat3", "void f(const vec3&in v0, const vec3&in v1, const vec3&in v2)", Mat3ConstructVecs);
	MATH_CONSTRUCT("mat3", "void f(const mat4&in)", Mat3ConstructMat4);
	MATH_CONSTRUCT("mat3", "void f(const mat3&in)", ConstructCopy<mat3>);
	MATH_METHOD("mat3", "bool opEquals(const mat3&in) const", Equals<mat3>);
	MATH_METHOD("mat3", "float get_xx() const property", (GetElem<mat3, vec3, &mat3::x, &vec3::x>));
	MATH_METHOD("mat3", "float get_xy() const property", (GetElem<mat3, vec3, &mat3::x, &vec3::y>));
	MATH_METHOD("mat3", "float get_xz() const property", (GetElem<mat3, vec3, &mat3::x, &vec3::z>));
	MATH_METHOD("mat3", "float get_yx() const property", (GetElem<mat3, vec3, &mat3::y, &vec3::x>));
	MATH_METHOD("mat3", "float get_yy() const property", (GetElem<mat3, vec3, &mat3::y, &vec3::y>));
	MATH_METHOD("mat3", "float get_yz() const property", (GetElem<mat3, vec3, &mat3::y, &vec3::z>));
	MATH_METHOD("mat3", "float get_zx() const property", (GetElem<mat3, vec3, &mat3::z, &vec3::x>));
	MATH_METHOD("mat3", "float get_zy() const property", (GetElem<mat3, vec3, &mat3::z, &vec3::y>));
	MATH_METHOD("mat3", "float get_zz() const property", (GetElem<mat3, vec3, &mat3::z, &vec3::z>));
	MATH_METHOD("mat3", "mat3 opMul(const mat3&in) const", Mat3MulMat3);
	MATH_METHOD("mat3", "mat3& opMulAssign(const mat3&in)", Mat3MulAssign);
	MATH_METHOD("mat3", "vec3 opMul(const vec2&in) const", Mat3MulVec2);
	MATH_METHOD("mat3", "vec3 opMul(const vec3&in) const", Mat3MulVec3);
	MATH_METHOD("mat3", "vec3 opMul_r(const vec2&in) const", Vec2MulMat3);
	MATH_METHOD("mat3", "vec3 opMul_r(const vec3&in) const", Vec3MulMat3);

	/*
	!CompatMeta
	for     Global
	with    ~iso4()
	with    iso4()
	with    iso4(const mat4&in)
	with    iso4(const iso4&in)
	being   behavior
	of      iso4
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    bool opEquals(const iso4&in)
	with    float get_xx() const
	with    float get_xy() const
	with    float get_xz() const
	with    float get_yx() const
	with    float get_yy() const
	with    float get_yz() const
	with    float get_zx() const
	with    float get_zy() const
	with    float get_zz() const
	with    float get_tx() const
	with    float get_ty() const
	with    float get_tz() const
	with    void Translate(float x, float y, float z)
	being   method
	of      iso4
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_CONSTRUCT("iso4", "void f()", Iso4ConstructIdentity);
	MATH_CONSTRUCT("iso4", "void f(const mat4&in)", Iso4ConstructMat4);
	MATH_CONSTRUCT("iso4", "void f(const iso4&in)", ConstructCopy<iso4>);
	MATH_METHOD("iso4", "bool opEquals(const iso4&in) const", Equals<iso4>);
	MATH_METHOD("iso4", "float get_xx() const property", (GetField<iso4, &iso4::xx>));
	MATH_METHOD("iso4", "float get_xy() const property", (GetField<iso4, &iso4::xy>));
	MATH_METHOD("iso4", "float get_xz() const property", (GetField<iso4, &iso4::xz>));
	MATH_METHOD("iso4", "float get_yx() const property", (GetField<iso4, &iso4::yx>));
	MATH_METHOD("iso4", "float get_yy() const property", (GetField<iso4, &iso4::yy>));
	MATH_METHOD("iso4", "float get_yz() const property", (GetField<iso4, &iso4::yz>));
	MATH_METHOD("iso4", "float get_zx() const property", (GetField<iso4, &iso4::zx>));
	MATH_METHOD("iso4", "float get_zy() const property", (GetField<iso4, &iso4::zy>));
	MATH_METHOD("iso4", "float get_zz() const property", (GetField<iso4, &iso4::zz>));
	MATH_METHOD("iso4", "float get_tx() const property", (GetField<iso4, &iso4::tx>));
	MATH_METHOD("iso4", "float get_ty() const property", (GetField<iso4, &iso4::ty>));
	MATH_METHOD("iso4", "float get_tz() const property", (GetField<iso4, &iso4::tz>));
	MATH_METHOD("iso4", "void Translate(float x, float y, float z)", Iso4Translate);

	/*
	!CompatMeta
	for     Global
	with    ~mat4()
	with    mat4()
	with    mat4(const iso4&in)
	with    mat4(const vec4&in v0, const vec4&in v1, const vec4&in v2, const vec4&in v3)
	with    mat4(const mat3&in)
	with    mat4(const mat4&in)
	being   behavior
	of      mat4
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    bool opEquals(const mat4&in)
	with    float get_xx() const
	with    float get_xy() const
	with    float get_xz() const
	with    float get_xw() const
	with    float get_yx() const
	with    float get_yy() const
	with    float get_yz() const
	with    float get_yw() const
	with    float get_zx() const
	with    float get_zy() const
	with    float get_zz() const
	with    float get_zw() const
	with    float get_tx() const
	with    float get_ty() const
	with    float get_tz() const
	with    float get_tw() const
	with    mat4 opMul(const mat4&in) const
	with    mat4& opMulAssign(const mat4&in)
	with    vec4 opMul(const vec3&in) const
	with    vec4 opMul(const vec4&in) const
	with    vec4 opMul_r(const vec3&in)
	with    vec4 opMul_r(const vec4&in)
	being   method
	of      mat4
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_CONSTRUCT("mat4", "void f()", Mat4ConstructIdentity);
	MATH_CONSTRUCT("mat4", "void f(const iso4&in)", Mat4ConstructIso4);
	MATH_CONSTRUCT("mat4", "void f(const vec4&in v0, const vec4&in v1, const vec4&in v2, const vec4&in v3)", Mat4ConstructVecs);
	MATH_CONSTRUCT("mat4", "void f(const mat3&in)", Mat4ConstructMat3);
	MATH_CONSTRUCT("mat4", "void f(const mat4&in)", ConstructCopy<mat4>);
	MATH_METHOD("mat4", "bool opEquals(const mat4&in) const", Equals<mat4>);
	MATH_METHOD("mat4", "float get_xx() const property", (GetElem<mat4, vec4, &mat4::x, &vec4::x>));
	MATH_METHOD("mat4", "float get_xy() const property", (GetElem<mat4, vec4, &mat4::x, &vec4::y>));
	MATH_METHOD("mat4", "float get_xz() const property", (GetElem<mat4, vec4, &mat4::x, &vec4::z>));
	MATH_METHOD("mat4", "float get_xw() const property", (GetElem<mat4, vec4, &mat4::x, &vec4::w>));
	MATH_METHOD("mat4", "float get_yx() const property", (GetElem<mat4, vec4, &mat4::y, &vec4::x>));
	MATH_METHOD("mat4", "float get_yy() const property", (GetElem<mat4, vec4, &mat4::y, &vec4::y>));
	MATH_METHOD("mat4", "float get_yz() const property", (GetElem<mat4, vec4, &mat4::y, &vec4::z>));
	MATH_METHOD("mat4", "float get_yw() const property", (GetElem<mat4, vec4, &mat4::y, &vec4::w>));
	MATH_METHOD("mat4", "float get_zx() const property", (GetElem<mat4, vec4, &mat4::z, &vec4::x>));
	MATH_METHOD("mat4", "float get_zy() const property", (GetElem<mat4, vec4, &mat4::z, &vec4::y>));
	MATH_METHOD("mat4", "float get_zz() const property", (GetElem<mat4, vec4, &mat4::z, &vec4::z>));
	MATH_METHOD("mat4", "float get_zw() const property", (GetElem<mat4, vec4, &mat4::z, &vec4::w>));
	MATH_METHOD("mat4", "float get_tx() const property", (GetElem<mat4, vec4, &mat4::t, &vec4::x>));
	MATH_METHOD("mat4", "float get_ty() const property", (GetElem<mat4, vec4, &mat4::t, &vec4::y>));
	MATH_METHOD("mat4", "float get_tz() const property", (GetElem<mat4, vec4, &mat4::t, &vec4::z>));
	MATH_METHOD("mat4", "float get_tw() const property", (GetElem<mat4, vec4, &mat4::t, &vec4::w>));
	MATH_METHOD("mat4", "mat4 opMul(const mat4&in) const", Mat4MulMat4);
	MATH_METHOD("mat4", "mat4& opMulAssign(const mat4&in)", Mat4MulAssign);
	MATH_METHOD("mat4", "vec4 opMul(const vec3&in) const", Mat4MulVec3);
	MATH_METHOD("mat4", "vec4 opMul(const vec4&in) const", Mat4MulVec4);
	MATH_METHOD("mat4", "vec4 opMul_r(const vec3&in) const", Vec3MulMat4);
	MATH_METHOD("mat4", "vec4 opMul_r(const vec4&in) const", Vec4MulMat4);

	/*
	!CompatMeta
	for     Global
	with    ~quat()
	with    quat()
	with    quat(const vec3&in euler)
	with    quat(const vec3&in axis, float angle)
	with    quat(float x, float y, float z, float w)
	with    quat(const mat3&in m)
	with    quat(const mat4&in m)
	with    quat(const quat&in)
	being   behavior
	of      quat
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     Global
	with    bool opEquals(const quat&in)
	with    quat opMul(const quat&in) const
	with    vec3 opMul(const vec3&in) const
	with    quat opMul_r(const quat&in)
	with    vec3 opMul_r(const vec3&in)
	with    vec3 get_xyz() const
	with    float Angle() const
	with    vec3 Axis() const
	with    vec3 Euler() const
	with    quat Inverse() const
	with    float Length() const
	with    float LengthSquared() const
	with    quat Normalized() const
	with    string ToString() const
	being   method
	of      quat
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_CONSTRUCT("quat", "void f()", QuatConstructIdentity);
	MATH_CONSTRUCT("quat", "void f(const vec3&in euler)", QuatConstructEuler);
	MATH_CONSTRUCT("quat", "void f(const vec3&in axis, float angle)", QuatConstructAxisAngle);
	MATH_CONSTRUCT("quat", "void f(float x, float y, float z, float w)", (ConstructFrom<quat, float, float, float, float>));
	MATH_CONSTRUCT("quat", "void f(const mat3&in m)", QuatConstructMat3);
	MATH_CONSTRUCT("quat", "void f(const mat4&in m)", QuatConstructMat4);
	MATH_CONSTRUCT("quat", "void f(const quat&in)", ConstructCopy<quat>);
	MATH_METHOD("quat", "bool opEquals(const quat&in) const", Equals<quat>);
	MATH_METHOD("quat", "quat opMul(const quat&in) const", QuatMulQuat);
	MATH_METHOD("quat", "vec3 opMul(const vec3&in) const", QuatMulVec3);
	MATH_METHOD("quat", "quat opMul_r(const quat&in) const", QuatMulQuatReversed);
	MATH_METHOD("quat", "vec3 opMul_r(const vec3&in) const", Vec3MulQuat);
	MATH_METHOD("quat", "vec3 get_xyz() const property", QuatXYZ);
	MATH_METHOD("quat", "float Angle() const", QuatAngle);
	MATH_METHOD("quat", "vec3 Axis() const", QuatAxis);
	MATH_METHOD("quat", "vec3 Euler() const", QuatEuler);
	MATH_METHOD("quat", "quat Inverse() const", QuatInverse);
	MATH_METHOD("quat", "float Length() const", Length<quat>);
	MATH_METHOD("quat", "float LengthSquared() const", LengthSquared<quat>);
	MATH_METHOD("quat", "quat Normalized() const", QuatNormalized);
	MATH_METHOD("quat", "string ToString() const", ToString<quat>);

	/*
	!CompatMeta
	for     Math
	with    vec2 Lerp(const vec2&in min, const vec2&in max, float x)
	with    vec3 Lerp(const vec3&in min, const vec3&in max, float x)
	with    vec4 Lerp(const vec4&in min, const vec4&in max, float x)
	with    float Distance2(const vec2&in a, const vec2&in b)
	with    float Distance2(const vec3&in a, const vec3&in b)
	with    float Distance(const vec2&in a, const vec2&in b)
	with    float Distance(const vec3&in a, const vec3&in b)
	with    float Dot(const vec2&in a, const vec2&in b)
	with    float Dot(const vec3&in a, const vec3&in b)
	with    float Angle(const vec2&in a, const vec2&in b)
	with    float Angle(const vec3&in a, const vec3&in b)
	with    vec3 Cross(const vec3&in a, const vec3&in b)
	with    quat Slerp(const quat&in a, const quat&in b, float x)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_FN("Math", "vec2 Lerp(const vec2&in min, const vec2&in max, float x)", Lerp<vec2>);
	MATH_FN("Math", "vec3 Lerp(const vec3&in min, const vec3&in max, float x)", Lerp<vec3>);
	MATH_FN("Math", "vec4 Lerp(const vec4&in min, const vec4&in max, float x)", Lerp<vec4>);
	MATH_FN("Math", "float Distance2(const vec2&in a, const vec2&in b)", Distance2<vec2>);
	MATH_FN("Math", "float Distance2(const vec3&in a, const vec3&in b)", Distance2<vec3>);
	MATH_FN("Math", "float Distance(const vec2&in a, const vec2&in b)", Distance<vec2>);
	MATH_FN("Math", "float Distance(const vec3&in a, const vec3&in b)", Distance<vec3>);
	MATH_FN("Math", "float Dot(const vec2&in a, const vec2&in b)", Dot<vec2>);
	MATH_FN("Math", "float Dot(const vec3&in a, const vec3&in b)", Dot<vec3>);
	MATH_FN("Math", "float Angle(const vec2&in a, const vec2&in b)", Angle<vec2>);
	MATH_FN("Math", "float Angle(const vec3&in a, const vec3&in b)", Angle<vec3>);
	MATH_FN("Math", "vec3 Cross(const vec3&in a, const vec3&in b)", Cross);
	MATH_FN("Math", "quat Slerp(const quat&in a, const quat&in b, float x)", Slerp);

	/*
	!CompatMeta
	for     mat3
	with    mat3 Identity()
	with    mat3 Translate(const vec2&in v)
	with    mat3 Rotate(float angle)
	with    mat3 Scale(const vec2&in scale)
	with    mat3 Scale(float scale)
	with    mat3 Inverse(const mat3&in)
	with    mat3 Transpose(const mat3&in)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_FN("mat3", "mat3 Identity()", Mat3Identity);
	MATH_FN("mat3", "mat3 Translate(const vec2&in v)", Mat3Translate);
	MATH_FN("mat3", "mat3 Rotate(float angle)", Mat3Rotate);
	MATH_FN("mat3", "mat3 Scale(const vec2&in scale)", Mat3Scale);
	MATH_FN("mat3", "mat3 Scale(float scale)", Mat3ScaleUniform);
	MATH_FN("mat3", "mat3 Inverse(const mat3&in)", Mat3Inverse);
	MATH_FN("mat3", "mat3 Transpose(const mat3&in)", (Transpose<mat3, 3>));

	/*
	!CompatMeta
	for     mat4
	with    mat4 Identity()
	with    mat4 Translate(const vec3&in v)
	with    mat4 Rotate(float angle, const vec3&in dir)
	with    mat4 Scale(const vec3&in scale)
	with    mat4 Scale(float scale)
	with    mat4 Inverse(const mat4&in)
	with    mat4 Transpose(const mat4&in)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     mat4
	with    mat4 Perspective(float yFov, float aspect, float nearZ, float farZ)
	as      mat4 Perspective(float yFov, float aspect, float nearZ, float farZ)
	being   function
	declare partial
	reason  Uses glm's right-handed convention with depth from -1 to 1, which may differ from Openplanet's
	since   v0.1.1
	!CompatMeta
	*/
	/*
	!CompatMeta
	for     mat4
	with    mat4 LookAt(const vec3&in eye, const vec3&in center, const vec3&in up)
	as      mat4 LookAt(const vec3&in eye, const vec3&in center, const vec3&in up)
	being   function
	declare partial
	reason  Uses glm's right-handed convention, which may differ from Openplanet's
	since   v0.1.1
	!CompatMeta
	*/
	MATH_FN("mat4", "mat4 Identity()", Mat4Identity);
	MATH_FN("mat4", "mat4 Translate(const vec3&in v)", Mat4Translate);
	MATH_FN("mat4", "mat4 Rotate(float angle, const vec3&in dir)", Mat4Rotate);
	MATH_FN("mat4", "mat4 Scale(const vec3&in scale)", Mat4Scale);
	MATH_FN("mat4", "mat4 Scale(float scale)", Mat4ScaleUniform);
	MATH_FN("mat4", "mat4 Perspective(float yFov, float aspect, float nearZ, float farZ)", Mat4Perspective);
	MATH_FN("mat4", "mat4 Inverse(const mat4&in)", Mat4Inverse);
	MATH_FN("mat4", "mat4 LookAt(const vec3&in eye, const vec3&in center, const vec3&in up)", Mat4LookAt);
	MATH_FN("mat4", "mat4 Transpose(const mat4&in)", (Transpose<mat4, 4>));

	static std::mt19937_64& Rng()
	{
		static std::mt19937_64 Engine{ std::random_device{}() };
		return Engine;
	}

	static int AbsInt(int I) { return I < 0 ? -I : I; }
	static float AbsFloat(float F) { return std::fabs(F); }
	static float SinF(float F) { return std::sin(F); }
	static float AsinF(float F) { return std::asin(F); }
	static float CosF(float F) { return std::cos(F); }
	static float AcosF(float F) { return std::acos(F); }
	static float TanF(float F) { return std::tan(F); }
	static float AtanF(float F) { return std::atan(F); }
	static float Atan2F(float Y, float X) { return std::atan2(Y, X); }
	static float ExpF(float F) { return std::exp(F); }
	static float PowF(float X, float Y) { return std::pow(X, Y); }
	static float SqrtF(float F) { return std::sqrt(F); }
	static float ToDeg(float Rad) { return Rad * (180.0f / 3.14159265358979f); }
	static float ToRad(float Deg) { return Deg * (3.14159265358979f / 180.0f); }
	static float LogF(float F) { return std::log(F); }
	static float Log2F(float F) { return std::log2(F); }
	static float Log10F(float F) { return std::log10(F); }
	static float Log1pF(float F) { return std::log1p(F); }
	static float LogbF(float F) { return std::logb(F); }
	static float FloorF(float F) { return std::floor(F); }
	static float CeilF(float F) { return std::ceil(F); }
	static float RoundF(float F) { return std::round(F); }

	static float RoundDecimals(float F, int Decimals)
	{
		double Scale = std::pow(10.0, Decimals);
		return (float)(std::round(F * Scale) / Scale);
	}

	static float RandFloat(float Min, float Max)
	{
		if (not (Max > Min)) return Min;
		return std::uniform_real_distribution<float>(Min, Max)(Rng());
	}

	static int RandInt(int Min, int Max)
	{
		if (Max <= Min) return Min;
		return std::uniform_int_distribution<int>(Min, Max - 1)(Rng());
	}

	static float InvLerpFloat(const float& Min, const float& Max, const float& Value) { return Max == Min ? 0.0f : (Value - Min) / (Max - Min); }
	static float InvLerpInt(const int& Min, const int& Max, const int& Value) { return Max == Min ? 0.0f : float(Value - Min) / float(Max - Min); }
	static float LerpFloat(const float& Min, const float& Max, float X) { return Min + (Max - Min) * X; }
	static int MinInt(int X, int Y) { return X < Y ? X : Y; }
	static float MinFloat(float X, float Y) { return X < Y ? X : Y; }
	static int MaxInt(int X, int Y) { return X > Y ? X : Y; }
	static float MaxFloat(float X, float Y) { return X > Y ? X : Y; }
	static int ClampInt(int X, int Min, int Max) { return X < Min ? Min : X > Max ? Max : X; }
	static float ClampFloat(float X, float Min, float Max) { return X < Min ? Min : X > Max ? Max : X; }
	static bool IsNaN(float F) { return std::isnan(F); }
	static bool IsInf(float F) { return std::isinf(F); }
	static uint16_t SwapBytes16(uint16_t V) { return _byteswap_ushort(V); }
	static uint32_t SwapBytes32(uint32_t V) { return _byteswap_ulong(V); }
	static uint64_t SwapBytes64(uint64_t V) { return _byteswap_uint64(V); }

	static const float  s_PI      = 3.14159265358979323846f;
	static const float  s_PI2     = 3.14159265358979323846f * 2.0f;
	static const double s_PIl     = 3.14159265358979323846;
	static const double s_PI2l    = 3.14159265358979323846 * 2.0;
	static const float  s_PosInf  = std::numeric_limits<float>::infinity();
	static const float  s_NegInf  = -std::numeric_limits<float>::infinity();
	static const double s_PosInfl = std::numeric_limits<double>::infinity();
	static const double s_NegInfl = -std::numeric_limits<double>::infinity();

	/*
	!CompatMeta
	for     Math
	with    int Abs(int i)
	with    float Abs(float f)
	with    float Sin(float f)
	with    float Asin(float f)
	with    float Cos(float f)
	with    float Acos(float f)
	with    float Tan(float f)
	with    float Atan(float f)
	with    float Atan2(float y, float x)
	with    float Exp(float f)
	with    float Pow(float x, float y)
	with    float Sqrt(float f)
	with    float ToDeg(float rad)
	with    float ToRad(float deg)
	with    float Rand(float min, float max)
	with    int Rand(int min, int max)
	with    float Log(float f)
	with    float Log2(float f)
	with    float Log10(float f)
	with    float Log1p(float f)
	with    float Logb(float f)
	with    float Floor(float f)
	with    float Ceil(float f)
	with    float Round(float f)
	with    float Round(float f, int decimals)
	with    float InvLerp(const float&in min, const float&in max, const float&in value)
	with    float InvLerp(const int&in min, const int&in max, const int&in value)
	with    float Lerp(const float&in min, const float&in max, float x)
	with    int Min(int x, int y)
	with    float Min(float x, float y)
	with    int Max(int x, int y)
	with    float Max(float x, float y)
	with    int Clamp(int x, int min, int max)
	with    float Clamp(float x, float min, float max)
	with    bool IsNaN(float)
	with    bool IsInf(float)
	with    uint16 SwapBytes(uint16)
	with    uint SwapBytes(uint)
	with    uint64 SwapBytes(uint64)
	being   function
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	MATH_FN("Math", "int Abs(int i)", AbsInt);
	MATH_FN("Math", "float Abs(float f)", AbsFloat);
	MATH_FN("Math", "float Sin(float f)", SinF);
	MATH_FN("Math", "float Asin(float f)", AsinF);
	MATH_FN("Math", "float Cos(float f)", CosF);
	MATH_FN("Math", "float Acos(float f)", AcosF);
	MATH_FN("Math", "float Tan(float f)", TanF);
	MATH_FN("Math", "float Atan(float f)", AtanF);
	MATH_FN("Math", "float Atan2(float y, float x)", Atan2F);
	MATH_FN("Math", "float Exp(float f)", ExpF);
	MATH_FN("Math", "float Pow(float x, float y)", PowF);
	MATH_FN("Math", "float Sqrt(float f)", SqrtF);
	MATH_FN("Math", "float ToDeg(float rad)", ToDeg);
	MATH_FN("Math", "float ToRad(float deg)", ToRad);
	MATH_FN("Math", "float Rand(float min, float max)", RandFloat);
	MATH_FN("Math", "int Rand(int min, int max)", RandInt);
	MATH_FN("Math", "float Log(float f)", LogF);
	MATH_FN("Math", "float Log2(float f)", Log2F);
	MATH_FN("Math", "float Log10(float f)", Log10F);
	MATH_FN("Math", "float Log1p(float f)", Log1pF);
	MATH_FN("Math", "float Logb(float f)", LogbF);
	MATH_FN("Math", "float Floor(float f)", FloorF);
	MATH_FN("Math", "float Ceil(float f)", CeilF);
	MATH_FN("Math", "float Round(float f)", RoundF);
	MATH_FN("Math", "float Round(float f, int decimals)", RoundDecimals);
	MATH_FN("Math", "float InvLerp(const float&in min, const float&in max, const float&in value)", InvLerpFloat);
	MATH_FN("Math", "float InvLerp(const int&in min, const int&in max, const int&in value)", InvLerpInt);
	MATH_FN("Math", "float Lerp(const float&in min, const float&in max, float x)", LerpFloat);
	MATH_FN("Math", "int Min(int x, int y)", MinInt);
	MATH_FN("Math", "float Min(float x, float y)", MinFloat);
	MATH_FN("Math", "int Max(int x, int y)", MaxInt);
	MATH_FN("Math", "float Max(float x, float y)", MaxFloat);
	MATH_FN("Math", "int Clamp(int x, int min, int max)", ClampInt);
	MATH_FN("Math", "float Clamp(float x, float min, float max)", ClampFloat);
	MATH_FN("Math", "bool IsNaN(float)", IsNaN);
	MATH_FN("Math", "bool IsInf(float)", IsInf);
	MATH_FN("Math", "uint16 SwapBytes(uint16)", SwapBytes16);
	MATH_FN("Math", "uint SwapBytes(uint)", SwapBytes32);
	MATH_FN("Math", "uint64 SwapBytes(uint64)", SwapBytes64);

	/*
	!CompatMeta
	for     Math
	with    PI
	with    PI2
	with    PIl
	with    PI2l
	with    PosInf
	with    NegInf
	with    PosInfl
	with    NegInfl
	being   global
	declare full
	since   v0.1.1
	!CompatMeta
	*/
	GECKO_EXPORT_GLOBAL(s_Exports, "Math", "const float PI", &s_PI);
	GECKO_EXPORT_GLOBAL(s_Exports, "Math", "const float PI2", &s_PI2);
	GECKO_EXPORT_GLOBAL(s_Exports, "Math", "const double PIl", &s_PIl);
	GECKO_EXPORT_GLOBAL(s_Exports, "Math", "const double PI2l", &s_PI2l);
	GECKO_EXPORT_GLOBAL(s_Exports, "Math", "const float PosInf", &s_PosInf);
	GECKO_EXPORT_GLOBAL(s_Exports, "Math", "const float NegInf", &s_NegInf);
	GECKO_EXPORT_GLOBAL(s_Exports, "Math", "const double PosInfl", &s_PosInfl);
	GECKO_EXPORT_GLOBAL(s_Exports, "Math", "const double NegInfl", &s_NegInfl);

	template<typename T>
	static void RegisterType(asIScriptEngine* Engine, const char* Name, asDWORD AllFlag)
	{
		Engine->RegisterObjectType(Name, sizeof(T), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<T>() | AllFlag);
	}

	// GeckoExport can't register types, so they're registered here
	void RegisterTypes(asIScriptEngine* Engine)
	{
		RegisterType<vec2>(Engine, "vec2", asOBJ_APP_CLASS_ALLFLOATS);
		RegisterType<vec3>(Engine, "vec3", asOBJ_APP_CLASS_ALLFLOATS);
		RegisterType<vec4>(Engine, "vec4", asOBJ_APP_CLASS_ALLFLOATS);
		RegisterType<int2>(Engine, "int2", asOBJ_APP_CLASS_ALLINTS);
		RegisterType<int3>(Engine, "int3", asOBJ_APP_CLASS_ALLINTS);
		RegisterType<nat2>(Engine, "nat2", asOBJ_APP_CLASS_ALLINTS);
		RegisterType<nat3>(Engine, "nat3", asOBJ_APP_CLASS_ALLINTS);
		RegisterType<iso3>(Engine, "iso3", asOBJ_APP_CLASS_ALLFLOATS);
		RegisterType<mat3>(Engine, "mat3", asOBJ_APP_CLASS_ALLFLOATS);
		RegisterType<iso4>(Engine, "iso4", asOBJ_APP_CLASS_ALLFLOATS);
		RegisterType<mat4>(Engine, "mat4", asOBJ_APP_CLASS_ALLFLOATS);
		RegisterType<quat>(Engine, "quat", asOBJ_APP_CLASS_ALLFLOATS);
	}

	void Registrar(asIScriptEngine* Engine)
	{
		RegisterAll(Engine, s_Exports);
	}
}
