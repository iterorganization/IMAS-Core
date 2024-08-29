Use Interface Data Structures
=============================

The Interface Data Structures (IDSs) are the main way to interact with IMAS
data. An IDS is a tree-like structure with one root element (the IDS) and
several branches with data at the leave nodes.

Many types of IDSs exist: check out the documentation of the |DD| for a complete
overview.


Creating IDSs
-------------

IDSs can be created in multiple ways:

1. :ref:`Load an IDS from disk <Loading IMAS data>`
2. :ref:`Create an empty IDS`
3. :ref:`Create a copy of an IDS`


Create an empty IDS
'''''''''''''''''''

You can create an empty instance of an IDS by |create_ids_text| creates an empty
``core_profiles`` IDS. This initializes all items in the IDS to their
:ref:`default values`.

.. literalinclude:: code_samples/ids_create
    :caption: |lang| example: create an empty IDS


Create a copy of an IDS
'''''''''''''''''''''''

You can create a copy of another IDS |copy_ids|.

.. literalinclude:: code_samples/ids_copy
    :caption: |lang| example: create a copy of an IDS


Deallocate an IDS
'''''''''''''''''

If you no longer need an IDS, you can deallocate it so it releases the (memory)
resources in use by the data. |deallocate_ids_text|

.. literalinclude:: code_samples/ids_deallocate
    :caption: |lang| example: deallocate an IDS


Mandatory and recommended IDS attributes
----------------------------------------

Some attributes in an IDS are mandatory or recommended to always fill. Below
list provides a short overview:

.. todo::

    Link to DD documentation

1. ``ids_properties/homogeneous_time`` `[mandatory]`: see :ref:`Time coordinates
   and time handling`.
2. ``ids_properties/comment`` `[recommended]`: a comment describing the content
   of this IDS.
3. ``ids_properties/provider`` `[recommended]`: name of the person in charge of
   producing this data.
4. ``ids_properties/creation_date`` `[recommended]`: date at which this data has
   been produced, recommended to use the `ISO 8601
   <https://en.wikipedia.org/wiki/ISO_8601>`_ ``YYYY-MM-DD`` format.

.. note::

    ``ids_properties/version_put`` is filled by the access layer when you
    :ref:`put an IDS <Store IMAS data>`.


Understanding the IDS structure
-------------------------------

An IDS is a `tree structure
<https://en.wikipedia.org/wiki/Tree_(data_structure)>`_. You can think of it
similar as a directory structure with files: the IDS is the root "directory",
and inside it you can find "subdirectories" and "files" with data.

We will use the general Computer Science terminology for tree structures and
call these "files" and "directories" of our IDSs `nodes`. IDSs can have a
limited number of different types of nodes:

1.  :ref:`Structure`: think of these as the directories of your file
    system. Structures contain one or more child nodes (files and
    subdirectories). Child nodes can be of any node type again.

2.  :ref:`Array of structures`: this is an array of structures (see
    previous point).

3.  :ref:`Data`: this is a data element. Like files on your file system
    these nodes contain the actual data stored in the IDS.


Structure
'''''''''

Structure nodes in an IDS are a container for other nodes. In |lang| they are
implemented as a |structures_type|. You can address child nodes as
|structures_child_attribute|, see the code sample below.

.. literalinclude:: code_samples/ids_structures_node
    :caption: |lang| example: address the child node of an IDS structure node


Array of structures
'''''''''''''''''''

Array of structure nodes in an IDS are one-dimensional arrays, containing structure
nodes. In |lang| they are implemented as a |aos_type|. The default value (for
example, when creating a new IDS) for these nodes is |aos_default|.

.. literalinclude:: code_samples/ids_array_of_structures_node
    :caption: |lang| example: address the child node of an IDS arrays of structure node


Resizing an array of structures
```````````````````````````````

You can resize an array of structures with |aos_resize_meth|. After calling
this, the array of structures will have ``n`` elements.

.. caution::

    Resizing an array of structures with |aos_resize_meth| will clear all data
    inside the array of structure! Use |aos_resize_keep_meth| to keep existing
    data.

.. literalinclude:: code_samples/ids_array_of_structures_resize
    :caption: |lang| example: resizing an array of structures


Data
''''

Data nodes in an IDS contain numerical or textual data. The data type and
dimensions of a node are defined in the |DD|.

.. literalinclude:: code_samples/ids_data_node
    :caption: |lang| example: get the data contained in a data node of an IDS


Data types
``````````

The following data types exist:

- Textual data (|str_type|)
- Whole numbers (|int_type|)
- Floating point numbers (|double_type|)
- Complex floating point numbers (|complex_type|)

Data nodes can be 0-dimensional, which means that the node accepts a single
value of the specified type. Multi-dimensional data nodes also exist:

- Textual data: at most 1 dimension (|str_1d_type|)
- Whole numbers: 1-3 dimensions (|int_nd_type|)
- Floating point numbers: 1-6 dimensions (|double_nd_type|)
- Complex floating point numbers: 1-6 dimensions (|complex_nd_type|)


.. _Empty fields:

Default values
``````````````

The default values for data fields (for example when creating an empty IDS) are
indicated in the following table. |isFieldValid|

.. csv-table::
    :header-rows: 1
    :stub-columns: 1
    
    , 0D, 1+ dimension
    "Textual

    data", |str_default|, |str_1D_default|
    "Whole

    numbers", |int_default|, |ND_default|
    "Floating

    point

    numbers", |double_default|, |ND_default|
    "Complex

    numbers", |complex_default|, |ND_default|


