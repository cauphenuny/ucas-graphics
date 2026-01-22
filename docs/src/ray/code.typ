#import "../preamble.typ": *
= 光线追踪代码详解

本章节详细介绍光线追踪项目的核心代码实现，从基础架构到高级特性，分为三个部分：基础路径追踪器、性能优化与复杂特性、蒙特卡洛积分与重要性采样。

== 第一部分：基础架构与路径追踪器

=== 基础数学：Vec3 向量运算

向量是光线追踪的基础数据结构，用于表示位置、方向、颜色等。我们的实现提供了完整的三维向量运算支持。

```cpp
class Vec3 {
    double d[3];

public:
    Vec3() = default;
    Vec3(double x, double y, double z) { d[0] = x, d[1] = y, d[2] = z; }
    
    double x() const { return d[0]; }
    double y() const { return d[1]; }
    double z() const { return d[2]; }
    
    // 基本运算符重载
    Vec3 operator-() const { return Vec3{-x(), -y(), -z()}; }
    Vec3& operator+=(const Vec3& v) {
        d[0] += v.x(); d[1] += v.y(); d[2] += v.z();
        return *this;
    }
    
    // 向量模长
    double sqrnorm() const { return x() * x() + y() * y() + z() * z(); }
    double norm() const { return std::sqrt(sqrnorm()); }
    Vec3 normalized() const { return *this / norm(); }
};
```

*核心功能：*
- 基本算术运算（加减乘除）
- 点积与叉积运算
- 向量归一化
- 随机向量生成（用于光线散射）

```cpp
// 向量运算
inline double dot(const Vec3& u, const Vec3& v) {
    return u.x() * v.x() + u.y() * v.y() + u.z() * v.z();
}

inline Vec3 cross(const Vec3& u, const Vec3& v) {
    return Vec3(
        u.y() * v.z() - u.z() * v.y(),
        u.z() * v.x() - u.x() * v.z(),
        u.x() * v.y() - u.y() * v.x()
    );
}

// 反射与折射
inline Vec3 reflect(const Vec3& v, const Vec3& n) {
    return v - 2 * dot(v, n) * n;
}

inline Vec3 refract(const Vec3& uv, const Vec3& n, double etai_over_etat) {
    auto cos_theta = std::fmin(dot(-uv, n), 1.0);
    Vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
    Vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.sqrnorm())) * n;
    return r_out_perp + r_out_parallel;
}
```

*随机采样功能：*

```cpp
// 单位球面上的随机点（重要性采样）
static Vec3 random_unit() {
    while (true) {
        auto p = Vec3::random(-1, 1);
        auto sqrnorm = p.sqrnorm();
        if (1e-10 < sqrnorm && sqrnorm < 1) {
            return p * (1.0 / std::sqrt(sqrnorm));
        }
    }
}

// 余弦加权采样（用于漫反射）
static Vec3 random_cosine_z() {
    auto r1 = random_double();
    auto r2 = random_double();
    auto phi = 2 * pi * r1;
    auto x = std::cos(phi) * std::sqrt(r2);
    auto y = std::sin(phi) * std::sqrt(r2);
    auto z = std::sqrt(1 - r2);
    return Vec3(x, y, z);
}
```

#split-semi

=== 光线与相机

==== Ray：光线类

光线是路径追踪的核心概念，由起点、方向和时间组成。

```cpp
class Ray {
    Point3 orig;
    Vec3 dir;
    double tm;

public:
    Ray(const Point3& origin, const Vec3& direction, double time = 0)
        : orig(origin), dir(direction), tm(time) {}

    const Point3& origin() const { return orig; }
    const Vec3& direction() const { return dir; }
    Point3 at(double t) const { return orig + dir * t; }
    double time() const { return tm; }
};
```

光线的参数方程：$bold(P)(t) = bold(O) + t bold(D)$，其中 $bold(O)$ 是起点，$bold(D)$ 是方向，$t$ 是参数。

// 图片占位：光线示意图
// #image("path/to/ray_diagram.png", width: 60%)

==== Camera：相机类

相机负责生成视角光线，支持视场角、景深、散焦模糊等特性。

```cpp
struct Camera {
    double aspect_ratio = 1.0;
    int image_width = 100;
    int samples_per_pixel = 10;
    int max_depth = 10;
    Color background = Color(0.7, 0.8, 1);

    double vfov = 90.0;           // 垂直视场角
    Point3 lookfrom = Point3(0, 0, 0);
    Point3 lookat = Point3(0, 0, -1);
    Vec3 vup = Vec3(0, 1, 0);

    double defocus_angle = 0;      // 散焦角度
    double focus_dist = 10;        // 焦距
};
```

*相机初始化：*

```cpp
void initialize() {
    image_height = std::max(static_cast<int>(image_width / aspect_ratio), 1);
    center = lookfrom;

    // 计算视口尺寸
    auto theta = degrees_to_radians(vfov);
    auto h = std::tan(theta / 2);
    auto viewport_height = 2.0 * focus_dist * h;
    auto viewport_width = ratio * viewport_height;

    // 建立相机坐标系
    w = (lookfrom - lookat).normalized();  // 向后
    u = cross(vup, w).normalized();         // 向右
    v = cross(w, u);                        // 向上

    // 计算像素间距
    pixel_delta_u = viewport_u / image_width;
    pixel_delta_v = viewport_v / image_height;
}
```

// 图片占位：相机坐标系示意图
// #image("path/to/camera_coordinate.png", width: 70%)

#split-semi

=== 几何形状

==== Hittable 抽象基类

所有几何体都继承自 `Hittable` 接口，提供光线相交测试和包围盒计算。

```cpp
struct HitResult {
    Point3 p;              // 交点位置
    Vec3 normal;           // 表面法线
    double t;              // 光线参数
    bool front_face;       // 是否为正面
    std::shared_ptr<Material> mat;
    double u, v;           // 纹理坐标

    void set_face_normal(const Ray& r, const Vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class Hittable {
public:
    virtual ~Hittable() = default;
    virtual bool hit(const Ray& ray, Interval interval, HitResult& result) const = 0;
    virtual BoundingBox bounding_box() const = 0;
};
```

==== Sphere：球体

球体是最基础的几何形状，支持静态和运动两种模式。

```cpp
class Sphere : public Hittable {
    double radius;
    Ray center;  // 支持运动：起点和方向定义运动轨迹
    std::shared_ptr<Material> mat;
    BoundingBox bbox;

public:
    // 运动球体构造函数
    Sphere(const Point3& center_start, const Point3& center_end,
           double radius, std::shared_ptr<Material> mat)
        : center(center_start, center_end - center_start),
          radius(std::max(0., radius)), mat(mat) {
        // 计算动态包围盒
        auto rvec = Vec3(radius, radius, radius);
        auto box0 = BoundingBox::diag(center.at(0) - rvec, center.at(0) + rvec);
        auto box1 = BoundingBox::diag(center.at(1) - rvec, center.at(1) + rvec);
        bbox = BoundingBox::combine(box0, box1);
    }
};
```

*球体光线相交：*

求解方程：$||bold(P)(t) - bold(C)||^2 = r^2$

