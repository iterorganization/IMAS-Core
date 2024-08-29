Loading and storing IMAS data
=============================

IMAS data is grouped together in Data Entries. A Data Entry is a collection of
:ref:`IDSs <Use Interface Data Structures>` and their (potentially) multiple
occurrences, which groups and stores data over multiple IDSs as a single
dataset. The Data Entry concept is used whether the collection of IDSs is stored
in a database or only exists temporarily (for example for communication in an
integrated workflow).

Loading and storing IMAS data happens through an IMAS Database Entry. A Database
Entry tracks the information required for locating where the Data Entry is (or
will be) stored on disk. In |lang| this object is modeled as |dbentry|.

You may :ref:`open an existing IMAS Database Entry`, which you can use for
loading data that was stored previously. Alternatively you can :ref:`create a
new IMAS Database Entry` to store IDS data.


Open an existing IMAS Database Entry
------------------------------------

To open an IMAS Database Entry, you need to know the URI indicating where the
Access Layer can find the data. IMAS URIs start with ``imas:`` and indicate
the format and the location of the stored data. You can find a detailed
description of the IMAS URI syntax on the :ref:`Data entry URIs` page.

.. literalinclude:: code_samples/dbentry_open
    :caption: |lang| example: open an existing IMAS Database Entry

.. seealso::

    API documentation for |dbentry_open|.


Loading IMAS data
-----------------

After you open a database entry, you can request to load data from disk.

.. contents::
    :local:


Load an entire IDS
''''''''''''''''''

With |dbentry_get| you can load ("get") an entire IDS from the database entry.

Multiple `occurrences` of an IDS may be stored in a data entry. By default, if
you don't specify an occurrence number, occurrence 0 is loaded. By providing an
occurrence number you can load a specific occurrence. How different occurrences
are used depends on the experiment. They could, for example, correspond to:

- different methods for computing the physical quantities of the IDS, or
- different functionalities in a workflow (e.g. initial values, prescribed
  values, values at next time step, …), or
- multiple subsystems (e.g. diagnostics) of the same type in an experiment, etc.

.. todo:: extend docs after Task 2c. is implemented (get multiple occurrences)

.. literalinclude:: code_samples/dbentry_get
    :caption: |lang| example: get an IDS from an IMAS Database Entry

.. seealso::

    - API documentation for |dbentry_get|.


Load a single `time slice` of an IDS
''''''''''''''''''''''''''''''''''''

Instead of loading a full IDS from disk, the Access Layer allows you to load a
specific `time slice`. This is often useful when you're not interested in the
full time evolution, but instead want data of a specific time. You can use
|dbentry_getslice| for this.

Most of the time there are no entries at that specific time, so you also need to
indicate an `interpolation method`. This determines what values the access layer
returns when your requested time is in between available time points in the
data. Three interpolation methods currently exist:

