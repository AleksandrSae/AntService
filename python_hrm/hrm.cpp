#include <iostream>
#include <string>
#include <functional>
#include <iomanip>

#include <Python.h>

#include <unistd.h>

#include "TtyUsbDevice.h"
#include "Stick.h"

static std::shared_ptr<Stick> stick_shared;

struct DLLInitialization
{
	DLLInitialization(){
		int pid;
		pid = getpid();

		std::cerr << " [TRACE] Python module DLL loaded by process PID = <"
				  << pid << "> "
				  << std::endl;

		std::cerr << " [TRACE] Attach the debugger with: $ gdb -pid=" << pid << "\n";

	}
	~DLLInitialization(){
		std::cerr << " [TRACE] DLL native DLL unloaded OK." << std::endl;
	}
};

DLLInitialization dllinit_hook;

// Expose to Python

PyObject* attach(PyObject* self, PyObject* args);
PyObject* init(PyObject* self, PyObject* args);
PyObject* hrm_callback(PyObject* self, PyObject* args);

static PyMethodDef ModuleFunctions [] =
{
	{"attach", attach, METH_VARARGS,
	  "attach arguments: attach(const char* path_to_device)"},

	{"init", init, METH_VARARGS,
	  "intit_device arguments: init()"},

	{"hrm_callback", hrm_callback, METH_VARARGS,
	  "Call python object that has the __call__ method, acattach arguments: attach(PyObject* pObj*)"},

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


// =========  Functions of the Python Module ======== //

PyObject* attach(PyObject* self, PyObject* args)
{
    char* path_to_device;
	if(!PyArg_ParseTuple(args, "s", &path_to_device))
		return nullptr;

    std::cout << "Attach Ant USB Stick: " << path_to_device << std::endl;

    stick_shared = std::make_shared<Stick>();
    stick_shared->AttachDevice(std::unique_ptr<Device>(new TtyUsbDevice()));

    Py_RETURN_NONE;
}


PyObject* init(PyObject* self, PyObject* args)
{
    stick_shared->Connect();
    stick_shared->Reset();
    stick_shared->QueryInfo();
    stick_shared->Init();

    Py_RETURN_NONE;
}


PyObject* hrm_callback(PyObject* self, PyObject* args)
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

    double x = 100.;
    double y = 100.;

    while (y > 0) {
        uint8_t channel_number;
        std::vector<uint8_t> payload;
        uint16_t device_number;
        uint8_t device_type;
        uint8_t trans_type;

        if (stick_shared->ReadExtendedMsg(channel_number, payload, device_number, device_type, trans_type)) {

        //std::cout << "Channel:" << std::dec << (unsigned) channel_number
        //          << " Payload:\"" << MessageDump(payload)
        //          << "\" Device number:" << std::dec << (unsigned) device_number
        //          << " Device type:0x" << std::hex << (unsigned) device_type
        //          << " Transfer type:0x" << std::hex << (unsigned) trans_type
        //          << std::endl;

            //pArgs  = Py_BuildValue("(d)", x);
            pArgs = Py_BuildValue("iiiiiiiii", (int)device_number, (int)payload[0], (int)payload[1],
                                  (int)payload[2], (int)payload[3], (int)payload[4],
                                  (int)payload[5], (int)payload[6], (int)payload[7]);
            pResult = PyEval_CallObject(pObj, pArgs);
            y = PyFloat_AsDouble(pResult);
            if(PyErr_Occurred() != nullptr){
                PyErr_SetString(PyExc_RuntimeError, "Error: Invalid command.");

                return nullptr;
            }
        }
    }
    Py_RETURN_NONE;
};
