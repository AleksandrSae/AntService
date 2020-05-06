from distutils.core import setup, Extension

hrm = Extension('hrm', sources = ['hrm.cpp'])

setup(
    name        = 'hrm',
    version     = '1.0',
    description = 'HRM python module',
    ext_modules = [hrm]
)
