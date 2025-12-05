Data entry URIs
===============

Data entry URIs specify where and how IMAS data is stored (or should be stored
to). When you load or store IMAS data, you
need to provide a data entry URI.

This page documents the URI structure and the options that are supported.
For the complete official IMAS URI scheme specification, see the
`IMAS Access-Layer URI Scheme Documentation <https://imas-data-dictionary.readthedocs.io/en/latest/IMAS-URI-scheme.html>`_.


Data entry URI structure
------------------------

The general structure of an IMAS URI is the following, with optional elements
indicated with square brackets:


.. code-block:: text

    imas:[//host/]backend?query

Let's break down each of the components:

1. ``imas:`` this part indicates that this is an IMAS URI
2. ``host`` when the data is located at another machine, you use this
   section to indicate the address of that machine. See the UDA backend section
   in :doc:`backends_guide` for further details.
3. ``backend`` select the Access Layer backend. See :doc:`backends_guide` for the
   options.
4. ``query`` the query consists of ``key=value`` pairs, separated by a
   semicolon ``;``. See :doc:`backends_guide` for further details.

.. 
   Commenting this out, as no backend currently supports URI fragments

   5. ``fragment`` In order to identify a subset from a given data-entry a
      ``fragment`` can be added to the URI. Such ``fragment``, which starts with a
      hash ``#``, is optional and allows to identify a specific IDS, or a part of
      an IDS. See :ref:`URI fragment` for further details.
