// LICENSE HERE.

//
// shared/math/vector3.h
//
// N&C Math Library: Vector3
//
#ifndef __INC__SHARED__MATH__VECTOR3_H__
#define __INC__SHARED__MATH__VECTOR3_H__

// Vector 3 type definiton. (X, Y, Z)
//typedef vec_t vec3_t[3];

//typedef struct vec3_s {
//	vec3_s() {
//		 xyz = &v.xyz[0];
//	}
//
//	// The actual vector member data.
//	union {
//		float xyz[3];
//		struct {
//			float x, y, z;
//		};
//	} v;
//
//	float* xyz;
//} vec3_t;
// TODO: Implement this method of vec3_t types.
typedef union {
    // Xyz array index accessor.
    float xyz[3];

    // X Y Z desegnator accessor.
    struct {
        float x, y, z;
    };
} vec3_t;

#endif // __INC__SHARED__MATH__VECTOR3_H__