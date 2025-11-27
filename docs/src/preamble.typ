#import "theme.typ"
#import theme: *
#let split-full = pad(top: 1.5em, bottom: 1.5em)[#align(center, line(stroke: 0.04em, length: 100%))]
#let split-semi = pad(top: 1.5em, bottom: 1.5em)[#align(center, line(stroke: 0.04em, length: 60%))]
#let split = split-semi
#let conf(title: none, doc) = theme.conf(
  title: title,
  semister: "2025秋",
  course: "CG Lab",
  author: "袁晨圃" + emph[2023K8009929012],
  doc,
)

