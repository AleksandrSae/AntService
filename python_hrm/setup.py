from distutils.core import setup, Extension

hrm = Extension('hrm',
                language = "c++",
                sources = ['hrm.cpp', '../src/TtyUsbDevice.cpp', '../src/Stick.cpp'],
                extra_compile_args=["-std=c++17"],
                include_dirs = ['../include'])

setup(
    name        = 'hrm',
    version     = '1.0',
    description = 'HRM python module',
    ext_modules = [hrm]
)
