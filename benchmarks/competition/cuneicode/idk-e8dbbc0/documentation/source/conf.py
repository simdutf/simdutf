# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys
import subprocess
import typing
# sys.path.insert(0, os.path.abspath('.'))

# -- Project information -----------------------------------------------------

project: str = 'ztd.idk'
copyright: str = "2022, ThePhD & Shepherd's Oasis, LLC"
author: str = "ThePhD & Shepherd's Oasis, LLC"

# The full version, including alpha/beta/rc tags
#
release: str = '0.0.0'

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
#
extensions: typing.List[str] = ['breathe', 'sphinx.ext.autosectionlabel', 'myst_parser']

# Add any paths that contain templates here, relative to this directory.
templates_path: typing.List[str] = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
#
exclude_patterns: typing.List[str] = []

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme: str = 'sphinx_rtd_theme'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
#
html_static_path: typing.List[str] = ["_static"]

html_js_files: typing.List[str] = ['inline_removal.js']

# Add any extra paths that contain custom files (such as robots.txt or
# .htaccess) here, relative to this directory. These files are copied
# directly to the root of the documentation.
#
html_extra_path: typing.List[str] = []

# Text that is pre-pended to every built file. Useful for global substitution patterns.
rst_prolog: str = """
.. |ub| replace:: ☢️☢️Undefined Behavior☢️☢️

.. |specializations_okay_different_types| replace:: User Specializations: ✔️ Okay! You can add other types to this classification by specializing the class template. Your specialization must have a type definition named ``type`` (as in, ``using type = ...;`` or `typedef ... type;``) inside of the class specialization that is ``public:``\ ly accessible. Note that specializing any type not explicitly marked with this notice is |ub|.

.. |specializations_okay_true_false_type| replace:: User Specializations: ✔️ Okay! You can add other types to this classification by specializing the class template to a definition that derives from ``std::true_type``, or turn it off explicitly by having a definition that derives from ``std::false_type``. Note that specializing any type not explicitly marked with this notice is |ub|.

.. |reserved_name| replace:: ⚠️ Names with double underscores, and within the ``__detail`` and ``__impl`` namespaces are reserved for the implementation. Referencing this entity directly is bad, and the name/functionality can be changed at any point in the future. Relying on anything not guaranteed by the documentation is |ub|.

.. |unfinished_warning| replace:: 🔨 This isn't finished yet! Come check back by the next major or minor version update.

.. |generic_type| replace:: ``ztd_generic_type`` is a name used as a placeholder. When it appears as a name (or within a name) or a type, it can be substituted out for another type name!

.. |benchmark_warning| replace:: 🤔 This is not an exhaustive benchmark suite, nor is it representative of all machines or architectures. All numbers should be taken in the context of the reported environment and standard library below, as well as any additional caveats listed.
"""

# C++ Index Configuration
#
cpp_index_common_prefix: typing.List[str] = [
    'ztd::idk::__impl::', 'ztd::__impl::', 'ztd::__idk_detail::', 'ztd::'
]

# Breathe Configuration
#
breathe_projects: typing.Dict[str, object] = {}
breathe_default_project: str = "ztd.idk"

# autosectionlabel Configuration
#
autosectionlabel_prefix_document: bool = True


# ReadTheDocs Build Help
#
def run_cmake_doxygen():
	"""Run the cmake command to get the doxygen sources"""

	# Make sure the directory exists
	cmake_dir = os.path.join(os.getcwd(), '_build/cmake-build')
	xml_dir = os.path.join(cmake_dir, 'documentation/doxygen/xml')
	os.makedirs(cmake_dir, exist_ok=True)
	os.makedirs(xml_dir, exist_ok=True)
	print("[ztd.idk/documentation/conf.py] CMake Directory: %s" % cmake_dir)
	print("[ztd.idk/documentation/conf.py] XML Directory: %s" % xml_dir)

	try:
		retcode = subprocess.call(
		    "cmake -DZTD_IDK_READTHEDOCS:BOOL=TRUE -DZTD_IDK_DOCUMENTATION:BOOL=TRUE -DZTD_IDK_DOCUMENTATION_NO_SPHINX:BOOL=TRUE ../../../..",
		    shell=True,
		    cwd=cmake_dir)
	except OSError as e:
		sys.stderr.write("cmake generation execution failed: %s\n" % e)
		return

	try:
		retcode = subprocess.call("cmake --build . --target ztd.idk.documentation.doxygen",
		                          shell=True,
		                          cwd=cmake_dir)
	except OSError as e:
		sys.stderr.write("cmake generation execution failed: %s\n" % e)
		return

	breathe_projects["ztd.idk"] = xml_dir


def generate_doxygen_xml(app):
	"""Run the doxygen make commands if we're on the ReadTheDocs server"""

	read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'
	if read_the_docs_build:
		print(
		    "[ztd.idk/documentation/conf.py] Detected READTHEDOCS environment variable: running cmake from conf.py"
		)
		run_cmake_doxygen()


def setup(app):
	"""Sphinx / ReadTheDocs hook for when the build system is initialized."""
	# Add hook for building doxygen xml when needed
	app.connect("builder-inited", generate_doxygen_xml)