```cpp
bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
    auto current_center = center.at(ray.time());
    auto oc = current_center - ray.origin();
    auto a = ray.direction().sqrnorm();
    auto h = dot(ray.direction(), oc);
    auto c = oc.sqrnorm() - radius * radius;
    auto discriminant = h * h - a * c;
    
    if (discriminant < 0) return false;
    
    auto sqrtd = std::sqrt(discriminant);
    auto root = (h - sqrtd) / a;
    if (!interval.surrounds(root)) {
        root = (h + sqrtd) / a;
        if (!interval.surrounds(root)) return false;
    }
    
    result.t = root;
    result.p = ray.at(root);
    Vec3 outward_normal = (result.p - current_center) / radius;
    result.set_face_normal(ray, outward_normal);
    result.mat = mat;
    return true;
}
```

*球面UV映射：*

```cpp
static auto sphere_uv(const Point3& p) {
    // p: 单位球面上的点
    // u: [0,1] 水平角度
    // v: [0,1] 垂直角度
    auto theta = std::acos(-p.y());
    auto phi = std::atan2(-p.z(), p.x()) + pi;
    auto u = phi / (2 * pi);
    auto v = theta / pi;
    return std::make_tuple(u, v);
}
```

// 图片占位：球体UV映射示意图
// #image("path/to/sphere_uv.png", width: 60%)

#split-semi

=== 基础材质

材质定义了光线与物体表面的交互方式，包括散射和发光。

```cpp
struct ScatterResult {
    Color attenuation;  // 衰减系数
    std::variant<std::unique_ptr<Vec3PDF>, Ray> scattered;  // 散射光线或PDF
};

class Material {
public:
    virtual bool scatter(const Ray& r_in, const HitResult& hit, 
                        ScatterResult& result) const { return false; }
    virtual Color emit(const Ray& r_in, const HitResult& hit) const {
        return Color(0, 0, 0);
    }
    virtual double scattering_pdf(const Ray& r_in, const HitResult& hit,
                                 const Ray& scattered) const { return 1.0; }
};
```

==== Lambertian：漫反射材质

理想漫反射表面，反射光线均匀分布在半球面上，遵循余弦定律。

```cpp
class Lambertian : public Material {
    std::shared_ptr<Texture> tex;

public:
    Lambertian(const Color& albedo) 
        : tex(std::make_shared<ColorTexture>(albedo)) {}

    bool scatter(const Ray& r_in, const HitResult& hit, 
                ScatterResult& result) const override {
        result.scattered = std::make_unique<CosinePDF>(hit.normal);
        result.attenuation = tex->value(hit.u, hit.v, hit.p);
        return true;
    }

    double scattering_pdf(const Ray& r_in, const HitResult& hit,
                         const Ray& scattered) const override {
        auto cosine = dot(hit.normal, scattered.direction().normalized());
        return (cosine < 0) ? 0.0 : (cosine / pi);
    }
};
```

散射概率密度函数：$p(omega) = (cos theta) / pi$

==== Metal：金属材质

完美镜面反射，支持模糊参数（fuzz）模拟粗糙表面。

```cpp
class Metal : public Material {
    Color albedo;
    double fuzz;  // 模糊系数

public:
    Metal(const Color& albedo, double fuzz = 0) : albedo(albedo), fuzz(fuzz) {}
    
    bool scatter(const Ray& r_in, const HitResult& hit,
                ScatterResult& result) const override {
        Vec3 reflected = reflect(r_in.direction().normalized(), hit.normal)
                        .normalized() + (fuzz * Vec3::random_unit());
        result.scattered = Ray(hit.p, reflected, r_in.time());
        result.attenuation = albedo;
        return dot(reflected, hit.normal) > 0;
    }
};
```

反射方向：$bold(R) = bold(D) - 2(bold(D) dot bold(N))bold(N)$

==== Dielectric：电介质材质

透明材质，支持折射和反射，使用 Schlick 近似计算菲涅尔反射率。

```cpp
class Dielectric : public Material {
    double refraction_index;

public:
    Dielectric(double ri) : refraction_index(ri) {}

    bool scatter(const Ray& r_in, const HitResult& hit,
                ScatterResult& result) const override {
        result.attenuation = Color(1.0, 1.0, 1.0);
        double etai_over_etat = hit.front_face ? 
            (1.0 / refraction_index) : refraction_index;

        Vec3 unit_direction = r_in.direction().normalized();
        double cos_theta = std::fmin(dot(-unit_direction, hit.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = etai_over_etat * sin_theta > 1.0;
        Vec3 direction;

        if (cannot_refract || reflectance(cos_theta, etai_over_etat) > random_double()) {
            direction = reflect(unit_direction, hit.normal);
        } else {
            direction = refract(unit_direction, hit.normal, etai_over_etat);
        }

        result.scattered = Ray(hit.p, direction, r_in.time());
        return true;
    }

    static double reflectance(double cosine, double ref_idx) {
        // Schlick 近似
        auto r0 = (1 - ref_idx) / (1 + ref_idx);
        r0 = r0 * r0;
        return r0 + (1 - r0) * std::pow((1 - cosine), 5);
    }
};
```

// 图片占位：材质类型对比图
// #image("path/to/material_comparison.png", width: 100%)

#split-semi

=== 渲染管线

==== 递归路径追踪

核心渲染函数采用递归方式追踪光线，直到达到最大深度或击中光源。

```cpp
auto ray_color(const Ray& ray, const Hittable& world, 
              Samplable* sample_target, int depth) -> Color const {
    if (depth <= 0) return Color(0, 0, 0);  // 深度限制
    
    HitResult hit_result;
    if (!world.hit(ray, Interval(0.001, infinity), hit_result)) {
        return background;  // 未击中物体，返回背景色
    }
    
    ScatterResult scatter_result;
    Color color_emit = hit_result.mat->emit(ray, hit_result);
    
    if (!hit_result.mat->scatter(ray, hit_result, scatter_result)) {
        return color_emit;  // 不散射，仅返回自发光
    }
    
    auto& [attenuation, scattered] = scatter_result;
    
    // 处理散射：使用模式匹配处理不同类型的散射结果
    auto color_scatter = Match{std::move(scattered)}(
        [&](Ray ray) -> Color {
            // 镜面反射/折射：直接使用散射光线
            return attenuation * ray_color(ray, world, sample_target, depth - 1);
        },
        [&](std::unique_ptr<Vec3PDF> pdf) -> Color {
            // 漫反射：使用概率密度函数进行重要性采样
            std::unique_ptr<Vec3PDF> scatter_pdf =
                sample_target ? std::make_unique<MixturePDF>(
                                    std::make_unique<ObjectPDF>(*sample_target, hit_result.p),
                                    std::move(pdf))
                              : std::move(pdf);
            auto scattered_ray = Ray(hit_result.p, scatter_pdf->generate(), ray.time());
            double sampling_prob = scatter_pdf->value(scattered_ray.direction());
            double scatter_prob = hit_result.mat->scattering_pdf(ray, hit_result, scattered_ray);
            Color sampled_color = ray_color(scattered_ray, world, sample_target, depth - 1);
            return (attenuation * scatter_prob * sampled_color) / sampling_prob;
        });
    
    return color_emit + color_scatter;
}
```

渲染方程：$L_o = L_e + integral f_r L_i cos theta dif omega$

==== Gamma 校正与抗锯齿

```cpp
class Color : public Vec3 {
public:
    Color to_gamma() {
        return Color(std::sqrt(r()), std::sqrt(g()), std::sqrt(b()));
    }
    
    std::tuple<uint8_t, uint8_t, uint8_t> to_byte() const {
        auto red = std::clamp(r(), 0.0, 0.9999);
        auto green = std::clamp(g(), 0.0, 0.9999);
        auto blue = std::clamp(b(), 0.0, 0.9999);
        return std::make_tuple(int(256 * red), int(256 * green), int(256 * blue));
    }
};
```

