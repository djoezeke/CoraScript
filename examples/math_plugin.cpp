#include "Cora/FFI.hpp"
#include <cmath>
#include <iostream>

double add(double a, double b) {
    return a + b;
}

double power(double base, double exp) {
    return std::pow(base, exp);
}

void greet(std::string name) {
    std::cout << "Hello from C++, " << name << "!" << std::endl;
}

CORA_MODULE(math, m) {
    m.def("add", &add);
    m.def("pow", &power);
    m.def("greet", &greet);

    // Example of calling a Cora function from C++
    // This assumes Cora code has defined a function called 'on_plugin_load'
    // auto on_load = cora::ffi::get_global(scope, "on_plugin_load");
    // if (on_load) {
    //     on_load("math_plugin loaded successfully");
    // }
}
