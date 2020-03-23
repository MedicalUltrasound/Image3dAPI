/* Basic Ultrasound Image library (UsImage).
Designed by Fredrik Orderud <fredrik.orderud@ge.com>
Copyright (c) 2015, GE Vingmed Ultrasound            */
#pragma once
#include <array>
#include <cassert>
#include <cmath>
#include <tuple>


/** 3D vector type. */
struct vec3f {
    vec3f() : x(0), y(0), z(0) {
    }
    vec3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {
    }

    vec3f operator + (vec3f other) const {
        return vec3f(x + other.x, y + other.y, z + other.z);
    }
    vec3f operator - (vec3f other) const {
        return vec3f(x - other.x, y - other.y, z - other.z);
    }

    vec3f operator - () const {
        return vec3f(-x, -y, -z);
    }

    vec3f & operator *= (float val) {
        x *= val;
        y *= val;
        z *= val;
        return *this;
    }
    vec3f & operator /= (float val) {
        x /= val;
        y /= val;
        z /= val;
        return *this;
    }

    vec3f & operator += (vec3f val) {
        x += val.x;
        y += val.y;
        z += val.z;
        return *this;
    }
    vec3f & operator -= (vec3f val) {
        x -= val.x;
        y -= val.y;
        z -= val.z;
        return *this;
    }

    bool operator == (vec3f other) const {
        if ((x == other.x) && (y == other.y) && (z == other.z))
            return true;
        return false;
    }
    bool operator != (vec3f other) const {
        return !operator==(other);
    }

    float x, y, z;
};

static vec3f operator * (float val, vec3f vec) {
    return vec3f(val*vec.x, val*vec.y, val*vec.z);
}

/** Calculates the vector cross-product */
static inline vec3f cross_prod(vec3f a, vec3f b) {
    return vec3f(a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x);
}


/** Calculates the vector dot-product */
static inline float dot_prod(vec3f a, vec3f b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}


/** Calculates the Eucledian length of a vector. */
static inline float length(vec3f vec) {
    return sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
}


/** Returns the input vector normalized to unit length. */
static inline vec3f normalize(vec3f vec) {
    vec3f result = vec;
    float norm = length(result);

    if (norm == 0.0f)
        return result;

    result.x /= norm;
    result.y /= norm;
    result.z /= norm;

    return result;
}


/** 3x3 matrix type. */
class mat33f {
public:
    mat33f() {
        clear();
    }

    float & operator () (size_t i, size_t j) {
        assert(i < 3 && j < 3);
        return m_data[i + 3 * j];
    }
    const float & operator () (size_t i, size_t j) const {
        assert(i < 3 && j < 3);
        return m_data[i + 3 * j];
    }

    void clear() {
        m_data.fill(0);
    }

    const float * data() const {
        return m_data.data();
    }

    void transpose() {
        static_assert(3 == 3, "only transpose of square matrices is supported");

        // make temporary matrix
        mat33f t;
        for (unsigned int i = 0; i < 3; ++i)
            for (unsigned int j = 0; j < 3; ++j)
                t(j, i) = (*this)(i, j);

        // copy to "this"
        (*this) = t;
    }

private:
    std::array<float, 3*3> m_data;
};

static inline void operator *= (mat33f & m, float val) {
    for (size_t j = 0; j < 3; j++)
        for (size_t i = 0; i < 3; i++)
            m(i, j) *= val;
}

/** Assign a row to a matrix. */
static inline void row_assign(mat33f & m, size_t i, const vec3f & val) {
    m(i, 0) = val.x;
    m(i, 1) = val.y;
    m(i, 2) = val.z;
}

/** Assign a column to a matrix. */
static inline void col_assign(mat33f & m, size_t i, const vec3f & val) {
    m(0, i) = val.x;
    m(1, i) = val.y;
    m(2, i) = val.z;
}


/** Determinant of a 3 x 3 matrix. */
static inline float det(const mat33f & M) {
    float a = M(0, 0), b = M(0, 1), c = M(0, 2);
    float d = M(1, 0), e = M(1, 1), f = M(1, 2);
    float g = M(2, 0), h = M(2, 1), i = M(2, 2);

    float det = a*(e*i - f*h) - b*(d*i - f*g) + c*(d*h - e*g);
    return det;
}

/** Inverts a 3 x 3 matrix analytically. */
static inline mat33f inv(const mat33f & M, bool normalize = true) {
    float a = M(0, 0), b = M(0, 1), c = M(0, 2);
    float d = M(1, 0), e = M(1, 1), f = M(1, 2);
    float g = M(2, 0), h = M(2, 1), i = M(2, 2);

    mat33f invM;
    invM(0, 0) = e*i - f*h;
    invM(0, 1) = c*h - b*i;
    invM(0, 2) = b*f - c*e;
    invM(1, 0) = f*g - d*i;
    invM(1, 1) = a*i - c*g;
    invM(1, 2) = c*d - a*f;
    invM(2, 0) = d*h - e*g;
    invM(2, 1) = b*g - a*h;
    invM(2, 2) = a*e - b*d;

    if (normalize) {
        invM *= 1.0f / det(M);
    }

    return invM;
}


