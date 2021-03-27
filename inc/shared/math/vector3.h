// LICENSE HERE.

//
// shared/math/vector3.h
//
// N&C Math Library: Vector3
// 
// Functions that state LEGACY: should not be used, they have a viable more
// preferenced alternative to use.
//
#ifndef __INC__SHARED__MATH__VECTOR3_H__
#define __INC__SHARED__MATH__VECTOR3_H__

//-----------------
// Vector 3 type definiton. (X, Y, Z)
//
// The vector is implemented like a union class.
//-----------------
//typedef union {
//    // Xyz array index accessor.
//    float xyz[3];
//
//    // X Y Z desegnator accessor.
//    struct {
//        float x, y, z;
//    };
//} vec3_t;
template<typename T> struct vec3_template
{
    union
    {
            // XYZ array index accessor.
            T xyz[3];
        
            // X Y Z desegnator accessors.
            struct {
                T x, y, z;
            };
    };

    // OPERATOR: -= float
    const vec3_template& operator -=(const T& other) {
        x += other;
        y += other;
        z += other;
        return *this;
    }

    // OPERATOR: -= vec3_template
    const vec3_template& operator -=(const vec3_t& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    // OPERATOR: += float
    const vec3_template& operator +=(const T& other) {
        x += other;
        y += other;
        z += other;
        return *this;
    }

    // OPERATOR: += vec3_template
    const vec3_template& operator +=(const vec3_t& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
};
typedef vec3_template<byte> bvec3_t;
typedef vec3_template<int> ivec3_t;
typedef vec3_template<float> vec3_t;
typedef vec3_template<double> dvec3_t;

//union T
//{
//    int x;
//    int y;
//
//    T& operator=(const T& other)
//    {
//        x = other.x;
//        return *this;
//    }
//
//    bool operator==(const T& other)
//    {
//        return x == other.x;
//    }
//};

//
//===============
// Vec3_Dot
// 
// Returns the dot product of a DOT b
//===============
//
static inline float Vec3_Dot(const vec3_t &a, const vec3_t &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

//
//===============
// Vec3_Cross
// 
// Returns the cross product of a CROSS b
//===============
//
static inline vec3_t Vec3_Cross(const vec3_t &a, const vec3_t &b) {
    return vec3_t { 
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x 
    };
}

//
//===============
// LEGACY: Vec3_Cross_
// 
// Returns the cross product of a CROSS b
// 
// ALTERNATIVE: Vec3_Cross
//===============
//
static inline vec3_t Vec3_Cross_(const vec3_t& a, const vec3_t& b, vec3_t &c) {
    c = vec3_t{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

//
//===============
// Vec3_Subtract
// 
// Returns the vector difference of a - b
//===============
//
static inline vec3_t Vec3_Subtract(const vec3_t &a, const vec3_t &b) {
    return vec3_t {
        a.x - b.x, 
        a.y - b.y, 
        a.z - b.z 
    };
}

//
//===============
// LEGACY: Vec3_Subtract_
// 
// Returns the vector difference of a - b
// 
// ALTERNATIVE: Vec3_Subtract
//===============
//
static inline vec3_t Vec3_Subtract_(const vec3_t& a, const vec3_t& b, vec3_t &c) {
    c = vec3_t{
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
}

//
//===============
// Vec3_Add
// 
// Returns the vector of a + b
//===============
//
static inline vec3_t Vec3_Add(const vec3_t &a, const vec3_t &b) {
    return vec3_t {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
}

//
//===============
// LEGACY: Vec3_Add_
// 
// Returns the vector of a + b
// 
// ALTERNATIVE: Use Vec3_Add
//===============
//
static inline vec3_t Vec3_Add_(const vec3_t& a, const vec3_t& b, vec3_t &c) {
    c = vec3_t{
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
}


//
//===============
// Vec3_Add2
// 
// Returns the vector a + b
//===============
//
static inline vec3_t Vec3_Add2(const vec3_t &a, const vec3_t &b, const vec3_t& c) {
    return vec3_t {
        a.x + b.x + c.x,
        a.y + b.y + c.y,
        a.z + b.z + c.z
    };
}

//
//===============
// LEGACY: Vec3_Add2_
// 
// Returns the vector a + b
// 
// ALTERNATIVE: Use Vec3_Add2
//===============
//
static inline vec3_t Vec3_Add2_(const vec3_t& a, const vec3_t& b, const vec3_t& c, vec3_t &d) {
    d = vec3_t{
        a.x + b.x + c.x,
        a.y + b.y + c.y,
        a.z + b.z + c.z
    };
}

//
//===============
// Vec3_Scale
// 
// Returns the vector v scaled by scale.
//===============
//
static inline vec3_t Vec3_Scale(const vec3_t &v, float scale) {
    return vec3_t {
        v.x * scale,
        v.y * scale,
        v.z * scale 
    };
}

//
//===============
// LEGACY: Vec3_Scale_
// 
// ALTERNATIVE: Use Vec3_Scale
//===============
//
static inline vec3_t Vec3_Scale_(const vec3_t &in, float scale, vec3_t &out) {
    out = vec3_t {
        in.x * scale,
        in.y * scale,
        in.z * scale
    };
}

//
//===============
// Vec3_Zero
// 
// Returns a zero vec3_t vector.
//===============
//
static inline vec3_t Vec3_Zero() {
    return vec3_t { 
        0.f, 
        0.f, 
        0.f
    };
}

//
//===============
// LEGACY: Vec3_Clear
// 
// Clears the vector v
// 
// ALTERNATIVE: Use Vec3_Zero
//===============
//
static inline void Vec3_Clear(vec3_t& v) {
    v = { 0.f, 0.f, 0.f };
}

//
//===============
// Vec3_Negate
// 
// Returns the negated vector v
//===============
//
static inline vec3_t Vec3_Negate(const vec3_t &v) {
    return Vec3_Scale(v, -1.f);
}

//
//===============
// Vec3_Inverse
// 
// Inverses the referenced vector v
//===============
//
static inline void Vec3_Inverse(vec3_t& v) {
    v = Vec3_Scale(v, -1.f);
}

//
//===============
// Vec3_Average
// 
// Inverses the referenced vector v
//===============
//
static inline void Vec3_Negate(vec3_t& v) {
    v = Vec3_Scale(v, -1.f);
}

//
//===============
// LEGACY: Vec3_Copy_
// 
// Copies vector A into B.
// 
// ALTERNATIVE: Assign vectors using the = operator instead.
//===============
//
static inline void Vec3_Copy_(const vec3_t& a, vec3_t& b) {
    b.x = a.x;
    b.y = a.y;
    b.z = a.z;
}

//
//===============
// LEGACY: Vec3_Set_
// 
// Sets the vector's (X, Y, Z) component values.
// 
// ALTERNATIVE: Assign vectors using the = operator instead.
//===============
//
static inline void Vec3_Set_(vec3_t& v, float x, float y, float z) {
    v = { x, y, z };
}

//
//===============
// Vec3_Average
// 
// Returns the 'average' vector of (a + b * 0.5)
//===============
//
static inline vec3_t Vec3_Average(const vec3_t& a, const vec3_t& b) {
    return vec3_t{
        (a.x + b.x) * 0.5f,
        (a.y + b.y) * 0.5f,
        (a.z + b.z) * 0.5f
    };
}

//
//===============
// LEGACY: Vec3_Add_
// 
// ALTERNATIVE: Use Vec3_Average
//===============
//
static inline vec3_t Vec3_Average_(const vec3_t& a, const vec3_t& b, vec3_t &c) {
    c = vec3_t{
        (a.x + b.x) * 0.5f,
        (a.y + b.y) * 0.5f,
        (a.z + b.z) * 0.5f
    };
}

// Functions marked with:
// '==' are directly copied over.
// '//' got a legacy fallback function.
// '--' Now have a syntactic alternative, and are only there in legacy mode. _ subset
//#define Vec3_Dot(x,y)         ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])         ==================================
//#define Vec3_Cross(v1,v2,cross) \                                                 ///////////////
//        ((cross)[0]=(v1)[1]*(v2)[2]-(v1)[2]*(v2)[1], \
//         (cross)[1]=(v1)[2]*(v2)[0]-(v1)[0]*(v2)[2], \
//         (cross)[2]=(v1)[0]*(v2)[1]-(v1)[1]*(v2)[0])
//#define Vec3_Subtract(a,b,c) \                                                    /////////////
//        ((c)[0]=(a)[0]-(b)[0], \
//         (c)[1]=(a)[1]-(b)[1], \
//         (c)[2]=(a)[2]-(b)[2])
//#define Vec3_Add(a,b,c) \                                                         ////////////////
//        ((c)[0]=(a)[0]+(b)[0], \
//         (c)[1]=(a)[1]+(b)[1], \
//         (c)[2]=(a)[2]+(b)[2])
//#define Vec3_Add3(a,b,c,d) \                                                      ///////////////
//        ((d)[0]=(a)[0]+(b)[0]+(c)[0], \
//         (d)[1]=(a)[1]+(b)[1]+(c)[1], \
//         (d)[2]=(a)[2]+(b)[2]+(c)[2])
//#define Vec3_Copy(a,b)     ((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])            ----------------------------------------
//#define Vec3_Clear(a)      ((a)[0]=(a)[1]=(a)[2]=0)                               ========================
//#define Vec3_Negate(a,b)   ((b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2])         ========================
//#define Vec3_Inverse(a)    ((a)[0]=-(a)[0],(a)[1]=-(a)[1],(a)[2]=-(a)[2])         ===========================
//#define Vec3_Set(v, x, y, z)   ((v)[0]=(x),(v)[1]=(y),(v)[2]=(z))                 ----------------------------------------
//#define Vec3_Average(a,b,c) \                                                     //////////////////
//        ((c)[0]=((a)[0]+(b)[0])*0.5f, \
//         (c)[1]=((a)[1]+(b)[1])*0.5f, \
//         (c)[2]=((a)[2]+(b)[2])*0.5f)
//#define Vec3_MA(a,b,c,d) \
//        ((d)[0]=(a)[0]+(b)*(c)[0], \
//         (d)[1]=(a)[1]+(b)*(c)[1], \
//         (d)[2]=(a)[2]+(b)*(c)[2])
//#define VectorVectorMA(a,b,c,d) \
//        ((d)[0]=(a)[0]+(b)[0]*(c)[0], \
//         (d)[1]=(a)[1]+(b)[1]*(c)[1], \
//         (d)[2]=(a)[2]+(b)[2]*(c)[2])
//#define Vec3_Empty(v) ((v)[0]==0&&(v)[1]==0&&(v)[2]==0)
//#define Vec3_Compare(v1,v2)    ((v1)[0]==(v2)[0]&&(v1)[1]==(v2)[1]&&(v1)[2]==(v2)[2])
//#define Vec3_Length(v)     (sqrt(Vec3_Dot((v),(v))))
//#define Vec3_LengthSquared(v)      (Vec3_Dot((v),(v)))
//#define Vec3_Scale(in,scale,out) \                                            ============================================
//        ((out)[0]=(in)[0]*(scale), \
//         (out)[1]=(in)[1]*(scale), \
//         (out)[2]=(in)[2]*(scale))
//#define Vec3_ScaleVec3(in,scale,out) \
//        ((out)[0]=(in)[0]*(scale)[0], \
//         (out)[1]=(in)[1]*(scale)[1], \
//         (out)[2]=(in)[2]*(scale)[2])
//#define DistanceSquared(v1,v2) \
//        (((v1)[0]-(v2)[0])*((v1)[0]-(v2)[0])+ \
//        ((v1)[1]-(v2)[1])*((v1)[1]-(v2)[1])+ \
//        ((v1)[2]-(v2)[2])*((v1)[2]-(v2)[2]))
//#define Distance(v1,v2) (sqrt(DistanceSquared(v1,v2)))
//#define LerpAngles(a,b,c,d) \
//        ((d)[0]=LerpAngle((a)[0],(b)[0],c), \
//         (d)[1]=LerpAngle((a)[1],(b)[1],c), \
//         (d)[2]=LerpAngle((a)[2],(b)[2],c))
//#define Vec3_Lerp(a,b,c,d) \
//    ((d)[0]=(a)[0]+(c)*((b)[0]-(a)[0]), \
//     (d)[1]=(a)[1]+(c)*((b)[1]-(a)[1]), \
//     (d)[2]=(a)[2]+(c)*((b)[2]-(a)[2]))
//#define Vec3_Lerp2(a,b,c,d,e) \
//    ((e)[0]=(a)[0]*(c)+(b)[0]*(d), \
//     (e)[1]=(a)[1]*(c)+(b)[1]*(d), \
//     (e)[2]=(a)[2]*(c)+(b)[2]*(d))
//#define PlaneDiff(v,p)   (Vec3_Dot(v,(p)->normal)-(p)->dist)

#endif // __INC__SHARED__MATH__VECTOR3_H__