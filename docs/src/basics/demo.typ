== demo

#align(center)[
  #grid(
    columns: (1fr, 1fr),
    figure(image("assets/computer0.png", width: 80%), caption: "computer"),
    figure(image("assets/computer1.png", width: 80%), caption: "computer, theme=xterm-dark"),
  )
]

#align(center)[
  #grid(
    columns: (1fr, 1fr),
    figure(image("assets/painter-demo0.png", width: 80%), caption: "画板画图demo"),

    figure(image("assets/painter-demo1.png", width: 80%), caption: "画板选项栏")
  )
]

== 操作说明

#v(0.5em)

画板功能：

- 支持绘制直线、折线、矩形、圆形、三角形、多边形：鼠标左键选点，右键或者 ESC 提交当前图形

- 支持撤销操作：按 Backspace 撤销上一个图形

- 支持的绘制选项：描边颜色、描边宽度、填充颜色、是否圆角（矩形）。按空格打开菜单或者绘制完图形后自动打开填充颜色选项栏，菜单打开后使用左键切换选项，右键/ESC退出。