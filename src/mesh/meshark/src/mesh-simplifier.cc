//
// Created by creeper on 7/20/24.
//
#include <meshark/mesh-simplifier.h>
#include <spdlog/spdlog.h>

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

    auto e12 = e->halfEdge();
    auto e2x = e12->next, ex1 = e2x->next;
    auto ex2 = ex1->twin, e1x = ex1->twin;

    spdlog::debug("First triangle:\n  e12: {:?}\n  e2x: {:?}\n  ex1: {:?}", e12, e2x, ex1);

    auto e21 = e12->twin;
    auto e1y = e21->next, ey2 = e1y->next;
    auto ey1 = e1y->twin, e2y = ey2->twin;

    spdlog::debug("Second triangle:\n  e21: {:?}\n  e1y: {:?}\n  ey2: {:?}", e21, e1y, ey2);

    auto v1 = e21->tip;
    auto v2 = e12->tip;
    auto vx = e2x->tip;
    auto vy = e1y->tip;

    spdlog::debug("Vertices:\n  v1: {:?}\n  v2: {:?}\n  vx: {:?}\n  vy: {:?}", v1, v2, vx, vy);

    auto fx = e12->face;
    auto fy = e21->face;

    spdlog::debug("Faces:\n  fx: {:?}\n  fy: {:?}", fx, fy);

    // merge v2 => v1
    for (auto h : v2->outgoingHalfEdges()) {
        if (h == e21 || h == e2x || h == e2y) continue;
        h->tail = v1;
        h->twin->tip = v1;
        h->next = v1->halfEdge()->next;
        v1->halfEdge()->next = h;
    }

    // delete edges e2/e2x/e2y
    removeEdge(e), mesh.removeHalfEdge(e12), mesh.removeHalfEdge(e21);
    removeEdge(e2x->edge), mesh.removeHalfEdge(e2x), mesh.removeHalfEdge(ex2);
    removeEdge(e2y->edge), mesh.removeHalfEdge(e2y), mesh.removeHalfEdge(ey2);

    // delete faces fx/fy, vertex v2
    mesh.removeFace(fx), mesh.removeFace(fy);
    removeVertex(v2);

    return v1;
}

MeshSimplifier::MinCostEdgeCollapsingResult MeshSimplifier::collapseMinCostEdge() {
    auto min_cost_edge = cost_edge_map.begin()->second;
    // DONE: finish this function
    if (mesh.isCollapsable(min_cost_edge)) {
        auto optimal_pos = computeOptimalCollapsePosition(min_cost_edge);
        auto vertex = collapseEdge(min_cost_edge);
        checkSanity();
        updateVertexPos(vertex, optimal_pos);
        checkSanity();
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
        checkSanity();
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

void MeshSimplifier::checkSanity() {
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
        sum_of_degrees += v->degree();
    }
    assert(sum_of_degrees == mesh.numEdges() * 2);
}

}  // namespace meshark
