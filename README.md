# DESCRIPTION
Yes, its a yet another implementation of an ECS on c++ with templates and stuff.
(Probably) unique featues:
- As much as possible compile-time optimization
- Archetypes, queries, systems and registries are defined at compile time
- Query types are deduced at compile time and they include references to required archetypes, so you can get them, iterate or check referenced entities for other components with essentially 0 overhead
- Fast add / remove / convert operations for entities
- No global indexing (and I honestly don't know how to implement it, at least without type erasure, or even why would you use it)
- No preservable indexing within archetypes - this could be changed later on

# TODOs
- Add callbacks for add / delete / convert operations or when entity is moved for archetypes
  - Can be useful to, for example, keep dirty flags when something like static collision map changes and a system requires recalculations because of this
- Indexing within archetypes
- More operations which will become required later on
- Better interface
