#include <Python.h>
int main() {
    Py_Initialize();
    PyRun_SimpleString("import numpy; print(numpy.__version__)");
    Py_Finalize();
    return 0;
}
