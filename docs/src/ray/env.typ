== 实验环境

- *编译器*：支持 C++20 的编译器（GCC 10+ / Clang 12+ / MSVC 2019+）
- *构建工具*：xmake
- *依赖库*：
  - GLM：用于向量和矩阵运算 （代码中已经包含）
  - OpenMP：用于多线程并行计算 （编译器通常自带支持）
  - STB Image：用于图像读写 （代码中已经包含）
  - meshark （来自 p2: mesh simplification），用于加载 wavefront obj 模型文件

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

#figure(
  image("assets/results/checker.png"),
  caption: "不同材质的球体渲染效果，包含景深效果和动态模糊",
)

#figure(
  image("assets/results/mesh.png", width: 60%),
  caption: "复杂网格模型渲染（p2 的 complex_bunny）",
)
#figure(image("assets/results/light.png", width: 60%), caption: "柏林噪声纹理和光源")

#figure(image("assets/results/complex.png", width: 50%), caption: "纹理贴图、体积雾、次表面散射")

#grid(columns: (1fr, 1fr), gutter: 1em)[
  #figure(image("assets/results/cornell-metal.png", width: 95%), caption: "康奈尔盒，金属材质")
][
  #figure(
    image("assets/results/cornell-metal-sample_light.png", width: 95%),
    caption: "康奈尔盒，金属材质，采样光源加速收敛",
  )
]

#figure(image("assets/results/cornell.png", width: 50%), caption: "康奈尔盒，玻璃材质")

#figure(
  image("assets/results/cornell-spectrum.png", width: 50%),
  caption: "康奈尔盒，使用光谱渲染实现色散效果",
)

渲染器支持多种场景和材质效果，包括：
- 漫反射材质（Lambertian）
- 金属材质（Metal）
- 电介质材质（Dielectric，如玻璃）
- 发光材质（Emissive Light）
- 纹理映射和柏林噪声
- 景深效果（Depth of Field）
- 运动模糊（Motion Blur）
- 体积雾与次表面散射

基本跟着 RayTracing in One Weekend 系列教程完整地实现了一遍，外加一些其他特性，比如光谱渲染、网格加载、多线程等。

代码量还挺大的，由于时间关系报告接下来的内容可能含有一些AI辅助生成的东西