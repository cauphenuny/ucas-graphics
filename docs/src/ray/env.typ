== 实验环境

本实验使用以下环境：

- *操作系统*：macOS / Linux
- *编译器*：支持 C++20 的编译器（GCC 10+ / Clang 12+ / MSVC 2019+）
- *构建工具*：xmake
- *依赖库*：
  - GLM：用于向量和矩阵运算
  - OpenMP：用于多线程并行计算
  - STB Image：用于图像读写

== 编译与运行

```bash
# 编译项目
xmake

# 运行不同的场景
xmake run project3 0  # final scene 0
xmake run project3 1  # final scene 1
xmake run project3 2  # cornell box
xmake run project3 3  # earth texture
xmake run project3 4  # perlin noise
xmake run project3 5  # basic shapes
xmake run project3 6  # mesh rendering
xmake run project3 7  # lighting demo
```

== 运行效果

// 以下为渲染效果图示例，实际图片需要替换
// #figure(
//   image("assets/cornell_box.png", width: 80%),
//   caption: "康奈尔盒渲染效果"
// )

// #figure(
//   image("assets/final_scene.png", width: 80%),
//   caption: "最终场景渲染效果"
// )

// #figure(
//   image("assets/materials.png", width: 80%),
//   caption: "不同材质渲染效果（漫反射、金属、玻璃）"
// )

渲染器支持多种场景和材质效果，包括：
- 漫反射材质（Lambertian）
- 金属材质（Metal）
- 电介质材质（Dielectric，如玻璃）
- 发光材质（Emissive Light）
- 纹理映射和柏林噪声
- 景深效果（Depth of Field）
- 运动模糊（Motion Blur）
