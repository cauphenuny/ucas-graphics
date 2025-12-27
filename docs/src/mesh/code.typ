#import "@preview/mitex:0.2.6": *
#import "@preview/theorion:0.4.1": *
// #import cosmos.fancy: *
#import cosmos.rainbow: *
// #import cosmos.clouds: *
#show: show-theorion

== Implementation

=== Mesh Elements

#let highlight(content) = text(blue)[#content]

#let func(namespace, name, args: `()`) = [#text(black)[#namespace]#text(blue, size: 1.1em)[#strong[#name]]#text(
    black,
  )[#args]]

- #func(`VertexElement::OutgoingHalfEdgeIterator::`, `operator++`)

  首先是实现顶点出边迭代器的 operator++ 方法

  这里我重构了一下框架代码，从创建一个新类 `OutgoingHalfEdgeRange` 并实现 `OutgoingHalfEdgeRange::Iterator` 改成直接实现一个 iterator，然后通过 `std::ranges::subrange` 从 start/end iterator 创建 range。

  这样的好处一个是写起来方便、避免重复造轮子，二个是这个 `std::ranges::subrange` 是满足 `std::ranges::range` concept 的，可以方便地用 `ranges` 提供的一些基础设施，如果想要自定义类满足这个concept的话可能还需要写一些繁琐的东西。

  总之实现是这样：

  ```cpp
  struct VertexElement::OutgoingHalfEdgeIterator {
      using difference_type = std::ptrdiff_t;
      using value_type = HalfEdge;

      OutgoingHalfEdgeIterator& operator++() {
          it = it->twin->next;
          if (it == start) it = static_cast<HalfEdge>(nullptr);
          return *this;
      }
      OutgoingHalfEdgeIterator operator++(int) {
          OutgoingHalfEdgeIterator temp = *this;
          ++(*this);
          return temp;
      }

      HalfEdge operator*() const { return it; }

      bool operator==(const OutgoingHalfEdgeIterator& other) const { return it == other.it; }

      HalfEdge start{nullptr};
      HalfEdge it{nullptr};
  };
  ```

  （`difference_type` 和 `value_type` 是 `std::iterator_traits` 推导需要的东西）

  ```cpp
  [[nodiscard]] auto VertexElement::outgoingHalfEdges() const {
      return std::ranges::subrange<OutgoingHalfEdgeIterator>{
          OutgoingHalfEdgeIterator{he, he}, OutgoingHalfEdgeIterator{he, HalfEdge{nullptr}}};
  }
  ```

  顺便给 `FaceElement::boundaryHalfEdge` 也重构了一下

=== Mesh Simplifier

==== Select Edges

#let meshfunc(name, args: `()`) = func(`MeshSimplifier::`, name, args: args)

+ #meshfunc(`computeQuadricMatrix`, args: `(Vertex v)`)

  #mitex(
    `
\begin{align}
\Delta(v_{a}\to v)
&=\sum_{p \in \text{plane}(v_{a})}(pv^\top)^{2} \\
&=\sum_{p \in \text{plane}(v_{a})}(pv^{\top})(pv^{\top}) \\
&=\sum_{p \in \text{plane}(v_{a})}(pv^{\top})^{\top}(pv^{\top}) \quad \text{(} pv^{\top}\text{ is scalar)} \\
&=\sum_{p \in \text{plane}(v_{a})}vp^{\top}pv^{\top} \\
&=v\left(\sum_{p \in \text{plane}(v_{a})}p^{\top}p\right)v^{\top} \\
&=vQ(v_{a})v^{\top}
\end{align}
  `,
  )

  #theorem[
    $
      Q(v)=(sum_(p in "plane"(v)) p^(tack.b)p)
    $
  ]

  根据上述公式可得 quadric matrix 的计算方法，即对与该顶点相邻的所有面的平面方程系数向量 p，计算 $p^(tack.b)p$ 的和。

  ```cpp
  glm::mat4 MeshSimplifier::computeQuadricMatrix(Vertex v) const {
      auto result = glm::mat4(0.0f);
      for (auto h : v->outgoingHalfEdges()) {
          auto f = h->face;
          if (!f) continue;
          auto n = glm::normalize(mesh.normal(f));
          auto p = mesh.pos(h->tip);
          Real d = -glm::dot(n, p);
          glm::vec4 plane(n, d);
          result += glm::outerProduct(plane, plane);
      }
      return result;
  }
  ```