*分层采样（Stratified Sampling）：*

```cpp
auto sample_square_stratified(int si, int sj) const {
    auto px = ((si + random_double()) * recip_sqrt_spp) - 0.5;
    auto py = ((sj + random_double()) * recip_sqrt_spp) - 0.5;
    return Vec3{px, py, 0.0};
}

// 主渲染循环
for (int sj = 0; sj < sqrt_spp; sj++) {
    for (int si = 0; si < sqrt_spp; si++) {
        Ray r = get_ray(i, j, si, sj);
        pixel_color += ray_color(r, world, sample_target, max_depth);
    }
}
```

#split-semi

=== 散焦模糊（景深）

模拟真实相机的光圈效果，通过在散焦盘上随机采样光线起点实现景深。

```cpp
auto sample_defocus_disk() const {
    auto p = Vec3::random_in_unit_disk();
    return center + (p.x() * defocus_disk_u) + (p.y() * defocus_disk_v);
}

auto get_ray(int i, int j, int si, int sj) const -> Ray {
    auto offset = sample_square_stratified(si, sj);
    auto pixel_sample = pixel00_loc + 
        ((i + offset.x()) * pixel_delta_u) + 
        ((j + offset.y()) * pixel_delta_v);
    
    auto ray_origin = (defocus_angle <= 0) ? center : sample_defocus_disk();
    auto ray_time = random_double();
    return Ray(ray_origin, pixel_sample - ray_origin, ray_time);
}
```

散焦半径计算：
```cpp
auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle) / 2);
defocus_disk_u = defocus_radius * u;
defocus_disk_v = defocus_radius * v;
```

// 图片占位：景深效果对比
// #image("path/to/depth_of_field.png", width: 100%)

#split-semi

== 第二部分：性能优化与复杂特性

=== 运动模糊

通过为每条光线分配随机时间戳，并在该时刻采样运动物体的位置，实现运动模糊效果。

==== MovingSphere：运动球体

```cpp
class Sphere : public Hittable {
    Ray center;  // 用 Ray 存储运动轨迹

public:
    Sphere(const Point3& center_start, const Point3& center_end,
           double radius, std::shared_ptr<Material> mat)
        : center(center_start, center_end - center_start),
          radius(std::max(0., radius)), mat(mat) {
        // 合并两个时刻的包围盒
        auto box0 = BoundingBox::diag(center.at(0) - rvec, center.at(0) + rvec);
        auto box1 = BoundingBox::diag(center.at(1) - rvec, center.at(1) + rvec);
        bbox = BoundingBox::combine(box0, box1);
    }

    bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
        auto current_center = center.at(ray.time());  // 在光线时间戳时的位置
        // ... 相交测试
    }
};
```

关键思想：
- 光线携带时间信息 `ray.time()`
- 物体位置根据时间插值：$bold(C)(t) = bold(C)_0 + t(bold(C)_1 - bold(C)_0)$
- 包围盒覆盖整个运动范围

// 图片占位：运动模糊效果
// #image("path/to/motion_blur.png", width: 80%)

#split-semi

=== BVH 加速结构

层次包围盒（Bounding Volume Hierarchy）通过空间划分大幅减少光线-物体相交测试次数。

==== AABB：轴对齐包围盒

```cpp
class BoundingBox {
public:
    Interval x, y, z;

    static BoundingBox diag(const Point3& a, const Point3& b) {
        auto x = Interval(std::fmin(a.x(), b.x()), std::fmax(a.x(), b.x()));
        auto y = Interval(std::fmin(a.y(), b.y()), std::fmax(a.y(), b.y()));
        auto z = Interval(std::fmin(a.z(), b.z()), std::fmax(a.z(), b.z()));
        return BoundingBox(x, y, z);
    }

    static BoundingBox combine(const BoundingBox& box0, const BoundingBox& box1) {
        auto x = Interval::combine(box0.x, box1.x);
        auto y = Interval::combine(box0.y, box1.y);
        auto z = Interval::combine(box0.z, box1.z);
        return BoundingBox(x, y, z);
    }

    bool hit(const Ray& ray, Interval interval) const {
        for (int axis = 0; axis < 3; axis++) {
            const Interval& ax = axis_interval(axis);
            const double inv_d = 1.0 / ray.direction()[axis];

            auto t0 = (ax.min - ray.origin()[axis]) * inv_d;
            auto t1 = (ax.max - ray.origin()[axis]) * inv_d;

            if (t0 > t1) std::swap(t0, t1);

            interval.min = std::fmax(t0, interval.min);
            interval.max = std::fmin(t1, interval.max);

            if (interval.max <= interval.min) return false;
        }
        return true;
    }
};
```

==== BVHNode：BVH 节点

```cpp
class BVHNode : public Hittable {
    std::shared_ptr<Hittable> left, right;
    BoundingBox bbox;

public:
    BVHNode(std::vector<std::shared_ptr<Hittable>> objects, 
            size_t start, size_t end) {
        // 计算包围盒
        bbox = BoundingBox::empty();
        for (size_t i = start; i < end; i++) {
            bbox = BoundingBox::combine(bbox, objects[i]->bounding_box());
        }
        
        int axis = bbox.longest_axis();  // 选择最长轴分割
        
        size_t object_span = end - start;
        if (object_span == 1) {
            left = right = objects[start];
        } else if (object_span == 2) {
            left = objects[start];
            right = objects[start + 1];
        } else {
            std::sort(objects.begin() + start, objects.begin() + end, 
                     comparator[axis]);
            auto mid = start + object_span / 2;
            left = BVHNode::create(objects, start, mid);
            right = BVHNode::create(objects, mid, end);
        }
    }

    bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
        if (!bbox.hit(ray, interval)) return false;  // 早期剔除
        
        bool hit_left = left->hit(ray, interval, result);
        bool hit_right = right->hit(ray, 
            hit_left ? Interval(interval.min, result.t) : interval, result);
        
        return hit_left || hit_right;
    }
};
```

BVH 构建策略：
1. 计算所有物体的总包围盒
2. 选择最长轴进行分割
3. 排序物体并递归构建子树
4. 叶节点包含 1-2 个物体

时间复杂度：$O(log N)$ 查询，$O(N log N)$ 构建

// 图片占位：BVH树结构示意图
// #image("path/to/bvh_structure.png", width: 80%)

#split-semi

=== 纹理映射

纹理系统支持纯色、图像、程序化噪声等多种类型。

==== Texture 基类

```cpp
class Texture {
public:
    virtual ~Texture() = default;
    virtual Color value(double u, double v, const Point3& p) const = 0;
};
```

==== ColorTexture：纯色纹理

```cpp
class ColorTexture : public Texture {
    Color albedo;

public:
    ColorTexture(const Color& albedo) : albedo(albedo) {}
    
    Color value(double u, double v, const Point3& p) const override {
        return albedo;
    }
};
```

==== ImageTexture：图像纹理

