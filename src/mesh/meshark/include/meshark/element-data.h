//
// Created by creeper on 7/23/24.
//

#ifndef MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_ELEMENT_DATA_H_
#define MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_ELEMENT_DATA_H_

#include <meshark/half-edge-mesh.h>
#include <vector>

namespace meshark {

template <typename Element, typename Type> struct ElementData {
    using T = std::conditional_t<std::is_same_v<Type, bool>, std::byte, Type>;
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

}  // namespace meshark
#endif  // MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_ELEMENT_DATA_H_
