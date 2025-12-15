//
// Created by creeper on 7/20/24.
//
#include <meshark/mesh-simplifier.h>
#include <ranges>
#include <spdlog/spdlog.h>
#include <vector>
#include <mystl/views.h>

namespace meshark {

void MeshSimplifier::removeEdge(Edge e) {
    eraseEdgeMapping(e);
    edge_collapse_cost.removeEdgeData(e);
    mesh.removeEdge(e);
}

void MeshSimplifier::removeVertex(Vertex v) {
    Q.removeVertexData(v);
    mesh.removeVertex(v);
}

Vertex MeshSimplifier::collapseEdge(Edge e) {
    // DONE: Implement this function

    /*
       <-- {vx} <--
      /            \
    /ex1            \e2x
  /                  \
{v1} <e21-------e12> {v2}
  \                  /
   \e1y            /ey2
    \            /
     --> {vy}-->
    */

    spdlog::debug("Collapsing edge: {:?}", e);
    
    // NOTE: variable with underscore suffux means it would be deleted

    auto e12_ = e->halfEdge();
    auto e2X_ = e12_->next, eX1 = e2X_->next;
    auto eX2_ = e2X_->twin, e1X = eX1->twin;
    auto vX = e2X_->tip;

    spdlog::debug("Collapsing triangle #1:\n  e12: {:?}\n  e2X: {:?}\n  eX1: {:?}", e12_, e2X_, eX1);

    auto e21_ = e12_->twin;
    auto e1Y = e21_->next, eY2_ = e1Y->next;
    auto eY1 = e1Y->twin, e2Y_ = eY2_->twin;
    auto vY = e1Y->tip;

    spdlog::debug("Collapsing triangle #2:\n  e21: {:?}\n  e1Y: {:?}\n  eY2: {:?}", e21_, e1Y, eY2_);

    auto v1 = e21_->tip;
    auto v2_ = e12_->tip;

    spdlog::debug("Vertices:\n  v1: {:?}\n  v2: {:?}\n  vX: {:?}\n  vY: {:?}", v1, v2_, vX, vY);
    mesh.showTopology(v1), mesh.showTopology(v2_);
    mesh.showTopology(vX), mesh.showTopology(vY);

    auto e2A = eX2_->next, eAX = e2A->next;
    auto vA = e2A->tip;

    auto eYB = e2Y_->next, eB2 = eYB->next;
    auto vB = eYB->tip;

    spdlog::debug("Merging triangle #1:\n  vA: {:?}\n  eX2: {:?}\n  e2A: {:?}\n  eAX: {:?}", vA, eX2_, e2A, eAX);
    spdlog::debug("Merging triangle #2:\n  vB: {:?}\n  e2Y: {:?}\n  eYB: {:?}\n  eB2: {:?}", vB, e2Y_, eYB, eB2);

    auto fX_ = e12_->face;
    auto fY_ = e21_->face;
    auto fA = eX2_->face;
    auto fB = e2Y_->face;

    spdlog::debug("Faces:\n  fX: {:?}\n  fY: {:?}\n  fA: {:?}\n  fB: {:?}", fX_, fY_, fA, fB);

    // NOTE: do not change topological relations within the traversing loop!
    // gather in/out half-edges of v2 except e2/e2x/e2y (these would be deleted)

    // clang-format off
    auto out_v2 = v2_->outgoingHalfEdges()
                | std::views::filter([=](auto h) { return !(h == e21_ || h == e2X_ || h == e2Y_); })
                | std::ranges::to<std::vector>();

    auto in_v2 = out_v2
               | std::views::transform([](HalfEdge h) { return h->twin; })
               | std::ranges::to<std::vector>();
    // clang-format on

    spdlog::debug("Gathered out half-edges of v2: {:?}", out_v2);
    spdlog::debug("Gathered in half-edges of v2: {:?}", in_v2);

    // START!

    // merge v2.out, v2.in => v1
    for (auto out : out_v2) out->tail = v1;
    for (auto in : in_v2) in->tip = v1;

    // add v2.out, v1.in to v1's half-edge list
    for (auto out : out_v2) {
        out->twin->next = v1->halfEdge()->twin->next;
        v1->halfEdge()->twin->next = out;
    }

    // merge fX+fA => new fA (comprising e2A, eAX, eX1)
    // merge fY+fB => new fB (comprising eB2, e1Y, eYB)
    auto fA_boundary = std::vector{e2A, eAX, eX1};
    auto fB_boundary = std::vector{eB2, e1Y, eYB};
    for (auto [idx, he] : mystl::views::enumerate(fA_boundary)) {
        he->face = fA;
        he->next = fA_boundary[(idx + 1) % fA_boundary.size()];
    }
    for (auto [idx, he] : mystl::views::enumerate(fB_boundary)) {
        he->face = fB;
        he->next = fB_boundary[(idx + 1) % fB_boundary.size()];
    }

    // delete edges e2/e2x/e2y
    removeEdge(e), mesh.removeHalfEdge(e12_), mesh.removeHalfEdge(e21_);
    removeEdge(e2X_->edge), mesh.removeHalfEdge(e2X_), mesh.removeHalfEdge(eX2_);
    removeEdge(e2Y_->edge), mesh.removeHalfEdge(e2Y_), mesh.removeHalfEdge(eY2_);

    // delete faces fx/fy, vertex v2
    mesh.removeFace(fX_), mesh.removeFace(fY_);
    removeVertex(v2_);

    return v1;
}

MeshSimplifier::MinCostEdgeCollapsingResult MeshSimplifier::collapseMinCostEdge() {
    auto min_cost_edge = cost_edge_map.begin()->second;
    // DONE: finish this function
    if (mesh.isCollapsable(min_cost_edge)) {
        auto optimal_pos = computeOptimalCollapsePosition(min_cost_edge);
        auto vertex = collapseEdge(min_cost_edge);
        checkMeshSanity();
        updateVertexPos(vertex, optimal_pos);
        checkMeshSanity();
        return {Edge(), true};
    } else {
        return {min_cost_edge, false};
    }
}

Real MeshSimplifier::computeEdgeCost(Edge e) const {
    // DONE: Implement this function
    auto v1 = e->firstVertex();
    auto v2 = e->secondVertex();
    auto qmat = Q(v1) + Q(v2);
    auto merged = computeOptimalCollapsePosition(e);
    auto homo = glm::vec4(merged, 1.0f);
    auto cost = glm::dot(homo, qmat * homo);
    return cost;
}

void MeshSimplifier::runSimplify(Real alpha) {
    for (auto v : mesh.vertices()) Q(v) = computeQuadricMatrix(v);
    for (auto e : mesh.edges()) {
        edge_collapse_cost(e) = computeEdgeCost(e);
        cost_edge_map.insert({edge_collapse_cost(e), e});
    }
    int round = 0;
    while (mesh.numEdges() > alpha * num_original_edges) {
        spdlog::info("Round {}: ", round);
        checkMeshSanity();
        auto result = collapseMinCostEdge();
        round++;
        if (!result.is_collapsable) {
            auto e = result.failed_edge;
            updateEdgeCost(e, std::numeric_limits<Real>::infinity());
            spdlog::warn("Min-cost edge is not collapsable, skip");
            continue;
        }
        spdlog::info("{} edges left\n", mesh.numEdges());
    }
}

glm::vec3 MeshSimplifier::computeOptimalCollapsePosition(Edge e) const {
    // DONE: computes formula argmin_{v}(v^T Q v)
    auto v1 = e->firstVertex();
    auto v2 = e->secondVertex();
    glm::mat4 qmat = Q(v1) + Q(v2);
    glm::mat3 coef(qmat);
    glm::vec3 rhs(qmat[3][0], qmat[3][1], qmat[3][2]);
    if (glm::determinant(coef) < 1e-6) {
        return (mesh.pos(v1) + mesh.pos(v2)) * 0.5f;
    }
    auto pos = -glm::inverse(coef) * rhs;
    return pos;
}

void MeshSimplifier::updateVertexPos(Vertex v, const glm::vec3& pos) {
    // DONE: implement this function
    mesh.setVertexPos(v, pos);
    for (auto h : v->outgoingHalfEdges()) {
        auto adj = h->edge;
        updateEdgeCost(adj, computeEdgeCost(adj));
        auto opp = h->next->edge;
        updateEdgeCost(opp, computeEdgeCost(opp));
    }
}

glm::mat4 MeshSimplifier::computeQuadricMatrix(Vertex v) const {
    // DONE: implement this function
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

void MeshSimplifier::eraseEdgeMapping(Edge e) {
    Real cost = edge_collapse_cost(e);
    auto range = cost_edge_map.equal_range(cost);
    assert(range.first != cost_edge_map.end());
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == e) {
            cost_edge_map.erase(it);
            break;
        }
    }
}

void MeshSimplifier::checkMeshSanity() {
    if (spdlog::default_logger()->level() >= spdlog::level::info) return;
    spdlog::debug("Checking Sanity...");
    assert(mesh.numEdges() == edge_collapse_cost.size());
    assert(mesh.numVertices() == Q.size());
    assert(mesh.numEdges() * 2 == mesh.numHalfEdges());

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

}  // namespace meshark
