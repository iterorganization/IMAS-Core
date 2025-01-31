# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import sys
import datetime
import subprocess
from pathlib import Path

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'IMAS-Core'
copyright = f"{datetime.datetime.now().year}, ITER Organization"
author = 'ITER Organization'
release = '1.0.0'


version = subprocess.check_output(["git", "describe"]).decode().strip()
last_tag = subprocess.check_output(["git", "describe", "--abbrev=0"]).decode().strip()
is_develop = version != last_tag

html_context = {
    "is_develop": is_develop
}

language = "en"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    "sphinx.ext.todo",
    "sphinx.ext.autosectionlabel",
    "sphinx.ext.autodoc",
    "sphinx.ext.intersphinx",
    "sphinx.ext.mathjax",
    "sphinx.ext.napoleon",
    "sphinx_immaterial",
    "sphinx_immaterial.apidoc.python.apigen",
    "breathe",
]

templates_path = ['templates']
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store", "doc_common", "plugins"]

# -- RST snippets to include in every page -----------------------------------
rst_epilog = """\
.. |DD| replace:: `Data Dictionary`_
.. _`Data Dictionary`: https://imas-data-dictionary.readthedocs.io/en/latest/
"""

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output


html_theme = "sphinx_immaterial"
html_theme_options = {
    "repo_url": "https://github.com/iterorganization/IMAS-Core",
    "repo_name": "IMAS-Core",
    "icon": {
        "repo": "fontawesome/brands/github",
    },
    "features": [
        # "navigation.expand",
        # "navigation.tabs",
        "navigation.sections",
        "navigation.instant",
        # "header.autohide",
        "navigation.top",
        # "navigation.tracking",
        # "search.highlight",
        # "search.share",
        # "toc.integrate",
        "toc.follow",
        "toc.sticky",
        # "content.tabs.link",
        "announce.dismiss",
        'globaltoc_collapse',

    ],
    # "toc_title_is_page_title": True,
    # "globaltoc_collapse": True,
    "palette": [
        {
            "media": "(prefers-color-scheme: light)",
            "scheme": "default",
            "primary": "indigo",
            "accent": "green",
            "toggle": {
                "icon": "material/lightbulb-outline",
                "name": "Switch to dark mode",
            },
        },
        {
            "media": "(prefers-color-scheme: dark)",
            "scheme": "slate",
            "primary": "light-blue",
            "accent": "lime",
            "toggle": {
                "icon": "material/lightbulb",
                "name": "Switch to light mode",
            },
        },
    ],
    "version_dropdown": True,
    "version_json": "../versions.js",
}

object_description_options = [
    (".*", dict(include_fields_in_toc=False)),
    (".*parameter", dict(include_in_toc=False)),
]

html_static_path = ["static"]

sphinx_immaterial_generate_extra_admonitions = True
sphinx_immaterial_custom_admonitions = [
    {
        "name": "output",
        "color": (245, 98, 245),
        "icon": "fontawesome/solid/terminal",
    },

]

# Configure Breathe
breathe_projects = {
    "al_lowlevel": "./doxygen_output_al_lowlevel/xml",
    "access_layer_plugin_manager": "./doxygen_output_access_layer_plugin_manager/xml",
    "ascii_backend": "./doxygen_output_ascii_backend/xml",
    "hdf5_backend": "./doxygen_output_hdf5_backend/xml",
    "data_interpolation": "./doxygen_output_data_interpolation/xml",
    "flexbuffers_backend": "./doxygen_output_flexbuffers_backend/xml",
    "memory_backend": "./doxygen_output_memory_backend/xml",
    "mdsplus_backend": "./doxygen_output_mdsplus_backend/xml",
}

breathe_default_project = "al_lowlevel"
