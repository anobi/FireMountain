// DISCLAIMER: A vibecoded piece of garbage because I don't care about this too much right now

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "python_embed.h"
#include <stdexcept>
#include <utility>

namespace {
    // RAII to initialize Python once, without touching your main()
    struct PythonRuntime {
        PythonRuntime() {
            PyConfig config;
            PyConfig_InitPythonConfig(&config);
            config.isolated = 0;
            config.use_environment = 1;  // allow env

            Py_InitializeFromConfig(&config);
            PyConfig_Clear(&config);
            // If you’ll call from multiple C++ threads, the GIL API below is enough on 3.8+.
            // (PyEval_InitThreads is deprecated/removed on modern Python.)
        }
        ~PythonRuntime() {
            if (Py_IsInitialized()) {
                Py_Finalize(); // optional; you can omit if you prefer not to finalize at exit
            }
        }
    };

    // lazy singleton so we don’t rely on global init order
    PythonRuntime& runtime() {
        static PythonRuntime r;
        return r;
    }

    // Enter/leave the GIL around any Python C-API use
    struct GilGuard {
        PyGILState_STATE s;
        GilGuard() : s(PyGILState_Ensure()) {}
        ~GilGuard() { PyGILState_Release(s); }
    };
} // namespace

std::unordered_map<std::string, std::vector<std::string>>
py_build_shaders(const std::string& shader_folder,
                 const std::string& module_dir,
                 const std::string& module_name,
                 const std::string& func_name)
{
    (void)runtime();     // ensure interpreter is up
    GilGuard gil;        // hold GIL during the call

    // Make sure we can import from module_dir
    PyObject* sys_path = PySys_GetObject("path"); // borrowed ref
    PyObject* pDir = PyUnicode_DecodeFSDefault(module_dir.c_str());
    if (!pDir) { throw std::runtime_error("Failed to create module_dir unicode"); }
    if (PyList_Insert(sys_path, 0, pDir) != 0) {
        Py_DECREF(pDir);
        PyErr_Print();
        throw std::runtime_error("Failed to insert module_dir into sys.path");
    }
    Py_DECREF(pDir);

    // Import module
    PyObject* pName = PyUnicode_FromString(module_name.c_str());
    PyObject* pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    if (!pModule) {
        PyErr_Print();
        throw std::runtime_error("Import failed for module: " + module_name);
    }

    // Get function
    PyObject* pFunc = PyObject_GetAttrString(pModule, func_name.c_str());
    if (!pFunc || !PyCallable_Check(pFunc)) {
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
        PyErr_Print();
        throw std::runtime_error("Function not found/callable: " + func_name);
    }

    // Call func(folder)
    //PyObject* pArgs = PyTuple_New(1);
    //PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(shader_folder.c_str())); // steals ref
    PyObject* pRet = PyObject_CallObject(pFunc, nullptr);
    //Py_DECREF(pArgs);
    Py_DECREF(pFunc);
    Py_DECREF(pModule);

    if (!pRet) {
        PyErr_Print();
        throw std::runtime_error("Python call failed");
    }

    // Convert dict[str, list[str]] -> unordered_map<string, vector<string>>
    std::unordered_map<std::string, std::vector<std::string>> out;

    if (!PyDict_Check(pRet)) {
        Py_DECREF(pRet);
        throw std::runtime_error("Unexpected return type (expected dict)");
    }

    PyObject *key, *val;
    Py_ssize_t pos = 0;
    while (PyDict_Next(pRet, &pos, &key, &val)) {
        const char* k = PyUnicode_AsUTF8(key);
        if (!k || !PyList_Check(val)) continue;
        std::vector<std::string> stages;
        const Py_ssize_t n = PyList_Size(val);
        stages.reserve(static_cast<size_t>(n));
        for (Py_ssize_t i = 0; i < n; ++i) {
            stages.emplace_back(PyUnicode_AsUTF8(PyList_GetItem(val, i)));
        }
        out.emplace(k, std::move(stages));
    }

    Py_DECREF(pRet);
    return out;
}