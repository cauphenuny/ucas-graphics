== GLM 常用方法笔记

GLM (OpenGL Mathematics) 是一个基于 GLSL 规范的 C++ 数学库。

=== `glm::vec3` (三维向量)

常用操作与函数：

- *构造*: `glm::vec3(x, y, z)` 或 `glm::vec3(scalar)` (所有分量设为 scalar)。
- *运算*: 支持 `+`, `-`, `*` (标量乘法或分量乘法), `/` 等运算符重载。
- *点积 (Dot Product)*: `glm::dot(v1, v2)`。返回标量，表示两个向量的相似程度或投影。
- *叉积 (Cross Product)*: `glm::cross(v1, v2)`。返回一个垂直于 v1 和 v2 的新向量。
- *长度 (Length)*: `glm::length(v)`。计算向量的模。
- *归一化 (Normalize)*: `glm::normalize(v)`。返回方向相同但长度为 1 的单位向量。
- *距离 (Distance)*: `glm::distance(p1, p2)`。计算两点之间的欧几里得距离。
- *反射 (Reflect)*: `glm::reflect(I, N)`。计算入射向量 `I` 关于法线 `N` 的反射向量。

```cpp
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

glm::vec3 v1(1.0f, 0.0f, 0.0f);
glm::vec3 v2(0.0f, 1.0f, 0.0f);
float dot = glm::dot(v1, v2);       // 0.0
glm::vec3 cross = glm::cross(v1, v2); // (0, 0, 1)
glm::vec3 norm = glm::normalize(glm::vec3(10.0f, 0.0f, 0.0f)); // (1, 0, 0)
```

=== `glm::mat4` (4x4 矩阵)

通常用于变换矩阵（模型、视图、投影）。

常用操作与函数：

- *单位矩阵*: `glm::mat4(1.0f)`。初始化对角线为 1，其余为 0。
- *矩阵乘法*: `m1 * m2` (注意顺序，通常是 `Projection * View * Model`)。也可以 `mat4 * vec4` 变换向量。
- *平移 (Translate)*: `glm::translate(matrix, vector)`。在给定矩阵基础上应用平移变换。
- *旋转 (Rotate)*: `glm::rotate(matrix, angle_in_radians, axis_vector)`。应用旋转变换。
- *缩放 (Scale)*: `glm::scale(matrix, vector)`。应用缩放变换。
- *透视投影 (Perspective)*: `glm::perspective(fov, aspect_ratio, near, far)`。生成透视投影矩阵。
- *正交投影 (Ortho)*: `glm::ortho(left, right, bottom, top, near, far)`。生成正交投影矩阵。
- *LookAt*: `glm::lookAt(eye, center, up)`。生成视图矩阵（摄像机）。
- *逆矩阵 (Inverse)*: `glm::inverse(matrix)`。
- *转置 (Transpose)*: `glm::transpose(matrix)`。
- *数据指针*: `glm::value_ptr(matrix)`。用于将矩阵数据传递给 OpenGL Shader (需包含 `<glm/gtc/type_ptr.hpp>`)。

```cpp
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

glm::mat4 model = glm::mat4(1.0f); // 单位矩阵
model = glm::translate(model, glm::vec3(1.0f, 0.0f, 0.0f)); // 平移
model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // 旋转

glm::mat4 view = glm::lookAt(
    glm::vec3(0.0f, 0.0f, 3.0f), // 相机位置
    glm::vec3(0.0f, 0.0f, 0.0f), // 目标位置
    glm::vec3(0.0f, 1.0f, 0.0f)  // 上向量
);

glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

// 传递给 Shader
// glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(model));
```