/** Matrix vector product. */
static inline vec3f prod(const mat33f & m, const vec3f & p) {
    vec3f sum(0, 0, 0);
    sum += p.x * vec3f(m(0, 0), m(1, 0), m(2, 0));
    sum += p.y * vec3f(m(0, 1), m(1, 1), m(2, 1));
    sum += p.z * vec3f(m(0, 2), m(1, 2), m(2, 2));
    return sum;
}


/** Convert from normalized voxel pos in [0,1) to (x,y,z) coordinate. */
static vec3f PosToCoord (vec3f origin, vec3f dir1, vec3f dir2, vec3f dir3, const vec3f pos) {
    mat33f M;
    col_assign(M, 0, dir1);
    col_assign(M, 1, dir2);
    col_assign(M, 2, dir3);

    return prod(M, pos) + origin;
}


static std::tuple<vec3f, vec3f, vec3f, vec3f> FromCart3dGeom (Cart3dGeom geom) {
    const vec3f origin(geom.origin_x, geom.origin_y, geom.origin_z);
    const vec3f dir1(geom.dir1_x, geom.dir1_y, geom.dir1_z);
    const vec3f dir2(geom.dir2_x, geom.dir2_y, geom.dir2_z);
    const vec3f dir3(geom.dir3_x, geom.dir3_y, geom.dir3_z);

    return std::make_tuple(origin, dir1, dir2, dir3);
}


static Cart3dGeom ToCart3dGeom (vec3f origin, vec3f dir1, vec3f dir2, vec3f dir3) {
    Cart3dGeom geom = {};
    {
        geom.origin_x = origin.x;
        geom.origin_y = origin.y;
        geom.origin_z = origin.z;

        geom.dir1_x = dir1.x;
        geom.dir1_y = dir1.y;
        geom.dir1_z = dir1.z;

        geom.dir2_x = dir2.x;
        geom.dir2_y = dir2.y;
        geom.dir2_z = dir2.z;

        geom.dir3_x = dir3.x;
        geom.dir3_y = dir3.y;
        geom.dir3_z = dir3.z;
    }
    return geom;
}


/** Convert from (x,y,z) coordinate to normalized voxel pos in [0,1). */
static vec3f CoordToPos (Cart3dGeom geom, const vec3f xyz) {
    vec3f origin, dir1, dir2, dir3;
    std::tie(origin, dir1, dir2, dir3) = FromCart3dGeom(geom);

    mat33f M;
    col_assign(M, 0, dir1);
    col_assign(M, 1, dir2);
    col_assign(M, 2, dir3);

    return prod(inv(M), xyz - origin);
}


template <class T>
static T SampleVoxel (const Image3d & frame, const vec3f pos) {
    assert(ImageFormatSize(frame.format) == sizeof(T));

    // out-of-bounds checking
    if ((pos.x < 0) || (pos.y < 0) || (pos.z < 0))
        return OUTSIDE_VAL;

    auto x = static_cast<unsigned int>(frame.dims[0] * pos.x);
    auto y = static_cast<unsigned int>(frame.dims[1] * pos.y);
    auto z = static_cast<unsigned int>(frame.dims[2] * pos.z);

    // out-of-bounds checking
    if ((x >= frame.dims[0]) || (y >= frame.dims[1]) || (z >= frame.dims[2]))
        return OUTSIDE_VAL;

    return static_cast<T*>(frame.data->pvData)[x + y*frame.stride0/sizeof(T) + z*frame.stride1/sizeof(T)];
}


template <class T>
static Image3d SampleFrame (const Image3d & frame, Cart3dGeom frame_geom, Cart3dGeom out_geom, unsigned short max_res[3]) {
    if (max_res[2] == 0)
        max_res[2] = 1; // require at least one plane to to retrieved

    vec3f out_origin, out_dir1, out_dir2, out_dir3;
    std::tie(out_origin, out_dir1, out_dir2, out_dir3) = FromCart3dGeom(out_geom);

    // allow 3rd axis to be empty if only retrieving a single slice
    if ((out_dir3 == vec3f(0, 0, 0)) && (max_res[2] < 2))
        out_dir3 = cross_prod(out_dir1, out_dir2);

    // sample image buffer
    std::vector<unsigned char> img_buf(sizeof(T) * max_res[0] * max_res[1] * max_res[2], 127);
    for (unsigned short z = 0; z < max_res[2]; ++z) {
        for (unsigned short y = 0; y < max_res[1]; ++y) {
            for (unsigned short x = 0; x < max_res[0]; ++x) {
                // convert from input texture coordinate to output texture coordinate
                vec3f pos_in(x*1.0f/max_res[0], y*1.0f/max_res[1], z*1.0f/max_res[2]);
                vec3f xyz = PosToCoord(out_origin, out_dir1, out_dir2, out_dir3, pos_in);
                vec3f pos_out = CoordToPos(frame_geom, xyz);

                T val = SampleVoxel<T>(frame, pos_out);
                reinterpret_cast<T*>(img_buf.data())[x + y*max_res[0] + z*max_res[0] * max_res[1]] = val;
            }
        }
    }

    return CreateImage3d(frame.time, frame.format, max_res, img_buf);
}
