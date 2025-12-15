//
// Created by creeper on 7/20/24.
//
#include <meshark/mesh-simplifier.h>
#include <mystl/views.h>
#include <range/v3/all.hpp>
#include <spdlog/spdlog.h>
#include <vector>

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

    spdlog::trace("Collapsing edge: {:?&}", e);

    // NOTE: variable with underscore suffux means it would be deleted

    auto e12_ = e->halfEdge();
    auto e2X_ = e12_->next, eX1_ = e2X_->next;
    auto eX2 = e2X_->twin, e1X = eX1_->twin;
    auto vX = e2X_->tip;

    spdlog::trace(
        "Collapsing triangle #1:\n  e12: {:?&}\n  e2X: {:?&}\n  eX1: {:?&}", e12_, e2X_, eX1_);

    auto e21_ = e12_->twin;
    auto e1Y_ = e21_->next, eY2_ = e1Y_->next;
    auto eY1 = e1Y_->twin, e2Y = eY2_->twin;
    auto vY = e1Y_->tip;

    spdlog::trace(
        "Collapsing triangle #2:\n  e21: {:?&}\n  e1Y: {:?&}\n  eY2: {:?&}", e21_, e1Y_, eY2_);

    auto v1 = e21_->tip;
    auto v2_ = e12_->tip;

    spdlog::trace("Vertices:\n  v1: {:?&}\n  v2: {:?&}\n  vX: {:?&}\n  vY: {:?&}", v1, v2_, vX, vY);

    auto fX_ = e12_->face;
    auto fY_ = e21_->face;

    spdlog::trace("Faces:\n  fX: {:?&}\n  fY: {:?&}\n", fX_, fY_);

    // gather in/out half-edges of v2 except e2/e2x/e2y (these would be deleted)

    std::vector<std::pair<HalfEdge, HalfEdge>> v2_edges =
        v2_->outgoingHalfEdges() |
        ranges::views::filter([=](HalfEdge h) { return !(h->face == fX_ || h->face == fY_); }) |
        ranges::views::transform([](HalfEdge h) { return std::make_pair(h, h->next->next); }) |
        ranges::to<std::vector>();

    spdlog::trace(
        "Gathered out half-edges of v2: {:?&}",
        v2_edges | ranges::views::transform([](auto p) { return p.first; }));

    // START!

    // merge v2.out, v2.in => v1
    for (auto [out, in] : v2_edges) {
        out->tail = v1;
        in->tip = v1;
    }

    // bind e1X/eX2 to an edge
    // bind eY1/e2Y to an edge
    auto bind = [this](HalfEdge h1, HalfEdge h2) {
        h1->twin = h2;
        h2->twin = h1;
        removeEdge(h1->edge);
        h1->edge = h2->edge;
        h1->edge->halfEdge() = h1;
    };
    bind(e1X, eX2);
    bind(eY1, e2Y);

    // delete fX/fY
    for (auto he : {eX1_, e12_, e2X_}) mesh.removeHalfEdge(he);
    for (auto he : {eY2_, e21_, e1Y_}) mesh.removeHalfEdge(he);
    mesh.removeFace(fX_);
    mesh.removeFace(fY_);

    // reset halfEdge() because the deleted e12_/eX1_/eY2_ might be halfEdge() before
    v1->halfEdge() = e1X;
    vX->halfEdge() = eX2;
    vY->halfEdge() = eY1;

    // delete v2 and e
    removeEdge(e);
    removeVertex(v2_);

    return v1;
}

MeshSimplifier::MinCostEdgeCollapsingResult MeshSimplifier::collapseMinCostEdge() {
    // DONE: finish this function
    auto [cost, min_cost_edge] = *cost_edge_map.begin();
    spdlog::debug("Collapsing min-cost {} with cost {}", min_cost_edge, cost);
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
        spdlog::info(
            "Round {} ({} vertices, {} edges, {} faces)", round, mesh.numVertices(),
            mesh.numEdges(), mesh.numFaces());
        checkMeshSanity();
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

glm::vec3 MeshSimplifier::computeOptimalCollapsePosition(Edge e) const {
    // DONE: computes formula argmin_{v}(v^T Q v)
    auto v1 = e->firstVertex();
    auto v2 = e->secondVertex();
    glm::mat4 qmat = Q(v1) + Q(v2);
    glm::mat3 coef(qmat);
    glm::vec3 rhs(qmat[3][0], qmat[3][1], qmat[3][2]);
    if (glm::determinant(coef) < 1e-6) {
        spdlog::warn("Quadric matrix is singular when collapsing {}, using midpoint instead", e);
        return (mesh.pos(v1) + mesh.pos(v2)) * 0.5f;
    }
    auto pos = -glm::inverse(coef) * rhs;
    return pos;
}

void MeshSimplifier::updateVertexPos(Vertex v, const glm::vec3& pos) {
    // DONE: implement this function
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
