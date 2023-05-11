# -*- coding: utf-8 -*-

"""Configuration file for the Sphinx documentation builder."""

import os
import re

# -- General ------------------------------------------------------------------

extensions = ["sphinxcontrib.doxylink", "lowdown"]

if os.environ.get("READTHEDOCS"):
    import doxygen

    doxygen.create_cmake_config()
    doxygen.build()

    html_extra_path = ["./api"]

# The suffix of src filenames.
source_suffix = ".rst"

# The master toctree document.
master_doc = "index"

# General information about the project.
project = u"USD Notice Framework"
copyright = u"2023, Walt Disney Animation Studio"

_root = os.path.join(os.path.dirname(__file__), "..", "..")

# Version
with open(os.path.join(_root, "CMakeLists.txt")) as _version_file:
    _version = re.search(
        r"project\(.* VERSION ([\d\\.]+)", _version_file.read(), re.DOTALL
    ).group(1)

version = _version
release = _version

doxylink = {
    "usd-cpp": (
        os.path.join(_root, "doc", "doxygen", "USD.tag"),
        "https://graphics.pixar.com/usd/release/api"
    ),
    "unf-cpp": (
        os.path.join(_root, "build", "doc", "doc", "UNF.tag"),
        os.path.join(_root, "build", "doc", "doc", "doxygen")
    )
}

# -- HTML output --------------------------------------------------------------

html_theme = "sphinx_rtd_theme"

# If True, copy src rst files to output for reference.
html_copy_source = True
