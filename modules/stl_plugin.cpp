#include "Cora/FFI.hpp"
#include <vector>

typedef std::vector<double> DoubleVector;

CORA_MODULE(stl, m) {
    cora::ffi::class_<DoubleVector>(m, "Vector")
        .def_constructor()
        .def("push_back", [](DoubleVector* v, double d) { v->push_back(d); })
        .def("size", [](DoubleVector* v) { return static_cast<double>(v->size()); })
        .def("at", [](DoubleVector* v, double i) { return v->at(static_cast<size_t>(i)); })
        .def("clear", [](DoubleVector* v) { v->clear(); });
}
