#include <pybind11/pybind11.h>
#include "CoreRoom.h"
#include "Vector3.h"

namespace py = pybind11;

// 파이썬에서 import game_core 로 불러올 수 있게 됩니다.
PYBIND11_MODULE(game_core, m) {
    m.doc() = "Project TOY Simulation Core";

    // Vector3 바인딩 예시
    py::class_<Vector3>(m, "Vector3")
        .def(py::init<float, float, float>())
        .def_readwrite("x", &Vector3::x)
        .def_readwrite("y", &Vector3::y)
        .def_readwrite("z", &Vector3::z);

    // 향후 CoreRoom, Player 등의 바인딩 코드를 여기에 추가합니다.
}