```cpp
class ImageTexture : public Texture {
    Image image;

public:
    ImageTexture(const char* filename) : image(filename) {}

    Color value(double u, double v, const Point3& p) const override {
        if (image.width() == 0 || image.height() == 0) {
            return Color(0.0, 1.0, 1.0);  // cyan for debug
        }

        u = std::clamp(u, 0.0, 1.0);
        v = 1.0 - std::clamp(v, 0.0, 1.0);  // 翻转 V 坐标

        auto i = std::clamp(static_cast<int>(u * image.width()), 
                           0, image.width() - 1);
        auto j = std::clamp(static_cast<int>(v * image.height()), 
                           0, image.height() - 1);

        auto pixel = image.data(i, j);
        auto r = static_cast<double>(pixel[0]) / 255.0;
        auto g = static_cast<double>(pixel[1]) / 255.0;
        auto b = static_cast<double>(pixel[2]) / 255.0;

        return Color(r, g, b);
    }
};
```

==== Perlin 噪声纹理

Perlin 噪声用于生成自然的程序化纹理（大理石、云层等）。

```cpp
class Perlin {
    static constexpr int point_count = 256;
    Vec3 randvec[point_count];
    int perm_x[point_count], perm_y[point_count], perm_z[point_count];

public:
    double noise(const Point3& p) const {
        auto u = p.x() - std::floor(p.x());
        auto v = p.y() - std::floor(p.y());
        auto w = p.z() - std::floor(p.z());
        auto i = int(std::floor(p.x())) & 255;
        auto j = int(std::floor(p.y())) & 255;
        auto k = int(std::floor(p.z())) & 255;
        
        Vec3 c[2][2][2];
        for (int di = 0; di < 2; di++) {
            for (int dj = 0; dj < 2; dj++) {
                for (int dk = 0; dk < 2; dk++) {
                    c[di][dj][dk] = randvec[
                        perm_x[(i + di) & 255] ^ 
                        perm_y[(j + dj) & 255] ^ 
                        perm_z[(k + dk) & 255]
                    ];
                }
            }
        }
        return interpolation(c, u, v, w);
    }

    double turb(const Point3& p, int depth = 7) const {
        auto accum = 0.0;
        auto temp_p = p;
        auto weight = 1.0;

        for (int i = 0; i < depth; i++) {
            accum += weight * noise(temp_p);
            weight *= 0.5;
            temp_p = temp_p * 2.0;
        }

        return std::fabs(accum);
    }
};
```

==== MarbleTexture：大理石纹理

```cpp
class MarbleTexture : public Texture {
    Perlin perlin;
    double scale;
    Vec3 dir;

public:
    MarbleTexture(double scale, Vec3 direction) 
        : scale(scale), dir(direction) {}

    Color value(double u, double v, const Point3& p) const override {
        return Color(1, 1, 1) * 0.5 * 
            (1 + std::sin(scale * dot(p, dir) + 10 * perlin.turb(p)));
    }
};
```

// 图片占位：不同纹理效果展示
// #image("path/to/texture_examples.png", width: 100%)

#split-semi

=== 光源与发光材质

==== Light：发光材质

```cpp
class Light : public Material {
    std::shared_ptr<Texture> tex;

public:
    Light(const Color& color) : tex(std::make_shared<ColorTexture>(color)) {}

    Color emit(const Ray& r_in, const HitResult& hit) const override {
        if (!hit.front_face) {
            return Color(0, 0, 0);  // 背面不发光
        }
        return tex->value(hit.u, hit.v, hit.p);
    }
};
```

关键特性：
- 只从正面发光（`front_face` 检查）
- 不散射光线（`scatter` 返回 `false`）
- 支持纹理化发光

使用示例：
```cpp
auto light_material = Light::create(Color(15, 15, 15));
auto light = Quadrilateral::create(
    Point3(213, 554, 227),
    Vec3(130, 0, 0),
    Vec3(0, 0, 105),
    light_material
);
```

#split-semi

=== 新型几何体

==== Quadrilateral：四边形

四边形是构建复杂场景的基础，支持光源采样。

```cpp
class Quadrilateral : public Shape2D, public Samplable {
    double area;

public:
    Quadrilateral(const Point3& origin, const Vec3& u, const Vec3& v,
                 std::shared_ptr<Material> mat) : Shape2D(origin, u, v, mat) {
        area = cross(u, v).norm();
        set_bounding_box();
    }

    bool is_interior(double alpha, double beta, HitResult& result) const override {
        auto unit = Interval(0, 1);
        if (!unit.contains(alpha)) return false;
        if (!unit.contains(beta)) return false;
        result.u = alpha;
        result.v = beta;
        return true;
    }

    // 用于重要性采样
    double pdf_value(const Point3& o, const Vec3& v) const override {
        HitResult result;
        Ray ray(o, v);
        if (!hit(ray, Interval(0.001, infinity), result)) return 0.0;

        auto distance_squared = result.t * result.t * v.sqrnorm();
        auto cosine = std::fabs(dot(normal, v.normalized()));

        return distance_squared / (cosine * area);
    }

    Vec3 random(const Point3& o) const override {
        auto random_point = origin + random_double() * vec_u + 
                           random_double() * vec_v;
        return random_point - o;
    }
};
```

==== Triangle：三角形

```cpp
class Triangle : public Shape2D {
    bool is_interior(double alpha, double beta, HitResult& result) const override {
        if (alpha + beta > 1) return false;  // 重心坐标约束
        
        auto unit = Interval(0, 1);
        if (!unit.contains(alpha)) return false;
        if (!unit.contains(beta)) return false;
        
        result.u = alpha;
        result.v = beta;
        return true;
    }
};
```

==== Box：立方体

```cpp
class Box : public Hittable {
    HittableList sides;

public:
    Box(const Point3& a, const Point3& b, std::shared_ptr<Material> mat) {
        auto min = Point3(std::fmin(a.x(), b.x()), 
                         std::fmin(a.y(), b.y()), 
                         std::fmin(a.z(), b.z()));
        auto max = Point3(std::fmax(a.x(), b.x()), 
                         std::fmax(a.y(), b.y()), 
                         std::fmax(a.z(), b.z()));

        auto dx = Vec3(max.x() - min.x(), 0, 0);
        auto dy = Vec3(0, max.y() - min.y(), 0);
        auto dz = Vec3(0, 0, max.z() - min.z());

        // 构建六个面
        sides.add(make_shared<Quadrilateral>(
            Point3(min.x(), min.y(), max.z()), dx, dy, mat));  // front
        sides.add(make_shared<Quadrilateral>(
            Point3(max.x(), min.y(), max.z()), -dz, dy, mat)); // right
        // ... 其余四个面
    }
};
```

// 图片占位：几何体组合示例
// #image("path/to/geometry_examples.png", width: 100%)

#split-semi

=== 参与介质

==== ConstantMedium：恒定密度介质

用于模拟烟雾、雾气等体积效果。

```cpp
class ConstantMedium : public Hittable {
    std::shared_ptr<Hittable> boundary;
    std::shared_ptr<Material> phase_function;
    double neg_inv_density;

public:
    ConstantMedium(std::shared_ptr<Hittable> b, double d, const Color& color)
        : boundary(std::move(b)), 
          neg_inv_density(-1.0 / d),
          phase_function(Isotropic::create(color)) {}

    bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
        HitResult hit1, hit2;
        
        // 找到进入和离开边界的两个交点
        if (!boundary->hit(ray, Interval::universe(), hit1)) return false;
        if (!boundary->hit(ray, Interval(hit1.t + 1e-4, infinity), hit2)) return false;

        hit1.t = std::fmax(hit1.t, interval.min);
        hit2.t = std::fmin(hit2.t, interval.max);

        if (hit1.t >= hit2.t) return false;
        if (hit1.t < 0) hit1.t = 0;

        auto ray_length = ray.direction().norm();
        auto distance = (hit2.t - hit1.t) * ray_length;

        // 随机采样散射距离
        auto hit_distance = neg_inv_density * std::log(random_double());

        if (hit_distance > distance) return false;

        result.t = hit1.t + hit_distance / ray_length;
        result.p = ray.at(result.t);
        result.normal = Vec3(1, 0, 0);  // arbitrary
        result.front_face = true;
        result.mat = phase_function;

        return true;
    }
};
```

