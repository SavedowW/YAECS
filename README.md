# DESCRIPTION
Yes, its a yet another implementation of an ECS on c++ with templates and stuff.
(Probably) unique featues:
- As much as possible compile-time optimization
- Archetypes, queries, systems and registries are defined at compile time
- Query types are deduced at compile time and they include references to required archetypes, so you can get them, iterate or check referenced entities for other components with essentially 0 overhead
- Fast add / remove / convert operations for entities
- No global indexing (and I honestly don't know how to implement it, at least without type erasure, or even why would you use it)
- No preservable indexing within archetypes - this could be changed later on

The only relevant files are `yaECS.hpp`, `Archetype.hpp`, `UntypeContainer.h` and `TypeManip.hpp`. `ExampleComponents.h` contains relevant examples, `utils.h` contains some utilities used for debugging and dumping data, `main.cpp` contains examples of systems and usage examples, `Vector2.h` contains some structures used for examples, the rest are essentially irrelevant.
Files like `yaECS_Static.hpp` are related to an alternative, almost fully static implementation of ecs. While its totally usable, it was abandoned due to giant, ugly interfaces and types which can't be always avoided with templates and `auto`s.

# TODOs
- Add callbacks for add / delete / convert operations or when entity is moved for archetypes
  - Can be useful to, for example, keep dirty flags when something like static collision map changes and a system requires recalculations because of this
- Indexing within archetypes
- More operations which will become required later on
- Better interface
