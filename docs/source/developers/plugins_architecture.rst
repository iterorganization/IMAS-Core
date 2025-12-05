=====================
Plugin implementation
=====================


Modified AL architecture for plugins management and execution
=============================================================

`Figure 2`_ depicts the modified AL architecture for enabling the use of
plugins. It introduces new components:

-  The C wrappers for plugins management

-  The plugins API

-  The plugin interface

-  The C++ plugins

From now on, the Low Level C wrappers call the Plugins API functions.
The LL API is remained unchanged.

.. figure:: ../_static/images/image2.png
   :name: Figure 2

   **Figure 2:** Modified layered AL architecture model for plugins
   orchestration


The C wrappers for plugins management
-------------------------------------

`Figure 3`_ displays the list of new C wrappers for plugins management and
the new plugins API. The latter includes:

-  functions for plugins management (registering and binding to a node)

-  functions which allow plugins to override the behavior of the LL data
   access functions

In the new AL plugin architecture, HLIs call the current C wrappers, which call
the new plugins API, which in turn call the plugins if they are registered and
bound to at least one node of the DD. For example, the
``al_begin_global_action(...)`` function calls the new plugins API
``beginGlobalActionPlugin(...)`` function of the plugins API. The functions
``al_read_data(...) and :code:``al_write_data(...)` delegate to the
``readDataSPlugin(...)`` and ``writeDataPlugin(...)`` plugins API
functions.

.. figure:: ../_static/images/image3.png
   :name: Figure 3

   **Figure 3:** The new C wrappers for plugins management and the new plugins API

Since the plugins API functions located in the LL C++ layer are not
accessible from the HLIs, the new C wrappers depicted in `Figure 3`_ allow
for accessing some plugins API functions from HLIs. These functions are
mainly devoted to plugin registering and activation, features supported
by the plugins API described in the next section:

-  The ``al_register_plugin(...)`` wrapper (resp.
   ``al_unregister_plugin(...)``) delegates to the dedicated C++
   ``registerPlugin(...)`` (resp. ``unregisterPlugin(...)``) function
   from the plugins API.

-  The ``al_bind_plugin(...)`` wrapper (``al_unbind_plugin(...)``)
   delegates to the dedicated C++ :code:`bindPlugin(...) (resp.
   ``unbindPlugin(...)``) function from the plugins API.


The Plugins API and the low level holder plugin class
-----------------------------------------------------

`Figure 4`_ depicts the ``LLplugin`` plugin holder class.

.. figure:: ../_static/images/image4.png
   :name: Figure 4

   **Figure 4:** The plugin holder class (``LLplugin``)


Plugin registration
~~~~~~~~~~~~~~~~~~~

Among the plugins API functions (`Figure 3`_), we find
``registerPlugin(plugin_name)`` (``unregisterPlugin(plugin_name)``)
which creates (resp. destroys) a plugin instance from a C++ class located in a
shared library (.so). The ``registerPlugin(...)`` function creates a
``LLplugin`` object for holding the plugin instance using the
``al_plugin`` pointer attribute defined in the ``LLplugin`` class (see
`Figure 4`_). The ``LLplugin`` object is then stored in a static map (named
````LLplugin``sStore``) of the ``LLplugin`` class. This map allows the
plugin framework to retrieve the plugin later using the name of the plugin as a
key (``plugin_name`` function argument).

.. _`Plugin activation`:

Plugin activation
~~~~~~~~~~~~~~~~~

Once a plugin is registered, it is available from the plugins framework as long
as the AL process is running and as long as users have not called the
``unregister_plugin(...)`` C wrapper. However, during put/get operations,
the AL plugin framework will ignore a registered plugin not bound to any DD
node. To activate a plugin, an HLI code bounds the plugin to at least one
particular IDS node using the ``al_bind_plugin(...)`` wrapper. This function
updates the ``boundPlugins`` static map of the ``LLplugin`` class, adding the name
of the plugin to a list which is mapped to the identifier of the DD node (the
identifier is the path to the node). To disable a plugin, HLIs use the
``al_unbind_plugin(...)`` function which removes the plugin name from the
list hold by the ``boundPlugins`` map.

Unregistering a plugin using ``al_unregister_plugin(plugin_name)`` will
remove the ``LLplugin`` object from the LL store ``LLpluginsStore`` and
destroy the underlying plugin instance.


