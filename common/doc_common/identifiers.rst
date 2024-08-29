Identifiers
===========

The "identifier" structure is used to provide an enumerated list of options for
defining, for example:

- A particular coordinate system, such as Cartesian, cylindrical, or spherical.
- A particle, which may be either an electron, an ion, a neutral atom, a
  molecule, a neutron, or a photon.
- Plasma heating may come from neutral beam injection, electron cyclotron
  heating, ion cyclotron heating, lower hybrid heating, alpha particles.

Identifiers are a list of possible valid labels. Each label has three
representations:

1. An index (integer)
2. A name (short string)
3. A description (long string).

.. csv-table:: Identifier examples (from part of the ``core_sources/source`` identifier)
    :header-rows: 1

    Index, Name, Description
    2, NBI, Source from Neutral Beam Injection
    3, EC, Sources from heating at the electron cyclotron heating and current drive
    4, LH, Sources from lower hybrid heating and current drive
    5, IC, Sources from heating at the ion cyclotron range of frequencies
    6, fusion, "Sources from fusion reactions, e.g. alpha particle heating"

The list of possible labels for a given identifier structure in the Data
Dictionary can be found in the |DD| documentation.

The use of private indices or names in identifiers structure is discouraged,
since this would defeat the purpose of having a standard enumerated list. Please
create a `JIRA <https://jira.iter.org/>`_ tracker when you want to add a new
identifier value.


Using the identifiers library
-----------------------------

|identifiers_link_instructions|

Below examples illustrates how to use the identifiers in your |lang| programs.

.. literalinclude:: code_samples/identifier_example1
    :caption: |lang| example 1: obtain identifier information of coordinate identifier ``phi``

.. literalinclude:: code_samples/identifier_example2
    :caption: |lang| example 2: Use the identifier library to fill the ``NBI`` label in the ``core_sources`` IDS

.. literalinclude:: code_samples/identifier_example3
    :caption: |lang| example 3: Use the identifier library to fill the type of coordinate system used in the ``equilibrium`` IDS