Time coordinates and time handling
''''''''''''''''''''''''''''''''''

Some quantities (and array of structures) are time dependent. In the |DD|
documentation this is indicated by a coordinate that refers to a time quantity.

This time-dependent coordinate is treated specially in the access layer, and it
depends on the value of ``ids_properties/homogeneous_time``. There are three
valid values for this property:

.. todo::

    Add links to DD.

1. |tm_heterogeneous| (=0): time-dependent quantities in the IDS may have
   different time coordinates. The time coordinates are stored as indicated by
   the path in the documentation. This is known as `heterogeneous time`.
2. |tm_homogeneous| (=1): All time-dependent quantities in this IDS use the same
   time coordinate. This is known as `homogeneous time`. This time coordinate is
   located in the root of the IDS, for example ``core_profiles/time``. The paths
   time paths indicated in the documentation are unused in this case.
3. |tm_independent| (=2): The IDS stores no time-dependent data.



IDS validation
--------------

The IDSs you fill should be consistent. To help you in validating that, the
Access Layer provides a validation method (|ids_validate|) that executes the
following checks.

.. contents:: Validation checks
    :local:
    :depth: 1

If you call this method and your IDS fails validation, the Access Layer
|validate_error| explaining the problem. See the following example:

.. literalinclude:: code_samples/ids_validate
    :caption: |lang| example: call IDS validation

The Access Layer automatically validates an IDS every time you do a
`put` or `put_slice`. To disable this feature, you must set the environment
variable ``IMAS_AL_DISABLE_VALIDATE`` to ``1``.

.. seealso::
    
    API documentation: |ids_validate|


Validate the time mode
''''''''''''''''''''''

The time mode of an IDS is stored in ``ids_properties.homogeneous_time``. This
property must be filled with a valid time mode (|tm_homogeneous|,
|tm_heterogeneous| or |tm_independent|). When the time
mode is |tm_independent|, all time-dependent quantities must be empty.


Validate coordinates
''''''''''''''''''''

If a quantity in your IDS has coordinates, then these coordinates must be filled. The
size of your data must match the size of the coordinates:

.. todo:: link to DD docs

1.  Some dimensions must have a fixed size. This is indicated by the Data Dictionary
    as, for example, ``1...3``.

    For example, in the ``magnetics`` IDS, ``b_field_pol_probe(i1)/bandwidth_3db`` has
    ``1...2`` as coordinate 1. This means that, if you fill this data field, the first
    (and only) dimension of this field must be of size 2.

2.  If the coordinate is another quantity in the IDS, then that coordinate must be
    filled and have the same size as your data.

    For example, in the ``pf_active`` IDS, ``coil(i1)/current_limit_max`` is a
    two-dimensional quantity with coordinates ``coil(i1)/b_field_max`` and
    ``coil(i1)/temperature``. This means that, if you fill this data field, their
    coordinate fields must be filled as well. The first dimension of
    ``current_limit_max`` must have the same size as ``b_field_max`` and the second
    dimension the same size as ``temperature``.

    Time coordinates are handled depending on the value of
    ``ids_properties/homogeneous_time``:

    -   When using |tm_homogeneous|, all time coordinates look at the root
        ``time`` node of the IDS.
    -   When using |tm_heterogeneous|, all time coordinates look at the time
        path specified as coordinate by the Data Dictionary.

        For dynamic array of structures, the time coordinates is a ``FLT_0D`` inside the
        AoS (see, for example, ``profiles_1d`` in the ``core_profiles`` IDS). In such
        cases the time node must be set to something different than ``EMPTY_FLOAT``.
        This is the only case in which values of the coordinates are verified, in all
        other cases only the sizes of coordinates are validated.

    .. rubric:: Alternative coordinates

    Version 4 of the Data Dictionary introduces alternative coordinates. An
    example of this can be found in the ``core_profiles`` IDS in
    ``profiles_1d(itime)/grid/rho_tor_norm``. Alternatives for this coordinate
    are:
    
    -   ``profiles_1d(itime)/grid/rho_tor``
    -   ``profiles_1d(itime)/grid/psi``
    -   ``profiles_1d(itime)/grid/volume``
    -   ``profiles_1d(itime)/grid/area``
    -   ``profiles_1d(itime)/grid/surface``
    -   ``profiles_1d(itime)/grid/rho_pol_norm``

    Multiple alternative coordinates may be filled (for example, an IDS might
    fill both the normalized and non-normalized toroidal flux coordinate). In
    that case, the size must be the same.

    When a quantity refers to this set of alternatives (for example
    ``profiles_1d(itime)/electrons/temperature``), at least one of the
    alternative coordinates must be set and its size match the size of the
    quantity.

3.  The Data Dictionary can indicate exclusive alternative coordinates. See for
    example the ``distribution(i1)/profiles_2d(itime)/density(:,:)`` quantity in the
    ``distributions`` IDS, which has as first coordinate
    ``distribution(i1)/profiles_2d(itime)/grid/r OR
    distribution(i1)/profiles_2d(itime)/grid/rho_tor_norm``. This means that
    either ``r`` or ``rho_tor_norm`` can be used as coordinate.
    
    Validation works the same as explained in the previous point, except that
    exactly one of the alternative coordinate must be filled. Its size must, of
    course, still match the size of the data in the specified dimension.

4.  Some quantites indicate a coordinate must be the same size as another quantity
    through the property ``coordinateX_same_as``. In this case, the other quantity is
    not a coordinate, but their data is related and must be of the same size.

    An example can be found in the ``edge_profiles`` IDS, quantity
    ``ggd(itime)/neutral(i1)/velocity(i2)/diamagnetic``. This is a two-dimensional field
    for which the first coordinate must be the same as
    ``ggd(itime)/neutral(i1)/velocity(i2)/radial``. When the diamagnetic velocity
    component is filled, the radial component must be filled as well, and have a
    matching size.
