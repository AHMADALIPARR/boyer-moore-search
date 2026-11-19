import Lake
open Lake DSL

package «boyer-moore-formal» where
  version := v!"0.1.0"

-- Mathlib pinned to v4.12.0 to match the Lean 4.12.0 toolchain
-- (lean-toolchain). `rev = "master"` is a moving target and has
-- moved past Lean 4.12.0; pinning keeps the build reproducible.
require mathlib from git
  "https://github.com/leanprover-community/mathlib4.git" @ "v4.12.0"

-- Aesop is a Mathlib dependency; it is pulled in transitively
-- at the version Mathlib v4.12.0 requires.

-- The three proof modules, at the package root. Declaring the lib
-- makes `lake build` actually typecheck them (without it, `lake build`
-- succeeds vacuously with no targets). The roots are named explicitly:
-- otherwise Lake looks for a single root module named after the lib.
lean_lib BoyerMooreFormal where
  srcDir := "."
  roots := #[`TokenModel, `LinearAlgebraVerification, `ArrayTemporalLock]
