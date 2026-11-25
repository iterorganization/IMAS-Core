Error handling
==============

All Access Layer functions use the status codes below. The status code 
indicates where the error occurred, while the accompanying message describes 
the specific problem. Inspect the full status message for details.

.. list-table:: Access Layer status codes
   :header-rows: 1
   :widths: 10 25 65

   * - Code
     - Constant
     - Meaning
   * - ``0``
     - 
     - Success
   * - ``-1``
     - ``UNKNOWN_ERR``
     - Unexpected or unclassified error
   * - ``-2``
     - ``CONTEXT_ERR``
     - Invalid Access Layer context or operation
   * - ``-3``
     - ``BACKEND_ERR``
     - Error reported by the selected storage backend
   * - ``-4``
     - ``LOWLEVEL_ERR``
     - Error in IMAS-Core processing

The C API provides the numeric status code and a detailed error message. 
Other language interfaces may return this status or report it as an
exception.