#import "@preview/codly:1.3.0": codly
#import "@preview/codly-languages:0.1.10": codly-languages
#codly(display-icon: false)
#codly(languages: codly-languages)

== 依赖项

基础依赖：比较新的C++编译器、`xmake`、`opengl`、`glfw`

为了让coding比较舒服，加了点别的库：`fmtlib, spdlog, magic_enum`

#let xmake-lua = read("../../../xmake.lua")

#figure(
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
      add_files("src/basics/*.cpp")
      add_packages("opengl")
      add_packages("glut")
      add_packages("spdlog")
      add_packages("glfw3")
      add_packages("magic_enum")
      if is_plat("macosx") then
          add_frameworks("Cocoa", "CoreFoundation", "IOKit")
      end
  ```,
  caption: "xmake.lua",
)

由于在我的 macOS 环境下 xmake/vcpkg 安装的 `opengl/glfw/spdlog` 库有点问题，所以改成用系统的库了，在编译前需要先安装这些依赖。

header-only 的 `magic_enum` 没有问题，能直接从 xmake 装。

== 编译&运行

```sh
xmake
xmake run project1
```

会先启动一个显示小电脑的窗口，关闭后，进入画板，可以画图

启动的时候可以选择主题
```sh
xmake run project1 catppuccin # default
xmake run project1 xterm-dark
xmake run project1 catppuccin-dark
```
