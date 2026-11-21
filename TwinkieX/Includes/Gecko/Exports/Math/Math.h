#pragma once
#include <Gecko/ScriptManager.h>

namespace Gecko::Exports::Math
{
	// The script's math value types.
	// They have no constructors, so they stay plain aggregates: AngelScript's native calls rely on that to pass and
	// return them the way the compiler does. Their layouts match the game's own types, so game members bind directly.

	struct vec2 { float x, y; };
	struct vec3 { float x, y, z; };
	struct vec4 { float x, y, z, w; };

	struct int2 { int32_t x, y; };
	struct int3 { int32_t x, y, z; };

	struct nat2 { uint32_t x, y; };
	struct nat3 { uint32_t x, y, z; };

	// A 2D rotation and scale (the x and y axes), then a translation
	struct iso3 { float xx, xy, yx, yy, tx, ty; };
	// A 3D rotation and scale (the x, y and z axes), then a translation
	struct iso4 { float xx, xy, xz, yx, yy, yz, zx, zy, zz, tx, ty, tz; };

	// Like glm, the vectors are the matrix's columns: a vector is transformed by M * v = x * v.x + y * v.y + z * v.z (+ t * v.w)
	struct mat3 { vec3 x, y, z; };
	struct mat4 { vec4 x, y, z, t; };

	struct quat { float x, y, z, w; };

	// Registers the types themselves. Must run before anything that uses the types, such as the game's classes.
	void RegisterTypes(asIScriptEngine*);

	// Registers everything else. Must run after the string type is registered.
	void Registrar(asIScriptEngine*);
}
