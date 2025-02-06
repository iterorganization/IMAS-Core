# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR      = source
BUILDDIR       = build
DOXYGEN       ?= doxygen
DOXYFILE      ?= Doxyfile
DOXYGEN_DIR  = doxygen/imas_core/
SPHINX_DOXYGEN_DIR  = source/imas_core/

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

# Target to run Doxygen and generate XML files
executedoxygen:
	@mkdir -p "$(DOXYGEN_DIR)"
	@doxygen $(DOXYFILE)
	@cp -rf "$(DOXYGEN)" "$(SOURCEDIR)"/static

doxysphinx: executedoxygen
	@doxysphinx -v INFO build "$(DOXYGEN_DIR)" "$(BUILDDIR)" "$(DOXYFILE)"
	@cp -rf "$(DOXYGEN_DIR)" "$(SPHINX_DOXYGEN_DIR)"

# Target to build Sphinx documentation after running Doxygen
html: doxysphinx
	@$(SPHINXBUILD) -M html "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	

# Clean up generated files
clean:
	@rm -rf "$(BUILDDIR)"
	@rm -rf "$(DOXYGEN_DIR)"
	@rm -rf "$(SPHINX_DOXYGEN_DIR)" 
	@rm -rf "$(SOURCEDIR)"/static/$(DOXYGEN)

