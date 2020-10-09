/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <Python.h>

#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))

PyMethodDef module_methods[] =
{
    { nullptr, nullptr, 0, nullptr }
};

struct module_state
{
    PyObject* error;
};

int module_traverse(PyObject* m, visitproc visit, void* arg)
{
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

int module_clear(PyObject* m)
{
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

struct PyModuleDef moduledef =
{
    PyModuleDef_HEAD_INIT,
    "_orcus_ml_formula_correction",
    nullptr,
    sizeof(struct module_state),
    module_methods,
    nullptr,
    module_traverse,
    module_clear,
    nullptr
};

extern "C" {

PyObject* PyInit__orcus_ml_formula_correction()
{
    PyObject* m = PyModule_Create(&moduledef);

    return m;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
