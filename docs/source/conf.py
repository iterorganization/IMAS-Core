# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import datetime
import subprocess

# -- Project information -----------------------------------------------------
project = 'IMAS-Core'
copyright = f"{datetime.datetime.now().year}, ITER Organization"
author = 'ITER Organization'
release = '1.0.0'

# Try to get version from git
try:
    version = subprocess.check_output(["git", "describe"], stderr=subprocess.DEVNULL).decode().strip()
    last_tag = subprocess.check_output(["git", "describe", "--abbrev=0"], stderr=subprocess.DEVNULL).decode().strip()
    is_develop = version != last_tag
except (subprocess.CalledProcessError, FileNotFoundError):
    version = "dev"
    last_tag = "dev"
    is_develop = True

html_context = {
    "is_develop": is_develop
}

language = "en"

# -- General configuration ---------------------------------------------------
templates_path = ['_templates']
exclude_patterns = []

# Static files and assets
html_static_path = ["_static"]

# -- RST snippets to include in every page -----------------------------------
rst_epilog = """\
.. |DD| replace:: `IMAS Data Dictionary`_
.. _`IMAS Data Dictionary`: https://imas-data-dictionary.readthedocs.io/en/latest/
.. |IMAS| replace:: Integrated Modelling and Analysis Suite
.. _`IMAS`: https://imas.iter.org/
"""

# -- Options for HTML output -------------------------------------------------
extensions = [
    "sphinx_immaterial",
    "sphinx.ext.autodoc",
    "sphinx.ext.intersphinx",
    "sphinx_tabs.tabs",
    "sphinx_design",
]

html_theme = "sphinx_immaterial"

# HTML static files path (already defined above in Static files and assets section)
# html_static_path = ["_static"]  # Moved to General configuration section

html_theme_options = {
    "repo_url": "https://github.com/iterorganization/IMAS-Core",
    "repo_name": "IMAS-Core",
    "icon": {
        "repo": "fontawesome/brands/github",
    },
    "features": [
        "navigation.sections",
        "navigation.instant",
        "navigation.top",
        "toc.follow",
        "toc.sticky",
        "announce.dismiss",
        'globaltoc_collapse',
    ],
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
}

object_description_options = [
    (".*", dict(include_fields_in_toc=False)),
    (".*parameter", dict(include_in_toc=False)),
]

sphinx_immaterial_generate_extra_admonitions = True
sphinx_immaterial_custom_admonitions = [
    {
        "name": "output",
        "color": (245, 98, 245),
        "icon": "fontawesome/solid/terminal",
    },
    {
        "name": "installation",
        "color": (51, 153, 255),
        "icon": "fontawesome/solid/download",
    },
]