==== Isotropic：各向同性材质

```cpp
class Isotropic : public Material {
    std::shared_ptr<Texture> tex;

public:
    Isotropic(const Color& color) : tex(std::make_shared<ColorTexture>(color)) {}

    bool scatter(const Ray& r_in, const HitResult& hit,
                ScatterResult& result) const override {
        result.scattered = std::make_unique<SpherePDF>();  // 均匀散射
        result.attenuation = tex->value(hit.u, hit.v, hit.p);
        return true;
    }

    double scattering_pdf(const Ray& r_in, const HitResult& hit,
                         const Ray& scattered) const override {
        return 1 / (4 * pi);  // 球面均匀分布
    }
};
```

散射距离采样：$d = -ln(xi) / rho$，其中 $xi$ 是随机数，$rho$ 是密度。

// 图片占位：体积渲染效果
// #image("path/to/volume_rendering.png", width: 80%)

#split-semi

== 第三部分：蒙特卡洛积分与重要性采样

=== 蒙特卡洛积分

渲染方程的核心是求解积分：

$ L_o(p, omega_o) = L_e(p, omega_o) + integral_(Omega) f_r(p, omega_i, omega_o) L_i(p, omega_i) cos theta_i dif omega_i $

蒙特卡洛方法通过随机采样估计积分值：

$ angle.l I angle.r = 1/N sum_(i=1)^N f(X_i) / p(X_i) $

其中 $p(X)$ 是概率密度函数（PDF）。

==== Vec3PDF 基类

```cpp
class Vec3PDF {
public:
    virtual ~Vec3PDF() = default;
    virtual double value(const Vec3& direction) const = 0;  // PDF值
    virtual Vec3 generate() const = 0;                      // 采样方向
};
```

==== SpherePDF：球面均匀采样

```cpp
class SpherePDF : public Vec3PDF {
public:
    double value(const Vec3& direction) const override {
        return 1 / (4 * pi);  // 均匀分布
    }
    
    Vec3 generate() const override {
        return Vec3::random_unit();
    }
};
```

#split-semi

=== 重要性采样

重要性采样通过选择合适的 PDF 来降低方差，提高收敛速度。

==== CosinePDF：余弦加权采样

用于漫反射表面，PDF 与 $cos theta$ 成正比。

```cpp
class CosinePDF : public Vec3PDF {
    OrthonormalBasis uvw;

public:
    CosinePDF(const Vec3& w) : uvw(w) {}

    double value(const Vec3& direction) const override {
        auto cosine = dot(direction.normalized(), uvw.w());
        return (cosine <= 0) ? 0.0 : (cosine / pi);
    }

    Vec3 generate() const override {
        return uvw.transform(Vec3::random_cosine_z());
    }
};
```

PDF 公式：$p(omega) = (cos theta) / pi$

采样方法（Malley's method）：
1. 在单位圆内均匀采样 $(x, y)$
2. 计算 $z = sqrt(1 - x^2 - y^2)$
3. 得到球面点 $(x, y, z)$

```cpp
static Vec3 random_cosine_z() {
    auto r1 = random_double();
    auto r2 = random_double();
    auto phi = 2 * pi * r1;
    auto x = std::cos(phi) * std::sqrt(r2);
    auto y = std::sin(phi) * std::sqrt(r2);
    auto z = std::sqrt(1 - r2);
    return Vec3(x, y, z);
}
```

==== ObjectPDF：几何体采样

直接对光源进行采样，适用于场景中有明确光源的情况。

```cpp
class ObjectPDF : public Vec3PDF {
    const Samplable& objects;
    Point3 origin;

public:
    ObjectPDF(const Samplable& objects, const Point3& origin)
        : objects(objects), origin(origin) {}

    double value(const Vec3& direction) const override {
        return objects.pdf_value(origin, direction);
    }

    Vec3 generate() const override {
        return objects.random(origin);
    }
};
```

对于四边形光源：

```cpp
double pdf_value(const Point3& o, const Vec3& v) const override {
    HitResult result;
    if (!hit(Ray(o, v), Interval(0.001, infinity), result)) return 0.0;
    
    auto distance_squared = result.t * result.t * v.sqrnorm();
    auto cosine = std::fabs(dot(normal, v.normalized()));
    
    return distance_squared / (cosine * area);
}
```

PDF 转换公式：$p(omega) = p(A) (r^2) / (cos theta dot A)$

#split-semi

=== 正交基 ONB

用于在局部坐标系和世界坐标系之间转换。

```cpp
class OrthonormalBasis {
    Vec3 axis[3];

public:
    explicit OrthonormalBasis(const Vec3& n) {
        axis[2] = n.normalized();  // w轴（法线方向）
        Vec3 a = (std::fabs(axis[2].x()) > 0.9) ? 
                 Vec3(0, 1, 0) : Vec3(1, 0, 0);
        axis[1] = cross(axis[2], a).normalized();  // v轴
        axis[0] = cross(axis[2], axis[1]);         // u轴
    }

    const Vec3& u() const { return axis[0]; }
    const Vec3& v() const { return axis[1]; }
    const Vec3& w() const { return axis[2]; }

    Vec3 transform(const Vec3& a) const {
        return a.x() * axis[0] + a.y() * axis[1] + a.z() * axis[2];
    }
};
```

构建方法：
1. 法线 $bold(n)$ 作为 $w$ 轴
2. 选择辅助向量 $bold(a)$（避免与 $bold(n)$ 平行）
3. 计算 $bold(v) = bold(w) times bold(a)$
4. 计算 $bold(u) = bold(w) times bold(v)$

// 图片占位：正交基示意图
// #image("path/to/onb_diagram.png", width: 60%)

#split-semi

=== 混合采样

==== MixturePDF：混合概率密度函数

结合多种采样策略（如 BRDF 采样 + 光源采样），平衡探索与利用。

```cpp
class MixturePDF : public Vec3PDF {
    std::unique_ptr<Vec3PDF> p0, p1;
    double weight_1;

public:
    MixturePDF(std::unique_ptr<Vec3PDF> p0, std::unique_ptr<Vec3PDF> p1,
               double w1 = 0.5)
        : p0(std::move(p0)), p1(std::move(p1)), weight_1(w1) {}

    double value(const Vec3& direction) const override {
        return (1 - weight_1) * p0->value(direction) + 
               weight_1 * p1->value(direction);
    }

    Vec3 generate() const override {
        if (random_double() < weight_1) {
            return p1->generate();
        } else {
            return p0->generate();
        }
    }
};
```

使用示例：

```cpp
std::unique_ptr<Vec3PDF> scatter_pdf = 
    sample_target ? std::make_unique<MixturePDF>(
        std::make_unique<ObjectPDF>(*sample_target, hit_result.p),
        std::move(pdf))
    : std::move(pdf);
```

