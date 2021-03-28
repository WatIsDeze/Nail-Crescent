// LICENSE HERE.

//
// shared/math/vector3.h
//
// N&C Math Library: Vector3
// 
// Functions that state LEGACY: should not be used, they have a viable more
// preferenced alternative to use.
//
#ifndef __INC_SHARED_MATH_VECTOR3_H__
#define __INC_SHARED_MATH_VECTOR3_H__

//-----------------
// Vector 3 type definiton. (X, Y, Z)
//
// The vector is implemented like a union class.
//-----------------
template<typename T> struct vec3_template {
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
    const vec3_template& operator -=(const float& other) {
        x += other;
        y += other;
        z += other;
        return *this;
    }

    // OPERATOR: -= vec3_template
    const vec3_template& operator -=(const vec3_template& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    // OPERATOR: += float
    const vec3_template& operator +=(const float& other) {
        x += other;
        y += other;
        z += other;
        return *this;
    }

    // OPERATOR: += vec3_template
    const vec3_template& operator +=(const vec3_template& other) {
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
static inline void Vec3_Cross_(const vec3_t& a, const vec3_t& b, vec3_t &c) {
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
static inline vec3_t Vec3_Subtract_(const vec3_t &a, const vec3_t &b) {
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
static inline void Vec3_Subtract_(const vec3_t& a, const vec3_t& b, vec3_t &c) {
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
static inline vec3_t Vec3_Add_(const vec3_t &a, const vec3_t &b) {
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
static inline void Vec3_Add_(const vec3_t& a, const vec3_t& b, vec3_t &c) {
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
static inline void Vec3_Add2_(const vec3_t& a, const vec3_t& b, const vec3_t& c, vec3_t &d) {
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
static inline vec3_t Vec3_Scale_(const vec3_t &v, float scale) {
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
static inline void Vec3_Scale_(const vec3_t &in, float scale, vec3_t &out) {
    out = vec3_t {
        in.x * scale,
        in.y * scale,
        in.z * scale
    };
}

//
//===============
// Vec3_ScaleVec3
// 
// Returns the vector v scaled by scale.
//===============
//
static inline vec3_t Vec3_ScaleVec3(const vec3_t& v, const vec3_t &scale) {
    return vec3_t{
        v.x * scale.x,
        v.y * scale.y,
        v.z * scale.z
    };
}

//
//===============
// LEGACY: Vec3_Scale_
// 
// ALTERNATIVE: Use Vec3_Scale
//===============
//
static inline void Vec3_Scale_(const vec3_t& in, const vec3_t &scale, vec3_t& out) {
    out = vec3_t{
        in.x * scale.x,
        in.y * scale.y,
        in.z * scale.z
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
    return Vec3_Scale_(v, -1.f);
}

//
//===============
// Vec3_Inverse
// 
// Inverses the referenced vector v
//===============
//
static inline void Vec3_Inverse(vec3_t& v) {
    v = Vec3_Scale_(v, -1.f);
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
// LEGACY: Vec3_Average_
// 
// ALTERNATIVE: Use Vec3_Average
//===============
//
static inline void Vec3_Average_(const vec3_t& a, const vec3_t& b, vec3_t &c) {
    c = vec3_t{
        (a.x + b.x) * 0.5f,
        (a.y + b.y) * 0.5f,
        (a.z + b.z) * 0.5f
    };
}

//
//===============
// Vec3_FmaF
// 
// Returns the vector v + (vector add * float multiply).
//===============
//
static inline vec3_t Vec3_FmaF(const vec3_t& add, const float &multiply, const vec3_t& v) {
    return vec3_t{
        std::fmaf(v.x, multiply, add.x),
        std::fmaf(v.y, multiply, add.y),
        std::fmaf(v.z, multiply, add.z)
    };
}

//
//===============
// LEGACY: Vec3_MA
// 
// ALTERNATIVE: Use Vec3_FmaF
//===============
//
static inline void Vec3_MA_(const vec3_t& add, const float &multiply, const vec3_t& v, vec3_t &out) {
    out = vec3_t{
        std::fmaf(v.x, multiply, add.x),
        std::fmaf(v.y, multiply, add.y),
        std::fmaf(v.z, multiply, add.z)
    };
}

//
//===============
// Vec3_VmaF
// 
// Returns the vector v + (vector add * vector multiply).
//===============
//
static inline vec3_t Vec3_VmaF(const vec3_t& add, const vec3_t& multiply, const vec3_t& v) {
    return vec3_t{
        std::fmaf(v.x, multiply.x, add.x),
        std::fmaf(v.y, multiply.y, add.y),
        std::fmaf(v.z, multiply.z, add.z)
    };
}

//
//===============
// Vec3_EqualEpsilon
// 
// Returns true if `a` and `b` are equal using the specified epsilon.
//===============
//
static inline bool Vec3_EqualEpsilon(const vec3_t &a, const vec3_t &b, float epsilon) {
    return EqualEpsilonf(a.x, b.x, epsilon) &&
        EqualEpsilonf(a.y, b.y, epsilon) &&
        EqualEpsilonf(a.z, b.z, epsilon);
}

//
//===============
// Vec3_Equal
// 
// Returns true if `a` and `b` are equal.
//===============
//
static inline bool Vec3_Equal(const vec3_t &a, const vec3_t &b) {
    return Vec3_EqualEpsilon(a, b, FLT_EPSILON);
}

//
//===============
// LEGACY: Vec3_MA
// 
// ALTERNATIVE: Use Vec3_FmaF
//===============
//
static inline void Vec3_VectorMA_(const vec3_t& add, const vec3_t& multiply, const vec3_t& v, vec3_t& out) {
    out = vec3_t{
        std::fmaf(v.x, multiply.x, add.x),
        std::fmaf(v.y, multiply.y, add.y),
        std::fmaf(v.z, multiply.z, add.z)
    };
}

//
//===============
// LEGACY: Vec3_Empty_
// 
// ALTERNATIVE: Use Vec3_Empty
//===============
//
static inline bool Vec3_Empty_(const vec3_t& v) {
    return v.x == 0 && v.y == 0 && v.z == 0;
}

//
//===============
// LEGACY: Vec3_Compare_
// 
// ALTERNATIVE: Use Vec3_Compare
//===============
//
static inline bool Vec3_Compare_(const vec3_t& v1, const  vec3_t &v2) {
    return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
}

//
//===============
// LEGACY: Vec3_Length_
// 
// ALTERNATIVE: Use Vec3_Length
//===============
//
static inline vec_t Vec3_Length_(const vec3_t& v) {
    return std::sqrtf(Vec3_Dot(v, v));
}

//
//===============
// LEGACY: Vec3_Length_
// 
// ALTERNATIVE: Use Vec3_Length
//===============
//
static inline vec_t Vec3_LengthSquared_(const vec3_t& v) {
    return Vec3_Dot(v, v);
}

//
//===============
// LEGACY: DistanceSquared_
// 
// ALTERNATIVE: Use Vec3_DistanceSquared
//===============
//
static inline vec_t DistanceSquared_(const vec3_t& v1, const vec3_t& v2) {
    return  ((v1.x - v2.x) * (v1.x - v2.x) +
            (v1.y - v2.y) * (v1.y - v2.y) +
            (v1.z - v2.z) * (v1.z - v2.z));
}

//
//===============
// LEGACY: Distance_
// 
// ALTERNATIVE: Use Vec3_Distance
//===============
//
static inline vec_t Distance_(const vec3_t& v1, const vec3_t& v2) {
    return std::sqrtf(DistanceSquared_(v1, v2));
}

//
//===============
// LEGACY: LerpAngles_
// 
// ALTERNATIVE: Use Vec3_LerpAngles
//===============
//
static inline void LerpAngles_(const vec3_t& a, const vec3_t& b, const float &c, vec3_t &out) {
    out.x = LerpAngle(a.x, b.x, c);
    out.y = LerpAngle(a.y, b.y, c);
    out.z = LerpAngle(a.z, b.z, c);
}

//
//===============
// Vec3_Lerp
// 
// Returns the LERPED vector between a and b, with fraction of c.
//===============
//
static inline vec3_t Vec3_Lerp(const vec3_t& a, const vec3_t& b, const float& c) {
    return vec3_t {
        a.x + c * (b.x - a.x),
        a.y + c * (b.y - a.y),
        a.z + c * (b.z - a.z)
    };
}

//
//===============
// LEGACY: Vec3_Lerp_
// 
// ALTERNATIVE: Use Vec3_Lerp
//===============
//
static inline void Vec3_Lerp_(const vec3_t& a, const vec3_t& b, const float& c, vec3_t& out) {
    out.x = a.x + c * (b.x - a.x);
    out.y = a.y + c * (b.y - a.y);
    out.z = a.z + c * (b.z - a.z);
}

//
//===============
// Vec3_Lerp2
// 
//===============
//
static inline vec3_t Vec3_Lerp2(const vec3_t& a, const vec3_t& b, const float& c, const float& d) {
    return vec3_t {
        a.x * c + b.x * d,
        a.y * c + b.y * d,
        a.z * c + b.z * d
    };
}

//
//===============
// LEGACY: Vec3_Lerp2_
// 
// ALTERNATIVE: Use Vec3_Lerp2
//===============
//
static inline void Vec3_Lerp2(const vec3_t& a, const vec3_t& b, const float& c, const float &d, vec3_t& out) {
    out.x = a.x * c + b.x * d;
    out.y = a.y * c + b.y * d;
    out.z = a.z * c + b.z * d;
}


//---------------------------------------------------------------------------------

void SetupRotationMatrix(vec3_t *matrix, const vec3_t &dir, float degrees);
void RotatePointAroundVector(vec3_t &dst, const vec3_t &dir, const vec3_t &point, float degrees);
void ProjectPointOnPlane(vec3_t &dst, const vec3_t &p, const vec3_t &normal);
void PerpendicularVector(vec3_t &dst, const vec3_t &src);

void AngleVectors(const vec3_t& angles, vec3_t* forward, vec3_t* right, vec3_t* up);
vec_t VectorNormalize(vec3_t& v);        // returns vector length
vec_t VectorNormalize2(const vec3_t& v, vec3_t& out);
void ClearBounds(vec3_t& mins, vec3_t& maxs);
void AddPointToBounds(const vec3_t& v, vec3_t& mins, vec3_t& maxs);
vec_t RadiusFromBounds(const vec3_t& mins, const vec3_t& maxs);
void UnionBounds(vec3_t* a[2], vec3_t* b[2], vec3_t* c[2]);


void vectoangles2(const vec3_t& value1, vec3_t& angles);

void MakeNormalVectors(const vec3_t& forward, vec3_t& right, vec3_t& up);

int DirToByte(const vec3_t &dir);
//void ByteToDir(int index, vec3_t dir);
// 
//
//===============
// AnglesToAxis
// 
// Set angles to axis.
//===============
//
static inline void AnglesToAxis(const vec3_t& angles, vec3_t* axis)
{
    AngleVectors(angles, &axis[0], &axis[1], &axis[2]);
    Vec3_Inverse(axis[1]);
}

//
//===============
// TransposeAxis
// 
// Transpoes the vector axis.
//===============
//
static inline void TransposeAxis(vec3_t* axis)
{
    vec_t temp;

    temp = axis[0].xyz[1];
    axis[0].xyz[1] = axis[1].xyz[0];
    axis[1].xyz[0] = temp;

    temp = axis[0].xyz[2];
    axis[0].xyz[2] = axis[2].xyz[0];
    axis[2].xyz[0] = temp;

    temp = axis[1].xyz[2];
    axis[1].xyz[2] = axis[2].xyz[1];
    axis[2].xyz[1] = temp;
}

//
//===============
// RotatePoint
// 
// Rotate point around axis.
//===============
//
static inline void RotatePoint(vec3_t& point, vec3_t* axis)
{
    vec3_t temp;

    Vec3_Copy_(point, temp);
    point.xyz[0] = Vec3_Dot(temp, axis[0]);
    point.xyz[1] = Vec3_Dot(temp, axis[1]);
    point.xyz[2] = Vec3_Dot(temp, axis[2]);
}

//#define Vec3_Lerp(a,b,c,d) \
//    ((d)[0]=(a)[0]+(c)*((b)[0]-(a)[0]), \
//     (d)[1]=(a)[1]+(c)*((b)[1]-(a)[1]), \
//     (d)[2]=(a)[2]+(c)*((b)[2]-(a)[2]))
//#define Vec3_Lerp2(a,b,c,d,e) \
//    ((e)[0]=(a)[0]*(c)+(b)[0]*(d), \
//     (e)[1]=(a)[1]*(c)+(b)[1]*(d), \
//     (e)[2]=(a)[2]*(c)+(b)[2]*(d))
//#define PlaneDiff(v,p)   (Vec3_Dot(v,(p)->normal)-(p)->dist)

//#define LerpAngles(a,b,c,d) \
//        ((d)[0]=LerpAngle((a)[0],(b)[0],c), \
//         (d)[1]=LerpAngle((a)[1],(b)[1],c), \
//         (d)[2]=LerpAngle((a)[2],(b)[2],c))
// 
// Functions marked with:
// '==' are directly copied over.
// '//' got a legacy fallback function.
// '--' Now have a syntactic alternative, and are only there in legacy mode. _ subset
//#define Vec3_Dot(x,y)         ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])         ==================================
//#define Vec3_Cross(v1,v2,cross) \                                                 ///////////////
//        ((cross)[0]=(v1)[1]*(v2)[2]-(v1)[2]*(v2)[1], \
//         (cross)[1]=(v1)[2]*(v2)[0]-(v1)[0]*(v2)[2], \
//         (cross)[2]=(v1)[0]*(v2)[1]-(v1)[1]*(v2)[0])
//#define Vec3_Subtract_(a,b,c) \                                                    /////////////
//        ((c)[0]=(a)[0]-(b)[0], \
//         (c)[1]=(a)[1]-(b)[1], \
//         (c)[2]=(a)[2]-(b)[2])
//#define Vec3_Add_(a,b,c) \                                                         ////////////////
//        ((c)[0]=(a)[0]+(b)[0], \
//         (c)[1]=(a)[1]+(b)[1], \
//         (c)[2]=(a)[2]+(b)[2])
//#define Vec3_Add3(a,b,c,d) \                                                      ///////////////
//        ((d)[0]=(a)[0]+(b)[0]+(c)[0], \
//         (d)[1]=(a)[1]+(b)[1]+(c)[1], \
//         (d)[2]=(a)[2]+(b)[2]+(c)[2])
//#define Vec3_Copy_(a,b)     ((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])            ----------------------------------------
//#define Vec3_Clear(a)      ((a)[0]=(a)[1]=(a)[2]=0)                               ========================
//#define Vec3_Negate(a,b)   ((b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2])         ========================
//#define Vec3_Inverse(a)    ((a)[0]=-(a)[0],(a)[1]=-(a)[1],(a)[2]=-(a)[2])         ===========================
//#define Vec3_Set_(v, x, y, z)   ((v)[0]=(x),(v)[1]=(y),(v)[2]=(z))                 ----------------------------------------
//#define Vec3_Average(a,b,c) \                                                     //////////////////
//        ((c)[0]=((a)[0]+(b)[0])*0.5f, \
//         (c)[1]=((a)[1]+(b)[1])*0.5f, \
//         (c)[2]=((a)[2]+(b)[2])*0.5f)
//#define Vec3_MA_(a,b,c,d) \                                                        ////
//        ((d)[0]=(a)[0]+(b)*(c)[0], \
//         (d)[1]=(a)[1]+(b)*(c)[1], \
//         (d)[2]=(a)[2]+(b)*(c)[2])
//#define VectorVectorMA(a,b,c,d) \                                                 /////
//        ((d)[0]=(a)[0]+(b)[0]*(c)[0], \
//         (d)[1]=(a)[1]+(b)[1]*(c)[1], \
//         (d)[2]=(a)[2]+(b)[2]*(c)[2])
//#define Vec3_Empty(v) ((v)[0]==0&&(v)[1]==0&&(v)[2]==0)
//#define Vec3_Compare(v1,v2)    ((v1)[0]==(v2)[0]&&(v1)[1]==(v2)[1]&&(v1)[2]==(v2)[2]) ////////////
//#define Vec3_Length(v)     (sqrt(Vec3_Dot((v),(v))))
//#define Vec3_LengthSquared(v)      (Vec3_Dot((v),(v)))
//#define Vec3_Scale_(in,scale,out) \                                                ////////////////
//        ((out)[0]=(in)[0]*(scale), \
//         (out)[1]=(in)[1]*(scale), \
//         (out)[2]=(in)[2]*(scale))
//#define Vec3_ScaleVec3(in,scale,out) \                                            ///////////////
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