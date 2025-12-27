== 编译环境

环境与实验一相同，没有引入新的依赖库

```lua
add_rules("mode.debug", "mode.release")
set_languages("c99", "c++20")
add_requires("opengl", {system = true})
add_requires("glut", {system = true})
add_requires("glfw3", {system = true})
add_requires("spdlog", {system = true})
add_requires("magic_enum")

target("project1")
    set_kind("binary")
    set_warnings("all")
    add_files("src/basics/*.cpp")
    add_packages("opengl")
    add_packages("glut")
    add_packages("spdlog")
    add_packages("glfw3")
    add_packages("magic_enum")
    if is_plat("macosx") then
        add_frameworks("Cocoa", "CoreFoundation", "IOKit")
    end

target("meshark")
    set_kind("static")
    add_files("src/mesh/meshark/src/*.cc")
    add_includedirs("src/mesh/meshark/include", {public = true})
    add_includedirs("src/mesh/external/glm", {public = true})
    add_headerfiles("src/mesh/meshark/include/(**.h)")
    add_packages("spdlog")

target("project2")
    set_kind("binary")
    add_files("src/mesh/meshark/apps/simplify.cc")
    add_deps("meshark")
    add_packages("spdlog")
    set_rundir("$(projectdir)")
```

== 运行方法

```
xmake run project2 <input obj path> <output obj path> <ratio|num_edges> [-v|-vv]
```

支持输入简化目标比例或目标边数

`-v/-vv` 控制 logger 输出等级

e.g.

```
xmake run project2 path/to/obj/sphere.obj path/to/out/sphere25.obj 0.25
xmake run project2 path/to/obj/sphere.obj path/to/out/sphere60e.obj 60 -vv
```

== 运行效果

以下展示了不同模型在不同简化率下的效果。其中 100% 表示原始模型（未简化），75%、50%、25% 分别表示保留 75%、50%、25% 的边数。

#let levels = (100, 75, 50, 25)
#let names = ("armadillo", "complex_bunny", "cube_triangle", "sphere", "spot", "torus")
#let base_path = "assets/screenshot/"

// TODO: display as matrix
#grid(
  columns: (1fr, 1fr, 1fr, 1fr),
  align: center + horizon,
  gutter: 1em,
  ..levels.map(level => [$#level$ %]),
  ..levels.map(level => [#image(base_path + "armadillo" + str(level) + ".png", width: 8em)]),
  ..levels.map(level => [#image(base_path + "complex_bunny" + str(level) + ".png", width: 8em)]),
  ..levels.map(level => [#image(base_path + "cube_triangle" + str(level) + ".png", width: 8em)]),
  ..levels.map(level => [#image(base_path + "sphere" + str(level) + ".png", width: 8em)]),
  ..levels.map(level => [#image(base_path + "spot" + str(level) + ".png", width: 8em)]),
  ..levels.map(level => [#image(base_path + "torus" + str(level) + ".png", width: 8em)]),
)

=== 实验结果分析

从实验结果可以看出：

- *几何形状保持*：QEM 算法在简化过程中能够较好地保持模型的整体形状特征。即使在 25% 的简化率下，大部分模型的主要几何特征仍然清晰可见。

- *不同模型的简化效果*：
  - *简单几何体*（如 `sphere`、`cube_triangle`、`torus`）：简化效果较好，即使在极低简化率下也能保持较好的形状。
  - *复杂模型*（如 `armadillo`、`complex_bunny`、`spot`）：在较高简化率（75%、50%）下效果良好，但在 25% 简化率下会出现一些细节丢失，这是预期的结果。

- *算法稳定性*：通过面翻转检测等机制，算法能够避免产生拓扑错误和视觉伪影，所有简化结果都保持了正确的网格拓扑结构。