Calling plugins from the Low Level
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

All plugins API functions in `Figure 3`_ (except functions for plugins
registering/activation) call plugins operations from the plugin
interface described in the next section during LL operations. For
example, the ``beginGlobalActionPlugin(...)`` function calls the
``begin_global_action(...)`` function of the plugin interface. More details
are provided in sequence diagrams presented in section :ref:`get() sequence
diagram` and :ref:`put() sequence diagram`.


The Access Layer plugin interface
---------------------------------

When creating a plugin, the plugin class has to inherit from the
``access_layer_plugin`` interface and implement each (pure virtual)
operation declared in this interface (see ``access_layer_plugin.h``).

The ``begin_global_action(...)`` (resp. ``begin_slice_action(...)``) functions are
plugins operations called when the HLI calls ``al_begin_global_action(...)``
(resp. ``al_begin_slice_action(...)``). These functions allow developers to
initialize the plugin before iteration of the IDS nodes performed by the
HLI. For example, during a slice operation using ``get_slice(...)`` or
``put_slice(...)``, the ``begin_slice_action(...)`` allows to store in the plugin
object the time of the slice and the interpolation method to perform the
slice.

The node_operation(``const std::string &path``) function returns the type of
operation applied to the data node (located at the value given by the
``path`` argument) by the plugin. This function must return one of the
following value: ``plugin::OPERATION::GET_ONLY``,
``plugin::OPERATION::PUT_ONLY`` or ``plugin::OPERATION::PUT_AND_GET``. The
``node_operation(...)`` function, called by the plugins framework, has several
purposes:

-  It allows the plugins framework to sort the plugins that are contributing to
   the ``get``/``get_slice`` operation, ``put``/``put_slice`` operation, or
   both. Only plugins returning ``plugin::OPERATION::GET_ONLY`` or
   ``plugin::OPERATION::PUT_AND_GET`` are added to the
   ``ids_properties/plugins/get_operation`` data structure during a
   ``get``/``get_slice`` operation. Moreover, only plugins returning
   ``plugin::OPERATION::PUT_ONLY`` or ``plugin::OPERATION::PUT_AND_GET`` are
   added to the ``ids_properties/plugins/put_operation`` data structure during a
   ``put``/``put_slice`` operation.

-  The plugins framework will call the plugin operation ``read_data(...)`` for
   a given IDS node path only if the ``node_operation(...)`` function returns
   ``plugin::OPERATION::GET_ONLY`` or ``plugin::OPERATION::PUT_AND_GET`` for
   this path.

-  The plugins framework will call the plugin operation ``write_data(...)``
   for a given IDS node path only if the ``node_operation(...)`` function
   returns ``plugin::OPERATION::PUT_ONLY`` or ``plugin::OPERATION::PUT_AND_GET``
   for this path.

The ``read_data(...)`` and (resp. ``write_data(...)``) allows a plugin to
intercept a data pointer coming from/going to the backend to read or modify it
on the fly. This documentation provides some plugins examples to illustrate the
use of these functions.


Calling low level data access API functions from plugin code
------------------------------------------------------------

New C wrappers allow plugins to call LL data access operations (the
latter should not be called by plugins code to avoid infinite
recursion). These functions have the same name that the existing
counterpart AL wrappers with prefix ``al`` replaced by ``al_plugin``
(`Figure 6`_).

.. code-block:: C++
   :caption: **Figure 6:** new C wrappers for calling LL data access operations from plugins
   :name: Figure 6

   void al_plugin_begin_global_action(const std::string &plugin_name, int pulseCtx, const char* dataobjectname, int mode, int opCtx);
   void al_plugin_slice_action(const std::string &plugin_name, int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx);
   void al_plugin_arraystruct_action(const std::string &plugin_name, int ctx, int *actxID, const char* fieldPath, const char* timeBasePath, int *arraySize);
   void al_plugin_read_data(const std::string &plugin_name, int ctx, const char* fieldPath, const char* timeBasePath, void **data, int datatype, int dim, int *size);
   void al_plugin_write_data(const std::string &plugin_name, int ctxID, const char *field, const char *timebase, void *data, int datatype, int dim, int *size);


Plugins orchestration
=====================

A registered and activated plugin (see :ref:`Plugin activation`) will be called
by the LL whenever a ``get()``/``get_slice()`` or ``put()``/``put_slice()``
operation is performed by an HLI. The next sections describe the dynamic of
plugins calls during a ``get()``/``get_slice()`` or a ``put()``/``put_slice()``
operation through sequence diagrams.

