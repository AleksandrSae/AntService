#include <Python.h>

#include <iostream>
#include <string>
#include <unistd.h>

#include "TtyUsbDevice.h"
#include "Stick.h"

static std::shared_ptr<Stick> stick_shared;

struct DLLInitialization
{
	DLLInitialization(){
		int pid;
		pid = getpid();

		std::cerr << "Python module DLL loaded by process PID = <"
				  << pid << "> "
				  << std::endl;

		std::cerr << "Attach the debugger with: $ gdb -pid=" << pid << "\n";

	}
	~DLLInitialization(){
		std::cerr << "DLL native DLL unloaded OK." << std::endl;
	}
};

DLLInitialization dllinit_hook;

// Expose to Python

PyObject* attach(PyObject* self, PyObject* args);
PyObject* init(PyObject* self, PyObject* args);
PyObject* set_callback(PyObject* self, PyObject* args);

static PyMethodDef ModuleFunctions [] =
{
	{"attach", attach, METH_VARARGS,
	  "attach arguments: attach(const char* path_to_device)"},

	{"init", init, METH_VARARGS,
	  "intit_device arguments: init()"},

	{"set_callback", set_callback, METH_VARARGS,
	  "Call python object that has the __call__ method, set_callback arguments: attach(PyObject* pObj*)"},

	// indicate the end of function listing.
	{nullptr, nullptr, 0, nullptr}
};


// Module definition
static struct PyModuleDef ModuleDefinitions {
	PyModuleDef_HEAD_INIT,
	// Module name as string
	"hrm",
	// Module documentation (docstring)
	"A HRM module for python3.",
	-1,
	// Functions exposed to the module
	ModuleFunctions
};


// Module Initialization function
PyMODINIT_FUNC PyInit_hrm(void)
{
	Py_Initialize();
	PyObject* pModule = PyModule_Create(&ModuleDefinitions);
	PyModule_AddObject(pModule, "version", Py_BuildValue("s", "version 0.1-Prototype"));
	return pModule;
}


// Functions of the Python Module

PyObject* attach(PyObject* self, PyObject* args)
{
    char* path_to_device;
	if(!PyArg_ParseTuple(args, "s", &path_to_device))
		return nullptr;

    std::cout << "Attach Ant USB Stick: " << path_to_device << std::endl;

    stick_shared = std::make_shared<Stick>();
    stick_shared->AttachDevice(std::unique_ptr<Device>(new TtyUsbDevice(path_to_device)));

    Py_RETURN_NONE;
}


PyObject* init(PyObject* self, PyObject* args)
{
    if (!stick_shared->Connect())
        return Py_False;

    if (!stick_shared->Reset())
        return Py_False;

    if (!stick_shared->Init())
        return Py_False;

    Py_RETURN_NONE;
}


PyObject* set_callback(PyObject* self, PyObject* args)
{
	PyObject* pObj = nullptr;

	if(!PyArg_ParseTuple(args, "O", &pObj))
		return nullptr;
	if(pObj == nullptr) {
		PyErr_SetString(PyExc_RuntimeError, "Error: invalid None object.");
		return nullptr;
	}
	PyObject* pArgs  = nullptr;
	PyObject* pResult = nullptr;

    while (true) {

        ExtendedMessage msg;

        if (stick_shared->ReadExtendedMsg(msg)) {

            std::stringstream json;

            json << "{" << std::endl
                 << "    \"Device\": " << static_cast<int>(msg.device_number) << "," << std::endl
                 << "    \"Payload\": [";

            for (int indx = 0; indx < 8; ++indx)
                json << "\"0x" << std::hex << (unsigned)msg.payload[indx] << "\",";

            // Replace the latest ','
            json.seekp(-1, std::ios_base::end);
            json << "]" << std::endl
                 << "}";

            pArgs = Py_BuildValue("(s)", json.str().c_str());
            pResult = PyObject_CallObject(pObj, pArgs);

            if (PyBool_Check(pResult) && pResult == Py_False) break;

            if(PyErr_Occurred() != nullptr){
                PyErr_SetString(PyExc_RuntimeError, "Error: Invalid command.");

                return nullptr;
            }
        }
    }

    Py_RETURN_NONE;
};
