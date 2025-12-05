===========================================
Plugins framework for the IMAS access layer
===========================================

Plugins are C++ software components compiled in separate
libraries from the Access Layer (AL) library, which make use of them. Using a
modified Low Level architecture, we demonstrate that plugins satisfy many
use-cases requirements, providing new features available from all existing HLIs.
We describe the Access Layer plugins architecture and some plugins examples.



Plugins offer the following advantages:

-  Developers write plugins in C++ and compile plugins separately from
   the AL library, which discovers them at runtime, according to some
   Data Dictionary-defined or user-defined plugins activation
   directives. More precisely, the AL Low Level layer loads plugins and
   calls them according to the previous directives. **Plugins provide
   new features with common behavior between High Level
   Interfaces**. There is no need for developing specific High Level
   Interface (HLI) implementation of plugins features, **decreasing
   therefore drastically development time**, **simplifying maintenance**
   and **reducing the risk of potential bugs**.

-  Contributing also to **reduced time development**, code **plugin
   compilation is fast** since plugins have no dependency on HLI
   classes. The compilation time must be compared to the one required
   for specific HLI code whose change may (depending on which source
   file has been modified) trigger the full recompilation of the HLI
   layer.

-  Features provided by a plugin behave the same in all HLIs with same
   exceptions, same outputs, same bugs... **Users experience remains
   unchanged** from one HLI to another.

-  Plugins source files are stored in separate repositories with
   separate development lifecycles. Developers group plugins into
   separate projects, improving **code management, lowering code
   coupling** (plugins are not coupled, they only implement the plugin
   interface described later) between AL components and **making clear
   separation of features** provided by each plugin.

-  Plugins depend on the AL library, not the other way around.
   Therefore, at development time, plugins developers do not need to
   recompile the AL, which lowers time development. Moreover, in
   production, upon new plugins release, no change are required to the
   AL sources or to the AL installed binaries, **preventing AL updates
   procedures and reducing AL deployment maintenance.**

AL plugins provide many features available for all HLIs as for example:

-  **Creating fast C++ post processing** using low-level R/W data access
   functions. Plugins have flexibility to run on-the-fly data
   transformations such as decompression/compression, calibration,
   filtering, ...

-  **Patching values from/to DD leaves** by overriding ``get()``/``put()``
   operations. An example is the unified NBC plugin (which overrides
   the ``get()`` operation) which provides data format backward
   compatibility.

-  **Displaying specific values** of DD leaves to users. Building
   reports for some specific Interface Data Structure (IDS) nodes.
   **Debugging**: developers check expected DD leaves values according
   to some logic implemented in the plugin (for some specific
   data/context). Plugins can display important warnings/infos to users
   if necessary.

-  **Selecting partial data to speed up IDS writing/reading.** Let us
   consider a partial read operation. Since a plugin has the control to
   read data from any DD node, plugin logic allows reading few specific
   nodes of an IDS and skip/ignore (large) nodes not required by the use
   case. We will show later an example, which shows how to read only
   some part of the data from an IDS, reducing IDS loading time.