混合策略的优势：
- 降低方差：结合多种采样方法
- 鲁棒性强：适应不同场景配置
- 收敛更快：充分利用场景信息

#split-semi

=== 康奈尔盒场景

经典测试场景，用于验证光照计算的正确性。

```cpp
auto cornell_box() {
    HittableList world;

    auto red = Lambertian::create(Color(.65, .05, .05));
    auto white = Lambertian::create(Color(.73, .73, .73));
    auto green = Lambertian::create(Color(.12, .45, .15));
    auto light = Light::create(Color(15, 15, 15));

    // 墙壁
    world.add(Quadrilateral::create(
        Point3(555, 0, 0), Vec3(0, 555, 0), Vec3(0, 0, 555), green));
    world.add(Quadrilateral::create(
        Point3(0, 0, 0), Vec3(0, 555, 0), Vec3(0, 0, 555), red));
    world.add(Quadrilateral::create(
        Point3(0, 0, 0), Vec3(555, 0, 0), Vec3(0, 0, 555), white));

    // 顶部光源
    world.add(Quadrilateral::create(
        Point3(213, 554, 227), Vec3(130, 0, 0), Vec3(0, 0, 105), light));

    // 盒子
    auto box1 = Box::create(Point3(0, 0, 0), Point3(165, 330, 165), white);
    auto box2 = Box::create(Point3(0, 0, 0), Point3(165, 165, 165), white);

    return world;
}
```

康奈尔盒特点：
- 左红右绿墙面（测试颜色渗透）
- 顶部区域光源
- 两个不同高度的白色立方体
- 标准尺寸：555 × 555 × 555

// 图片占位：康奈尔盒渲染结果
// #image("path/to/cornell_box.png", width: 80%)

#split-semi

=== 现代渲染架构

==== 完整渲染流程

```cpp
auto render(const Hittable& world, Samplable* sample_target = nullptr) {
    initialize();
    auto image = std::vector<Color>(image_width * image_height);

    std::atomic<int> counter = 0;
    #pragma omp parallel for
    for (int j = 0; j < image_height; ++j) {
        for (int i = 0; i < image_width; ++i) {
            Color pixel_color(0, 0, 0);
            int num_samples = 0;
            
            for (int sj = 0; sj < sqrt_spp; sj++) {
                for (int si = 0; si < sqrt_spp; si++) {
                    Ray r = get_ray(i, j, si, sj);
                    auto color = ray_color(r, world, sample_target, max_depth);
                    
                    // NaN检测
                    if (color.r() != color.r() || color.g() != color.g() ||
                        color.b() != color.b()) {
                        continue;
                    }
                    num_samples++;
                    pixel_color += color;
                }
            }
            image[j * image_width + i] = pixel_color / (double)num_samples;
        }
    }
    return RenderResult{.width = image_width, .height = image_height, 
                        .data = std::move(image)};
}
```

==== 智能采样调度

```cpp
auto color_scatter = Match{std::move(scattered)}(
    [&](Ray ray) -> Color {
        // 确定性散射（镜面反射、折射）
        return attenuation * ray_color(ray, world, sample_target, depth - 1);
    },
    [&](std::unique_ptr<Vec3PDF> pdf) -> Color {
        // 随机散射（漫反射）
        std::unique_ptr<Vec3PDF> scatter_pdf =
            sample_target ? std::make_unique<MixturePDF>(
                std::make_unique<ObjectPDF>(*sample_target, hit_result.p),
                std::move(pdf))
            : std::move(pdf);
        
        auto scattered_ray = Ray(hit_result.p, scatter_pdf->generate(), ray.time());
        double sampling_prob = scatter_pdf->value(scattered_ray.direction());
        double scatter_prob = 
            hit_result.mat->scattering_pdf(ray, hit_result, scattered_ray);
        
        Color sampled_color = ray_color(scattered_ray, world, sample_target, depth - 1);
        return (attenuation * scatter_prob * sampled_color) / sampling_prob;
    }
);
```

核心优化技术：
1. *OpenMP 并行化*：`#pragma omp parallel for`
2. *分层采样*：减少方差
3. *混合重要性采样*：BRDF + 光源采样
4. *BVH 加速*：$O(log N)$ 光线求交
5. *NaN 检测*：避免数值错误
6. *自适应深度*：Russian Roulette（可选）

渲染方程的最终蒙特卡洛估计：

$ L_o approx L_e + (f_r dot p_"scatter" dot L_i) / p_"sample" $

其中：
- $f_r$：BRDF
- $p_"scatter"$：散射概率
- $p_"sample"$：采样概率
- $L_i$：入射辉度（递归计算）

#split-semi

== 第四部分：光谱渲染（Spectral Rendering）

=== 概述与动机

传统的 RGB 渲染虽然计算简单，但存在物理准确性问题：RGB 只是人眼对可见光谱的简化表示，无法精确模拟真实光线的波长相关行为。光谱渲染通过在整个可见光波长范围内进行计算，提供了更物理准确的光照模拟。

*光谱渲染的优势：*
1. *物理准确性*：直接模拟光的波长特性
2. *色散模拟*：棱镜、彩虹等色散现象
3. *真实材质*：金属、宝石等材质的光谱反射特性
4. *荧光效应*：波长转换现象（如荧光材料）
5. *色彩管理*：更准确的色彩空间转换

*核心实现 commit 参考：*
- `ca89419`：光谱数据结构与 CIE 色彩匹配函数
- `47d5f67`：RGB 到光谱转换（Smits 1999 方法）
- `336d96f`：材质系统重构，支持光谱 BRDF

=== 重构概览：从 RGB 到 Spectrum

*之前的 RGB 架构：*
```cpp
// 旧实现：直接使用 Vec3 表示颜色
class Material {
    Color albedo;  // Color = Vec3，三通道 RGB
    virtual Color scatter(...) const;
};
```

*重构后的 Spectrum 架构：*
```cpp
// 新实现：使用 Spectrum 类进行光谱采样
class Material {
    Spectrum albedo;  // 60个波长样本（400-700nm）
    virtual Spectrum scatter(...) const;
};
```

*关键变化：*
1. *颜色表示*：`Vec3 (RGB)` → `Spectrum (60 samples)`
2. *波长范围*：400nm - 700nm（可见光）
3. *采样策略*：均匀分布 60 个样本点
4. *转换接口*：支持 RGB ↔ Spectrum 双向转换

=== Spectrum 类实现

==== 基础结构

```cpp
class Spectrum {
public:
    static constexpr int nSamples = 60;           // 采样数量
    static constexpr double lambdaStart = 400;    // 起始波长（nm）
    static constexpr double lambdaEnd = 700;      // 结束波长（nm）

private:
    std::array<double, nSamples> c = {};  // 光谱样本值
    
    // 静态数据：CIE XYZ 匹配函数、RGB 转换表
    struct StaticData {
        std::array<double, nSamples> sX, sY, sZ;  // CIE 色彩匹配
        // RGB to Spectrum 转换表（反射 + 发光）
        std::array<double, nSamples> rgbRefl2Spect[7];
        std::array<double, nSamples> rgbIllum2Spect[7];
        bool initialized = false;
    };
};
```

*设计要点：*
- 60 个样本覆盖 400-700nm 可见光范围
- 每个样本间隔约 5nm
- 采用懒加载：首次使用时初始化转换表

==== RGB 到光谱转换（Smits 1999）

