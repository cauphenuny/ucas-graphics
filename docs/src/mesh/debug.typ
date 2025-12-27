#v(1em)

#let func(namespace, name, args: `()`) = [#text(black)[#namespace]#text(blue, size: 1.1em)[#strong[#name]]#text(
    black,
  )[#args]]

1. 简化的点位置不对

  现象：简化后的点位置不对，简化结果不理想，网格形状有很大的变化。

  分析：简化时新点的位置计算错误，可能是Q-matrix计算错误或者边cost没有正确更新。

  解决方案：重新分析了一下更新的逻辑，发现之前更新时没有考虑两步的影响，只考虑了当前节点的 Q-mat和直接相邻的边的cost，实际上要更新所以相邻点的Q-mat，更新所有二阶相邻边的cost。修改后简化结果符合预期。

2. 简化出来的网格有伪影

  这是简化 25% 的 cube，可以看到网格中有黑色的条形洞

  #align(center)[
    #image("assets/bug/cube_bug_artifact.png", width: 15em)
  ]

  分析：既然通过了 `checkMeshSanity()`，说明拓扑结构没有问题，应该是几何位置导致的，二分查找了造成问题的那一步简化，结果如下：

  #grid(
    columns: (1fr, 1fr),
    gutter: 1em,
    figure(image("assets/bug/cube_bug00.png"), caption: "简化前"),
    figure(image("assets/bug/cube_bug01.png"), caption: "简化后"),
  )

  可以看到是简化造成了一些面的方向翻转了

  解决：新增一个方法 #func(`GeometryMesh::`, `isCollapsable`, args: `(Edge e, glm::vec3 target)`)，在拓扑结构判断的基础上，判断新位置会不会造成面翻转，如果会，那么无法 collapse

  首先获取与一个点相邻的面片（不包含另一个点的，因为这个面会被删），然后在每一个面片中计算新位置与面片另外两个点的叉积，得到新的法线方向，和原法线方向做点积，如果小于0就说明翻转了。

  ```cpp
  bool GeometryMesh::isCollapsable(Edge e, glm::vec3 target) const {
      if (!HalfEdgeMesh::isCollapsable(e)) return false;
      auto v1 = e->firstVertex();
      auto v2 = e->secondVertex();
      namespace rv = std::ranges::views;
      auto check = [=, this](Vertex v1, Vertex v2) {
          // clang-format off
          auto halfedges = v1->outgoingHalfEdges()
                          | rv::filter([v2](HalfEdge h){ return h->tip != v2;});
          // clang-format on
          for (auto he : halfedges) {
              auto norm = normals(he->face);
              auto va = he->tip;
              auto vb = he->next->tip;
              auto cross = glm::cross(pos(va) - target, pos(vb) - target);
              if (glm::length(cross) < 1e-6) {
                  spdlog::warn("Collapse {} would result in degenerate face", e);
                  return false;  // collapse to line
              }
              auto new_norm = glm::normalize(cross);
              if (glm::dot(norm, new_norm) < 1e-3) {
                  spdlog::warn("Collapse {} would invert face {}", e, he->face);
                  return false;  // face inverted
              }
          }
          return true;
      };
      return check(v1, v2) && check(v2, v1);
  }
  ```

  加上这个判断后没有伪影问题了，以下是 25% 的效果：

  #figure(image("assets/screenshot/cube_triangle25.png", width: 20em))

  可以看到没有伪影了，网格也规则了很多

  这是最简的 18 条边的网格，依旧正常

  #figure(image("assets/screenshot/cube_triangle18edge.png", width: 20em))