+ #meshfunc(`computeOptimalCollapsePosition`, args: `(Edge e)`)

  合并 $(v_1, v_2)->v$ 的误差为：

  #let v1 = $v_1$
  #let v2 = $v_2$

  $
    Delta(v)=Delta(v_1->v)+Delta(v2->v)=v (Q_v1+Q_v2) v^(tack.b)
  $

  #mitex(
    `
  \Delta=\begin{pmatrix}x & y & z & 1\end{pmatrix}\begin{pmatrix}q_{11} & q_{12} & q_{13} & q_{14} \\ q_{21} & q_{22} & q_{23} & q_{24} \\ q_{31} & q_{32} & q_{33} & q_{34} \\ q_{41} & q_{42} & q_{43} & q_{44}\end{pmatrix}\begin{pmatrix}x \\ y \\ z \\ 1\end{pmatrix}=\sum_{i=1}^{4}\sum_{i=1}^{4}v_{i}v_{j}q_{ij}
  `,
  )

  求解

  #mitex(`\dfrac{\partial\Delta}{\partial x}=\dfrac{\partial\Delta}{\partial y}=\dfrac{\partial\Delta}{\partial z}=0`)

  hint: #mi(`\dfrac{\partial (\mathbf{x}^{T}\mathbf{A}\mathbf{x})}{\partial \mathbf{x}}=(\mathbf{A}+\mathbf{A}^{T})\mathbf{x}`)

  可得：

  #theorem[
    最优点 $(x,y,z)$ 满足
    #mitex(
      `\begin{pmatrix}
q_{11} & q_{12} & q_{13}  & q_{14}\\ q_{21} & q_{22} & q_{23}  & q_{24}\\ q_{31} & q_{32} & q_{33}  & q_{34} \\
\end{pmatrix}\begin{pmatrix}
x \\
y \\
z \\
\end{pmatrix}
=
-\begin{pmatrix}
q_{14} \\
q_{24} \\
q_{34} \\
\end{pmatrix}`,
    )
  ]

  ```cpp
  glm::vec3 MeshSimplifier::computeOptimalCollapsePosition(Edge e) const {
      auto v1 = e->firstVertex();
      auto v2 = e->secondVertex();
      glm::mat4 qmat = Q(v1) + Q(v2);
      glm::mat3 coef(qmat);
      glm::vec3 rhs(qmat[3][0], qmat[3][1], qmat[3][2]);
      spdlog::trace(
          "Computing optimal collapse position for edge {} (det={:.2e}):", e, glm::determinant(coef));
      if (glm::determinant(coef) < 1e-6) {
          spdlog::debug("Quadric matrix is singular when collapsing {}, using midpoint", e);
          return (mesh.pos(v1) + mesh.pos(v2)) * 0.5f;
      }
      auto pos = -glm::inverse(coef) * rhs;
      return pos;
  }
  ```

+ #meshfunc(`computeEdgeCost`, args: `(Edge e)`)

  根据 QEM 算法，用 quadric matrix 和 new_pos 计算出 cost

  ```cpp
  Real MeshSimplifier::computeEdgeCost(Edge e) const {
      auto v1 = e->firstVertex();
      auto v2 = e->secondVertex();
      auto qmat = Q(v1) + Q(v2);
      auto merged = computeOptimalCollapsePosition(e);
      auto homo = glm::vec4(merged, 1.0f);
      auto cost = glm::dot(homo, qmat * homo);
      return cost;
  }
  ```

==== Collapse Edges

+ #meshfunc(`removeEdge`), #meshfunc(`removeVertex`)

  由于 `MeshSimplifier` 含有 edge/vertex data，所以需要在删除边/点时维护一下

  ```cpp
  void MeshSimplifier::removeEdge(Edge e) {
      eraseEdgeMapping(e);
      edge_collapse_cost.remove(e);
      mesh.removeEdge(e);
  }

  void MeshSimplifier::removeVertex(Vertex v) {
      Q.remove(v);
      mesh.removeVertex(v);
  }
  ```