核心算法基于 *Smits 1999* 论文，使用基函数插值：

```cpp
static Spectrum fromRGB(const double rgb[3], SpectrumType type) {
    auto& white = (type == Reflectance) ? rgbRefl2SpectWhite : rgbIllum2SpectWhite;
    auto& cyan  = (type == Reflectance) ? rgbRefl2SpectCyan  : rgbIllum2SpectCyan;
    auto& magenta = /* ... */;
    auto& yellow  = /* ... */;
    auto& red     = /* ... */;
    auto& green   = /* ... */;
    auto& blue    = /* ... */;
    
    Spectrum result;
    
    // 根据 RGB 大小关系选择插值路径
    if (rgb[0] <= rgb[1] && rgb[0] <= rgb[2]) {
        // R 最小：white + cyan + (blue or green)
        result += rgb[0] * white;
        if (rgb[1] <= rgb[2]) {
            result += (rgb[1] - rgb[0]) * cyan;
            result += (rgb[2] - rgb[1]) * blue;
        } else {
            result += (rgb[2] - rgb[0]) * cyan;
            result += (rgb[1] - rgb[2]) * green;
        }
    }
    // ... 其他分支（G 最小、B 最小）
    
    return result.clamp();
}
```

*算法原理：*
1. RGB 空间被分解为 6 个四面体区域
2. 每个区域用 3 个基函数插值
3. 基函数：白、青、品红、黄、红、绿、蓝
4. 区分 *反射光谱* 和 *发光光谱* 两种类型

*类型区分：*
- `SpectrumType::Reflectance`：表面反射率（0-1）
- `SpectrumType::Illuminant`：光源发光强度（可 > 1）

==== 光谱到 RGB 转换

通过 CIE XYZ 色彩匹配函数进行转换：

```cpp
void toXYZ(double xyz[3]) const {
    xyz[0] = xyz[1] = xyz[2] = 0;
    for (int i = 0; i < nSamples; ++i) {
        xyz[0] += sX[i] * c[i];  // X 通道
        xyz[1] += sY[i] * c[i];  // Y 通道（亮度）
        xyz[2] += sZ[i] * c[i];  // Z 通道
    }
    // 归一化
    double scale = (lambdaEnd - lambdaStart) / (CIE_Y_integral * nSamples);
    xyz[0] *= scale; xyz[1] *= scale; xyz[2] *= scale;
}

void toRGB(double rgb[3]) const {
    double xyz[3];
    toXYZ(xyz);
    XYZToRGB(xyz, rgb);  // 标准 XYZ -> RGB 矩阵变换
}
```

*转换流程：*
```
Spectrum (60 samples) 
    ↓ 与 CIE XYZ 匹配函数卷积
XYZ 三刺激值
    ↓ 线性变换矩阵
RGB 颜色
```

*CIE XYZ 色彩匹配函数：*
- 基于人眼视锥细胞响应曲线
- 81 个标准样本点（380-780nm，5nm 间隔）
- 通过插值映射到 60 个光谱样本

=== CIE 色彩匹配与转换

==== CIE 1931 标准观察者

```cpp
namespace spectrum_data {
    // CIE XYZ 匹配函数（380-780nm，81 个样本）
    inline constexpr int nCIESamples = 81;
    inline constexpr double CIE_lambda[nCIESamples] = {
        380, 385, 390, ..., 775, 780
    };
    inline constexpr double CIE_X[nCIESamples] = { /* ... */ };
    inline constexpr double CIE_Y[nCIESamples] = { /* ... */ };
    inline constexpr double CIE_Z[nCIESamples] = { /* ... */ };
    inline constexpr double CIE_Y_integral = 106.856895;
}
```

*功能：*
- $overline(x)(lambda), overline(y)(lambda), overline(z)(lambda)$：三刺激值函数
- 将任意光谱分布转换为 XYZ 颜色空间
- $overline(y)(lambda)$ 对应人眼亮度响应（V(λ) 函数）

==== XYZ ↔ RGB 转换矩阵

```cpp
// XYZ → sRGB（D65 白点）
inline void XYZToRGB(const double xyz[3], double rgb[3]) {
    rgb[0] =  3.240479 * xyz[0] - 1.537150 * xyz[1] - 0.498535 * xyz[2];
    rgb[1] = -0.969256 * xyz[0] + 1.875991 * xyz[1] + 0.041556 * xyz[2];
    rgb[2] =  0.055648 * xyz[0] - 0.204043 * xyz[1] + 1.057311 * xyz[2];
}

// sRGB → XYZ
inline void RGBToXYZ(const double rgb[3], double xyz[3]) {
    xyz[0] = 0.412453 * rgb[0] + 0.357580 * rgb[1] + 0.180423 * rgb[2];
    xyz[1] = 0.212671 * rgb[0] + 0.715160 * rgb[1] + 0.072169 * rgb[2];
    xyz[2] = 0.019334 * rgb[0] + 0.119193 * rgb[1] + 0.950227 * rgb[2];
}
```

*标准：*
- sRGB 色彩空间（IEC 61966-2-1）
- D65 标准光源（色温 6500K）
- 线性 RGB（未应用 gamma 校正）

=== 光谱采样与插值

==== 波长采样

```cpp
double operator()(double lambda) const {
    if (lambda < lambdaStart || lambda > lambdaEnd) return 0;
    
    // 线性插值
    double t = (lambda - lambdaStart) / (lambdaEnd - lambdaStart) * nSamples;
    int i = std::min(int(t), nSamples - 1);
    double dt = t - i;
    
    if (i >= nSamples - 1) return c[nSamples - 1];
    return (1 - dt) * c[i] + dt * c[i + 1];
}
```

*用途：*
- 查询任意波长的光谱值
- 支持色散模拟（不同波长不同折射率）

==== 区间平均采样

```cpp
inline double averageSpectrumSamples(
    const double* lambda, const double* vals, int n,
    double lambdaStart, double lambdaEnd) {
    
    // 梯形积分法
    double sum = 0;
    for (int i = 0; i + 1 < n && lambdaEnd >= lambda[i]; ++i) {
        double segStart = std::max(lambdaStart, lambda[i]);
        double segEnd = std::min(lambdaEnd, lambda[i + 1]);
        sum += 0.5 * (interp(segStart, i) + interp(segEnd, i)) 
               * (segEnd - segStart);
    }
    return sum / (lambdaEnd - lambdaStart);
}
```

*应用场景：*
- 初始化时，将 CIE 81 样本映射到 60 样本
- 保证能量守恒的降采样

=== 光谱运算

```cpp
class Spectrum {
    // 算术运算（逐波长）
    Spectrum operator+(const Spectrum& s) const;  // 光谱叠加
    Spectrum operator*(const Spectrum& s) const;  // 逐波长相乘（BRDF 调制）
    Spectrum operator*(double a) const;           // 标量缩放
    
    // 工具函数
    double y() const;  // 计算亮度（Y 通道）
    double maxComponent() const;  // 最大光谱值
    bool isBlack() const;  // 是否为黑色
    Spectrum clamp(double low = 0, double high = 1e30) const;
    
    // 数学函数（逐波长应用）
    friend Spectrum sqrt(const Spectrum& s);
    friend Spectrum exp(const Spectrum& s);
    friend Spectrum pow(const Spectrum& s, double e);
    friend Spectrum lerp(double t, const Spectrum& s1, const Spectrum& s2);
};
```