|PREVIOUS_INTERP|
    Returns the `previous` time slice if the requested time does not exactly
    exist in the original IDS.

    For example, when data exists at :math:`t=\{1, 3, 4\}`, requesting
    :math:`t_r=2.1` will give you the data at :math:`t=1`.

    .. csv-table:: Edge case behaviour. :math:`\{t_i\}, i=1..N` represents the time series stored in the IDS.
        :header-rows: 1

        Case, Behaviour
        :math:`t_r \lt t_1`, Return data at :math:`t_1`.
        :math:`t_r = t_i` [#equal_note]_, Return data at :math:`t_i`.


|CLOSEST_INTERP|
    Returns the `closest` time slice in the original IDS. This can also be
    `after` the requested time.

    For example, when data exists at :math:`t=\{1, 3, 4\}`, requesting
    :math:`t=2.1` will give you the data at :math:`t=3`.

    .. csv-table:: Edge case behaviour. :math:`\{t_i\}, i=1..N` represents the time series stored in the IDS.
        :header-rows: 1

        Case, Behaviour
        :math:`t_r \lt t_1`, Return data at :math:`t_1`.
        :math:`t_r = t_i` [#equal_note]_, Return data at :math:`t_i`.
        :math:`t_r - t_i = t_{i+1} - t_r` [#equal_note]_, Return data at :math:`t_{i+1}`.

.. [#equal_note] Equality for floating point numbers is tricky. For example,
    :code:`3.0/7.0 + 2.0/7.0 + 2.0/7.0` is not exactly equal to :code:`1.0`. It
    is therefore advised not to depend on this behaviour.

|LINEAR_INTERP|
    Returns a linear interpolation between the existing slices before and after
    the requested time.

    For example, when data exists at :math:`t=\{1, 3, 4\}`, requesting
    :math:`t=2.1` will give you a linear interpolation of the data at
    :math:`t=1` and the data at :math:`t=3`.

    Note that the linear interpolation will be successful only if, between the
    two time slices of an interpolated dynamic array of structure, the same
    leaves are populated and they have the same size. Otherwise
    |dbentry_getslice| will interpolate all fields with a compatible size and
    leave others empty.

    .. csv-table:: Edge case behaviour. :math:`\{t_i\}, i=1..N` represents the time series stored in the IDS.
        :header-rows: 1

        Case, Behaviour
        :math:`t_r \lt t_1`, Return data at :math:`t_1`.
        :math:`t_r \gt t_N`, Return data at :math:`t_N`.

.. literalinclude:: code_samples/dbentry_getslice
    :caption: |lang| example: get a time slice from an IMAS Database Entry

.. note::

    The access layer assumes that all time arrays are stored in increasing
    order. |dbentry_getslice| may return unexpected results if your data does
    not adhere to this assumption.

.. seealso::

    API documentation for |dbentry_getslice|.


.. include:: partial_get


Create a new IMAS Database Entry
--------------------------------

To create a new IMAS Database Entry, you need to provide the URI to indicate the
format and the location where you want to store the data. You can find a
detailed description of the IMAS URI syntax and the options available on the
:ref:`Data entry URIs` page.

.. caution::
    
    This function erases any existing database entry on the specified URI!


.. literalinclude:: code_samples/dbentry_create
    :caption: |lang| example: create a new IMAS Database Entry

.. seealso::

    API documentation for |dbentry_create|.


Store IMAS data
---------------

After you have created an IMAS Database Entry, you can use it for storing IDS
data. There are two ways to do this:

.. contents::
    :local:


Store an entire IDS
'''''''''''''''''''

With |dbentry_put| you can store ("put") an entire IDS in a database entry.
First you need to have an IDS with data: you can create a new one or :ref:`load
an IDS <Loading IMAS Data>` which you modify. See :ref:`Use Interface Data
Structures` for more information on using and manipulating IDSs.

.. caution::

    This function erases the existing IDS in the data entry if any was already
    stored previously.

Multiple `occurrences` of an IDS may be stored in a data entry. By default, if
you don't specify an occurrence number, the IDS is stored as occurrence 0. By
providing an occurrence number you can store the IDS as a specific occurrence.

.. note::

    The MDS+ backend has a limitation on the number of occurrences of a given
    IDS. This number is indicated in the |DD| documentation in the "Max.
    occurrence number" column of the list of IDSs. This limitation doesn't apply
    to other backends.

.. literalinclude:: code_samples/dbentry_put
    :caption: |lang| example: put an IDS to an IMAS Database Entry

.. seealso::

    API documentation for |dbentry_put|.


Append a time slice to an already-stored IDS
''''''''''''''''''''''''''''''''''''''''''''

With |dbentry_put_slice| you can append a time slice to an existing database
entry. This is useful when you generate data inside a time loop (for example in
simulations, or when taking measurements of an experiment).

It means you can put a time slice with every iteration of your loop such that
you don't have to keep track of the complete time evolution in memory. Instead,
the Access Layer will keep appending the data to the Database Entry in the
storage backend.

.. note::

    Although being put progressively time slice by time slice, the final IDS
    must be compliant with the data dictionary. A typical error when
    constructing IDS variables time slice by time slice is to change the size of
    the IDS fields during the time loop, which is not allowed but for the
    children of an array of structure which has time as its coordinate.

.. literalinclude:: code_samples/dbentry_put_slice
    :caption: |lang| example: iteratively put time slices to an IMAS Database Entry

.. seealso::

    API documentation for |dbentry_put_slice|.


Listing all occurrences of an IDS from a backend
''''''''''''''''''''''''''''''''''''''''''''''''

With |list_all_occurrences| you can List all non-empty occurrences of an IDS 
using its name in the dataset, and optionnally return the content of a 
descriptive node path. 

.. note::

    The MDS+ backend is storing IDS occurrences infos (pulse file metadata)
    for AL version > 5.0.0. Pulse files created with AL version <= 5.0.0. 
    do not provide these informations (an exception will occur for such 
    pulse files when calling |list_all_occurrences|).

.. literalinclude:: code_samples/dbentry_list_all_occurrences
    :caption: |lang| example: listing all occurrences of a magnetics IDS from an IMAS Database Entry