+ #meshfunc(`collapseEdge`, args: `(Edge e)`)

  这是debug最久的一块 (

  一开始的时候想错了，与其说是删除3条边和它对应的两条半边，不如说是删掉两个完整的面和它的 boundaryHalfEdge，后面的这个思路更符合 halfedge-mesh 的结构，debug 也很顺利。

  假设 $e$ 的两个端点分别是 $v_1, v_2$，我们将 $v_2$ 合并到 $v_1$

  #figure(image("assets/collapse_edge.png", width: 30em), caption: "示意图，其中红色部分是将被删除的元素")

  首先取出 $v_2$ 的出入边（属于 $f_x,f_y$ 的会被删除，不算）

  ```cpp
  auto v2_edges = v2_->outgoingHalfEdges()
                | std::views::filter([=](HalfEdge h) { return !(h->face == fX_ || h->face == fY_); })
                | std::views::transform([](HalfEdge h) { return std::make_pair(h, h->next->next); });
  ```

  将取出来的出入边重定向到 $v_1$

  ```cpp
  for (auto [out, in] : v2_edges) {
      out->tail = v1;
      in->tip = v1;
  }
  ```

  然后是绑定图中对应的两条黑色实线半边 (`e1X/eX2, eY1/e2Y`) 为一条新的边

  ```cpp
  auto bind = [this](HalfEdge h1, HalfEdge h2) {
      h1->twin = h2;
      h2->twin = h1;
      removeEdge(h1->edge);
      h1->edge = h2->edge;
      h1->edge->halfEdge() = h1;
  };
  bind(e1X, eX2);
  bind(eY1, e2Y);
  ```

  删除 $f_x,f_y$

  ```cpp
  for (auto he : {eX1_, e12_, e2X_}) mesh.removeHalfEdge(he);
  for (auto he : {eY2_, e21_, e1Y_}) mesh.removeHalfEdge(he);
  mesh.removeFace(fX_);
  mesh.removeFace(fY_);
  ```


  由于 $v_1,v_x,v_y$ 的 `halfEdge()` 可能被删了，需要重新设定一下

  ```cpp
  v1->halfEdge() = e1X;
  vX->halfEdge() = eX2;
  vY->halfEdge() = eY1;
  ```

  删除 `v2, e`

  ```cpp
  removeVertex(v2_);
  removeEdge(e);
  ```

+ #meshfunc(`updateVertexPos`, args: `(Vertex v, const glm::vec3& pos)`)

  设置点的坐标，然后更新所有可能被影响到的点的 Q-mat

  被更新的 Q-mat 又会造成周围边的 cost 更新

  ```cpp
  void MeshSimplifier::updateVertexPos(Vertex v, const glm::vec3& pos) {
      mesh.setVertexPos(v, pos);
      Q(v) = computeQuadricMatrix(v);
      for (auto h : v->outgoingHalfEdges()) {
          Q(h->tip) = computeQuadricMatrix(h->tip);
      }
      for (auto h : v->outgoingHalfEdges()) {
          for (auto h2 : h->tip->outgoingHalfEdges()) {
              auto e = h2->edge;
              updateEdgeCost(e, computeEdgeCost(e));
          }
      }
  }
  ```

+ #meshfunc(`collapseMinCoseEdge`)

  找到最小cost的边，然后尝试collapse

  ```cpp
  MeshSimplifier::MinCostEdgeCollapsingResult MeshSimplifier::collapseMinCostEdge() {
      auto [cost, min_cost_edge] = *cost_edge_map.begin();
      spdlog::debug("Collapsing min-cost {} with cost {}", min_cost_edge, cost);
      if (cost == std::numeric_limits<Real>::infinity()) {
          spdlog::error("All remaining edges have infinite cost, can not find collapsable edge");
          exit(1);
      }
      auto optimal_pos = computeOptimalCollapsePosition(min_cost_edge);
      if (mesh.isCollapsable(min_cost_edge, optimal_pos)) {
          auto vertex = collapseEdge(min_cost_edge);
          checkMeshSanity();
          updateVertexPos(vertex, optimal_pos);
          return {Edge(), true};
      } else {
          return {min_cost_edge, false};
      }
  }
  ```

+ #meshfunc(`runSimplify`, args: `(std::variant<int, Real> target)`)

  主循环，计算初始 Q-mat, cost，然后开始 collapse

  ```cpp
  void MeshSimplifier::runSimplify(std::variant<int, Real> target) {
      for (auto v : mesh.vertices()) Q(v) = computeQuadricMatrix(v);
      for (auto e : mesh.edges()) {
          edge_collapse_cost(e) = computeEdgeCost(e);
          cost_edge_map.insert({edge_collapse_cost(e), e});
      }
      checkMeshSanity();

      int target_edges = Match{target}(
          [](int num) -> int { return num; },
          [&](Real ratio) -> int { return ratio * num_original_edges; });
      spdlog::info("Target: {} edges", target_edges);

      int round = 0;
      while (mesh.numEdges() > target_edges) {
          spdlog::info(
              "Round {} ({} vertices, {} edges, {} faces)", round, mesh.numVertices(),
              mesh.numEdges(), mesh.numFaces());
          auto result = collapseMinCostEdge();
          round++;
          if (!result.is_collapsable) {
              auto e = result.failed_edge;
              updateEdgeCost(e, std::numeric_limits<Real>::infinity());
              spdlog::warn("Min-cost edge is not collapsable, skip");
              continue;
          }
      }
  }
  ```

  其中 `Match` 是一个封装了 `std::visit` 的小工具，可以方便地对 `variant` 进行模式匹配

  ```cpp
  template <typename... Ts> struct Visitor : Ts... {
      using Ts::operator()...;
  };

  template <typename... Ts> Visitor(Ts...) -> Visitor<Ts...>;

  template <typename T> struct Match {
      T value;
      Match(T&& value) : value(std::forward<T>(value)) {}
      template <typename... Ts> auto operator()(Ts&&... params) {
          return std::visit(Visitor{std::forward<Ts>(params)...}, std::forward<T>(value));
      }
      template <typename... Ts> auto operator()(Visitor<Ts...> visitor) {
          return std::visit(visitor, std::forward<T>(value));
      }
  };

  template <typename T> Match(T&&) -> Match<T&&>;
  ```

+ #func(`GeometryMesh::`, `isCollapsable`, args: `(Edge e, glm::vec3 target)`)

  这个函数在 `HalfEdgeMesh::isCollapsable()` 的基础上增加了一个判断，在 bug 分析部分详细描述。

== Refactoring

除了上述的对 iterator 的重构以外，还有一个小重构：

框架中的 `VertexData<Type>/EdgeData<Type>/FaceData<Type>` 几乎一模一样，用模板类 `ElementData<ElementType, T>` 抽象出来似乎更好一些：

```cpp
template <typename Element, typename Type> struct ElementData {
    using T = std::conditional_t<std::is_same_v<Type, bool>, std::byte, Type>;
    // 怎么还有绕过 vector<bool> 的小巧思（笑）
    ElementData() = default;
    explicit ElementData(int num_vertices) : data(num_vertices) {}
    T& operator()(Element elem) { return data[elem->index]; }
    const T& operator()(Element elem) const { return data[elem->index]; }
    void append(const T& t) { data.push_back(t); }
    void remove(Element elem) {
        if (elem->index == data.size() - 1) {
            data.pop_back();
            return;
        }
        std::swap(data[elem->index], data.back());
        data.pop_back();
    }
    size_t size() const { return data.size(); }

private:
    std::vector<T> data{};
};

template <typename Type> using VertexData = ElementData<Vertex, Type>;
template <typename Type> using EdgeData = ElementData<Edge, Type>;
template <typename Type> using FaceData = ElementData<Face, Type>;
```

== Utility / Debug Tools

这里是一些相对不那么重要的工具代码

+ #meshfunc(`checkMeshSanity`)

  用来检查 halfedge-mesh 的完整性，debug 过程中非常有用，可以迅速发现问题

  ```cpp
  void MeshSimplifier::checkMeshSanity() const {
      if (spdlog::default_logger()->level() >= spdlog::level::info) return;
      spdlog::debug("Checking Sanity...");
      assert(mesh.numEdges() == edge_collapse_cost.size());
      assert(mesh.numVertices() == Q.size());
      assert(mesh.numEdges() * 2 == mesh.numHalfEdges());
      assert(mesh.numFaces() * 3 == mesh.numHalfEdges());

      for (auto he : mesh.halfEdges()) {
          assert(he->twin->twin == he);
          assert(he->next->tail == he->tip);
          assert(he->edge->halfEdge() == he || he->edge->halfEdge() == he->twin);
          assert(he->face->halfEdge());
      }

      int sum_of_degrees = 0;
      for (auto v : mesh.vertices()) {
          int degree = 0;
          for (auto h : v->outgoingHalfEdges()) {
              assert(h->tail == v);
              degree++;
          }
          sum_of_degrees += degree;
      }
      if (sum_of_degrees != mesh.numEdges() * 2) {
          spdlog::error("sum_of_degrees = {}, expected = {}", sum_of_degrees, mesh.numEdges() * 2);
          // diagnostic: find half-edges not present in any vertex outgoing list
          std::vector<char> seen(mesh.numHalfEdges(), 0);
          for (auto v : mesh.vertices()) {
              for (auto h : v->outgoingHalfEdges()) {
                  if (h) seen[h->getIndex()] = 1;
              }
          }
          std::vector<int> missing;
          for (auto he : mesh.halfEdges()) {
              if (!seen[he->getIndex()]) missing.push_back(he->getIndex());
          }
          spdlog::error("missing half-edges (count={}): {}", missing.size(), missing);
          for (int idx : missing) {
              auto he = mesh.halfEdge(idx);
              spdlog::error(
                  "HalfEdge({}) tail={}, tip={}, twin={}, next={}, edge={}, face={}", idx, he->tail,
                  he->tip, he->twin ? he->twin->getIndex() : -1, he->next ? he->next->getIndex() : -1,
                  he->edge, he->face);
          }
      }
      assert(sum_of_degrees == mesh.numEdges() * 2);

      for (auto e : mesh.edges()) {
          assert(e->halfEdge()->edge == e);
          assert(e->halfEdge()->twin->edge == e);
      }

      for (auto f : mesh.faces()) {
          for (auto h : f->boundaryHalfEdges()) {
              if (h->face != f) {
                  spdlog::error("HalfEdge {} has face {}, expected {}", h, h->face, f);
                  spdlog::error("Face {} boundary half-edges: {:?}", f, f->boundaryHalfEdges());
              }
              assert(h->face == f);
          }
      }
  }
  ```

+ `struct fmt::formatter<T>`

  写了一些给打log用的formatter，这里就不放进来了，代码见 `mystl/fmt.h`

+ #func(`MeshElement::`, `repr`), #func(`MeshElement::`, `str`)

  写了这两个函数，把formatter暴露给 gdb/lldb，不然debug的时候没法复用formatter的功能。（名字抄自 python 的 `__repr__/__str__`）

  ```cpp
  template <typename Type> struct ToString : StaticPolymorphism<Type> {
      [[gnu::used, gnu::noinline, gnu::visibility("default")]]  // for using it in gdb/lldb
      std::string repr() const {
          static_assert(fmt::formattable<Type>, "type must be formattable by fmt");
          // use debug format if available
          if constexpr (requires { fmt::format("{:?}", this->derived()); }) {
              return fmt::format("{:?}", this->derived());
          } else {
              return fmt::format("{}", this->derived());
          }
      }
      [[gnu::used, gnu::noinline, gnu::visibility("default")]]
      std::string str() const {
          static_assert(fmt::formattable<Type>, "type must be formattable by fmt");
          return fmt::format("{}", this->derived());
      }
  };
  ```
