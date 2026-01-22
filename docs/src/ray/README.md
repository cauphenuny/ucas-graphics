# 光线追踪项目报告

## 文件结构

```
docs/src/ray/
├── main.typ        # 主报告文件
├── env.typ         # 实验环境介绍
├── code.typ        # 代码详解（三部分结构）
└── assets/
    └── badge.svg   # GitHub 徽章
```

## 编译方法

### 安装 typst

```bash
# 方法1：使用 cargo（推荐）
cargo install --locked typst-cli

# 方法2：从 GitHub 下载预编译二进制文件
# 访问 https://github.com/typst/typst/releases
```

### 编译报告

```bash
# 在项目根目录下执行
cd /path/to/ucas-graphics
typst c --root . docs/src/ray/main.typ docs/p3-ray.pdf
```

或者使用简化命令：

```bash
# 在 docs/src/ray 目录下
typst c --root ../../.. main.typ ../../../docs/p3-ray.pdf
```

## 报告内容

### 第一部分：基础架构与路径追踪器
- Vec3 向量运算
- Ray 和 Camera
- Hittable 和 Sphere
- 基础材质（Lambertian、Metal、Dielectric）
- 渲染管线（递归追踪、Gamma校正）
- 散焦模糊（景深效果）

### 第二部分：性能优化与复杂特性
- 运动模糊
- BVH 加速结构
- 纹理映射（Perlin噪声）
- 发光材质
- 新型几何体（Triangle、Quad）
- 参与介质（烟雾效果）

### 第三部分：蒙特卡洛积分与重要性采样
- 蒙特卡洛积分理论
- 重要性采样（PDF）
- 正交基 ONB
- 混合采样
- 康奈尔盒场景
- 现代渲染架构

## 注意事项

- 报告中的图片使用注释占位，实际使用时需要替换为真实的渲染结果图片
- 需要网络连接以下载 typst 包依赖
- 如果编译失败，请检查 typst 版本（推荐 0.11.0+）
