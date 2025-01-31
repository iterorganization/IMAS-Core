# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = source
BUILDDIR      = _build
DOXYGEN       ?= doxygen
DOXYFILE      ?= Doxyfile
DOXYFILES = Doxyfile_access_layer_plugin_manager \
			Doxyfile_al_lowlevel \
			Doxyfile_ascii_backend \
			Doxyfile_main \
			Doxyfile_data_interpolation \
			Doxyfile_flexbuffers_backend \
			Doxyfile_hdf5_backend \
			Doxyfile_mdsplus_backend \
			Doxyfile_memory_backend \
			Doxyfile_hdf5_backend

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

# Target to run Doxygen and generate XML files
doxygen:
	@for doxyfile in $(DOXYFILES); do \
		$(DOXYGEN) $$doxyfile; \
	done

# Target to build Sphinx documentation after running Doxygen
html: doxygen
	@$(SPHINXBUILD) -M html "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

# Clean up generated files
clean:
	@for doxyfile in $(DOXYFILES); do \
		@rm -rf $(SOURCEDIR)/$$doxyfile; \
	done
	@rm -rf $(BUILDDIR)