.. _`get() sequence diagram`:

``get()`` sequence diagram
--------------------------

.. figure:: ../_static/images/image5.png
   :name: Figure 7

   **Figure 7:** ``get()`` operation sequence diagram

`Figure 7`_ depicts an example of the ``get()`` operations sequence of a
``camera_ir`` IDS using the ``camera_ir`` plugin developed at WEST. The
client code uses HLI operations to read data of a ``camera_ir`` IDS. In
the first two calls, the client registers the plugin and binds it to the
node identified by the path ``camera_ir/frame/surface_temperature``, then
the AL API ``get()`` operation is called on the ``camera_ir`` IDS object (the
preliminary operation for creating the IDS object is not shown in the
figure). The ``al_begin_global_action(...)`` function calls first the
``beginGlobalActionPlugin(...)`` function of the plugins API (whatever the
node to which the plugin is bound) which in turn calls the
``begin_arraystruct_action(...)`` of the plugin interface. Similarly, each
call to ``al_begin_arraystruct_action(...)`` generates first a call to
``beginArraystructActionPlugin(...)``, followed by a call to the
``begin_arraystruct_action(...)`` function of the plugin interface.

If several plugins are bound to the same IDS node, the plugin
framework will iterate over each plugin and call the appropriate plugin
interface function for each plugin sequentially. The iteration order
follows the order in which plugins bindings have been performed by the
HLI client using ``al_bind_plugin(...)``.

.. _`put() sequence diagram`:

``put()`` sequence diagram
--------------------------

The ``put()`` sequence diagram (`Figure 8`_) is quite similar to the ``get()``
sequence diagram. The example uses the ``camera_ir_write`` plugin
described later in this document.

.. figure:: ../_static/images/image6.png
   :name: Figure 8

   **Figure 8:** ``put()`` operation sequence diagram

Note that the wrapper ``al_begin_arraystruct_action(...)`` is called in this
modified AL plugin architecture even if the corresponding array of
structure (AOS) has a 0-shape (this is not true for the previous AL
architecture where HLIs were not calling ``al_begin_arraystruct_action(...)``
if the AOS was found to be empty). The reason is to make plugins able to
write/update the size of AOSs.


Access Layer plugin provenance
==============================

Recent versions of the Data Dictionary store information concerning plugin
provenance.

To take into account these change, we have defined new plugins requirements with
the ``provenance_plugin_feature`` and the ``readback_plugin_feature`` interfaces
(`Figure 8`_). Moreover, in order to extend the ability of the LL to accept any
type of plugins, we have introduced a new access_layer_base_plugin interface.
The latter is more general and does not hold the read/write data access and
*readback* requirements.

It may be necessary to define *readback* plugins that can read data
written by other plugins. For instance, the ``camera_ir_write`` plugin
(used currently on WEST) writes compressed data of the ``camera_ir`` IDS
to the backend. In order to read these data during a ``get()`` or
``get_slice()`` operation, the ``camera_ir_write`` plugin implements the
``readback_plugin_feature`` interface, which provides information to the
Access Layer about a *readback* plugin capable of reading and
uncompressing the data. For this example, the ``camera_ir`` readback
plugin is able to read and decompress data stored by the
``camera_ir_write`` plugin, providing uncompressed data to the HLI. The
name of the *readback* plugin is obtained using the
``getReadbackName(const std::string &path, int *index)`` function of the
``readback_plugin_feature`` interface for a given IDS node path. It is worth
noting that several plugins can be applied to the same IDS node path,
and the ``index`` parameter indicates which *readback* plugin should be
used. For instance, if two *readback* plugins are defined for the same
node path, a value of 1 for the ``index`` parameter indicates that the
plugin should be applied after the plugin defined at ``index=0``.

During a ``put()``/``put_slice()`` operation, the **readback**
informations specified by the **readback_plugin_feature** interface are
stored in the backend. These data are read during a ``get()``/``get_slice()``
operation and used to bind and execute the *readback* plugins.

.. figure:: ../_static/images/image7.png
   :name: Figure 9

   **Figure 9:** The ``provenance_plugin_feature``, ``readback_plugin_feature``
   and ``access_layer_base_plugin`` interfaces