*物理意义：*
- `a * b`：反射率调制（材质 BRDF × 入射光）
- `a + b`：光谱能量叠加（多光源）
- `sqrt(s)`：用于重要性采样的方差计算

=== 材质系统重构

==== 光谱 BRDF

```cpp
class Material {
protected:
    Spectrum albedo;  // 光谱反射率

public:
    // 构造函数支持 RGB 输入（自动转换）
    Material(const Color& color) 
        : albedo(Spectrum::fromRGB(color, SpectrumType::Reflectance)) {}
    
    virtual bool scatter(const Ray& r_in, const HitResult& hit, 
                        ScatterResult& result) const {
        // result.attenuation 现在是 Spectrum 类型
        result.attenuation = albedo;  // 光谱衰减
        // ...
    }
};
```

==== 示例：金属材质

```cpp
class Metal : public Material {
    Spectrum albedo;  // 光谱反射率（金属的波长相关反射）
    double fuzz;
    
public:
    Metal(const Color& color, double fuzz = 0) 
        : albedo(Spectrum::fromRGB(color, SpectrumType::Reflectance)), fuzz(fuzz) {}
    
    bool scatter(const Ray& r_in, const HitResult& hit, 
                 ScatterResult& result) const override {
        Vec3 reflected = reflect(r_in.direction(), hit.normal);
        // 光谱调制
        result.attenuation = albedo;
        result.scattered = Ray(hit.p, reflected + fuzz * random_in_unit_sphere());
        return true;
    }
};
```

*优势：*
- 真实金属（铜、金、银）的光谱反射曲线
- 自然呈现金属的色彩倾向

=== 向后兼容性

为保证平滑过渡，Spectrum 类提供 RGB 兼容接口：

```cpp
class Spectrum {
    // 从 Color/Vec3 隐式构造
    explicit Spectrum(const Color& color);
    explicit Spectrum(const Vec3& rgb);
    
    // 转换回 Color
    Color toColor() const;
    
    // 静态工厂函数
    static Spectrum fromRGB(const Color& color, 
                           SpectrumType type = SpectrumType::Reflectance);
    
    // 旧 API 别名
    static constexpr double visible_min = lambdaStart;
    static constexpr double visible_max = lambdaEnd;
    static Spectrum constant(double v) { return Spectrum(v); }
};
```

*使用示例：*
```cpp
// 旧代码仍可运行
Material* mat = new Lambertian(Color(0.8, 0.3, 0.3));

// 内部自动转换：
// Color → Spectrum::fromRGB(...) → 60 个波长样本

// 渲染输出时：
Spectrum pixel_spectrum = trace_ray(...);
Color rgb = pixel_spectrum.toColor();  // Spectrum → RGB
```

=== 性能考量

*内存占用：*
- RGB：3 × 8 = 24 字节
- Spectrum：60 × 8 = 480 字节
- *增幅：20 倍*

*计算开销：*
- RGB 运算：3 次浮点操作
- Spectrum 运算：60 次浮点操作
- *增幅：20 倍*

*优化策略：*
1. *惰性初始化*：转换表仅初始化一次
2. *SIMD 向量化*：可用 AVX 指令加速（8 个 double 并行）
3. *自适应采样*：重要波长区域增加采样密度
4. *混合渲染*：预览用 RGB，最终渲染用 Spectrum

*适用场景：*
- ✓ 需要物理准确性的科学可视化
- ✓ 宝石、金属等材质的真实渲染
- ✓ 色散、荧光等波长相关效果
- ✗ 实时渲染（性能开销大）
- ✗ 简单场景（RGB 已足够）

=== 扩展应用

==== 色散模拟

```cpp
class Dielectric : public Material {
    Spectrum albedo;
    
    // 波长相关折射率（Sellmeier 公式）
    double refractionIndex(double wavelength) const {
        // 例：BK7 玻璃
        double lambda2 = (wavelength / 1000.0) * (wavelength / 1000.0);
        double n2 = 1.0 + 
            1.03961212 * lambda2 / (lambda2 - 0.00600069867) +
            0.231792344 * lambda2 / (lambda2 - 0.0200179144) +
            1.01046945 * lambda2 / (lambda2 - 103.560653);
        return std::sqrt(n2);
    }
};
```

==== 黑体辐射

```cpp
// Planck 定律
inline double blackbody(double lambda, double T) {
    const double c = 299792458.0;      // 光速
    const double h = 6.62607015e-34;  // Planck 常数
    const double kb = 1.380649e-23;    // Boltzmann 常数
    
    double l = lambda * 1e-9;  // nm → m
    return (2 * h * c * c) / (std::pow(l, 5) * (std::exp((h*c) / (l*kb*T)) - 1));
}

// 生成不同色温的光源光谱
Spectrum lightSpectrum;
for (int i = 0; i < 60; ++i) {
    double lambda = 400 + i * 5;  // 400-700nm
    lightSpectrum[i] = blackbody(lambda, 6500);  // D65 (6500K)
}
```

=== 小结

光谱渲染重构实现了从 RGB 到物理准确光谱的跨越：

*架构变化：*
- 核心颜色表示：`Vec3` → `Spectrum` (60 samples)
- 转换系统：Smits 1999 RGB ↔ Spectrum
- 色彩科学：CIE XYZ 色彩匹配函数
- 材质系统：全面支持光谱 BRDF

*技术亮点：*
- 物理准确的可见光建模（400-700nm）
- 能量守恒的转换算法
- 完整的向后兼容性
- 支持色散、荧光等高级效果

*实现参考：*
- PBRT-v3 光谱系统设计
- Physically Based Rendering 4th Edition
- Smits 1999: "An RGB-to-Spectrum Conversion for Reflectances"

#split-semi

== 总结

本光线追踪项目实现了从基础到高级的完整渲染管线：

*第一部分* 建立了核心框架：
- Vec3 向量数学库
- 光线追踪基础（Ray, Camera）
- 几何求交（Sphere, Hittable）
- 物理材质（Lambertian, Metal, Dielectric）
- 递归路径追踪与景深效果

*第二部分* 引入优化与扩展：
- 运动模糊（时间维度）
- BVH 空间加速（对数时间复杂度）
- 纹理系统（图像、Perlin噪声）
- 面光源（Light 材质）
- 复杂几何体（Quad, Triangle, Box）
- 体积渲染（ConstantMedium）

*第三部分* 实现高级采样技术：
- 蒙特卡洛积分理论
- 重要性采样（CosinePDF, ObjectPDF）
- 正交基坐标变换
- 混合采样策略（MixturePDF）
- 康奈尔盒经典场景

*第四部分* 光谱渲染系统：
- 从 RGB 到光谱的架构重构（60 个波长样本）
- Smits 1999 RGB ↔ Spectrum 转换算法
- CIE XYZ 色彩匹配与标准观察者
- 光谱 BRDF 材质系统
- 支持色散、黑体辐射等物理效果

渲染器性能指标：
- 支持数万级物体场景（BVH 加速）
- 多线程并行渲染（OpenMP）
- 物理正确的光线传输
- 灵活的材质与纹理系统

未来改进方向：
- 双向路径追踪（BDPT）
- 光子映射（Photon Mapping）
- 梯度域渲染（Gradient-domain）
- GPU 加速（CUDA/OptiX）
- 实时降噪（AI Denoiser）

// 图片占位：最终渲染作品集
// #image("path/to/final_gallery.png", width: 100%)
