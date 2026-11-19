/-
  SOVEREIGN LEVIATHAN NODE LICENSE
  License-ID: SL-AGPL3-001 | Covenant-Version: 1.0
  Copyright (C) 2026 SnapKittyWest. Ahmad Ali Parr, Bel Esprit D'Accord Irrevocable Trust.
  ========================================================================

  This file is a covered work under the GNU Affero General Public License,
  version 3, together with the Sovereign Leviathan additional terms.

  Licensed under: Apache-2.0 OR GPL-3.0-or-later
  Commercial repository - no MIT license.

  Hark, though this node be but a spark,
  Its covenant endureth through the dark.

  Ignorantia juris non excusat.
  ========================================================================
-/

/-
  TokenModel.lean
  Core definitions for formal verification of linear-algebraic transformation protocol

  This file defines:
  - Finite-dimensional real inner product spaces
  - Linear operators (matrices)
  - Outer products
  - Update operators
  - Activation functions
  - Threshold predicates
-/

import Mathlib.Analysis.InnerProductSpace.Basic
import Mathlib.Data.Matrix.Basic
import Mathlib.Data.Real.Basic

open scoped InnerProductSpace

namespace TokenModel

/-! ## Vector Space Definitions -/

/- A finite-dimensional real inner product space -/
variable {V : Type*} [NormedAddCommGroup V] [InnerProductSpace ℝ V] [FiniteDimensional ℝ V]

/-- The dimension of the vector space -/
noncomputable def dim (V : Type*) [NormedAddCommGroup V] [InnerProductSpace ℝ V] [FiniteDimensional ℝ V] : ℕ :=
  FiniteDimensional.finrank ℝ V

/-! ## Outer Product -/

/-- The outer product of two vectors: v ⊗ xᵀ -/
def outerProduct {V : Type*} [NormedAddCommGroup V] [InnerProductSpace ℝ V] (v x : V) : V →ₗ[ℝ] V where
  toFun y := ⟪x, y⟫_ℝ • v
  map_add' a b := by simp [inner_add_right, add_smul]
  map_smul' c y := by simp [inner_smul_right, smul_smul]

/-- Matrix representation of outer product -/
def outerProductMatrix {n : ℕ} (v x : Fin n → ℝ) : Matrix (Fin n) (Fin n) ℝ :=
  Matrix.of (fun i j => v i * x j)

/-! ## Update Operator -/

/-- The update operator: ΔW = η · (v ⊗ xᵀ) -/
def updateOperator {V : Type*} [NormedAddCommGroup V] [InnerProductSpace ℝ V]
    (η : ℝ) (v x : V) : V →ₗ[ℝ] V :=
  η • outerProduct v x

/-- Matrix representation of update operator -/
def updateMatrix {n : ℕ} (η : ℝ) (v x : Fin n → ℝ) : Matrix (Fin n) (Fin n) ℝ :=
  η • outerProductMatrix v x

/-! ## Activation -/

/-- The activation of input x under operator W -/
def activation {V : Type*} [NormedAddCommGroup V] [InnerProductSpace ℝ V]
    (W : V →ₗ[ℝ] V) (x : V) : V :=
  W x

/-- The updated activation -/
def updatedActivation {V : Type*} [NormedAddCommGroup V] [InnerProductSpace ℝ V]
    (W : V →ₗ[ℝ] V) (η : ℝ) (v x : V) : V :=
  (W + updateOperator η v x) x

/-! ## Threshold Predicate -/

/-- The threshold predicate: ThoughtFires(y, v, θ) ⟺ ⟨y, v⟩ > θ -/
def thoughtFires {V : Type*} [NormedAddCommGroup V] [InnerProductSpace ℝ V]
    (y v : V) (θ : ℝ) : Prop :=
  ⟪y, v⟫_ℝ > θ

/-! ## Core Theorems -/

/-- AX-001: Outer product action -/
theorem outerProductAction {V : Type*} [NormedAddCommGroup V] [InnerProductSpace ℝ V]
    (v x : V) :
    (outerProduct v x) x = ⟪x, x⟫_ℝ • v := by
  rfl

/-- AX-002: Update action -/
theorem updateAction {V : Type*} [NormedAddCommGroup V] [InnerProductSpace ℝ V]
    (η : ℝ) (v x : V) :
    (updateOperator η v x) x = η • ⟪x, x⟫_ℝ • v := by
  simp [updateOperator, outerProductAction, smul_smul, smul_eq_mul]

/-- ALG-001: Linearity of updated activation -/
theorem linearityOfUpdatedActivation {V : Type*} [NormedAddCommGroup V] [InnerProductSpace ℝ V]
    (W : V →ₗ[ℝ] V) (η : ℝ) (v x : V) :
    (W + updateOperator η v x) x = W x + (updateOperator η v x) x := by
  simp [LinearMap.add_apply]

/-- ALG-002: Projection expansion -/
theorem projectionExpansion {V : Type*} [NormedAddCommGroup V] [InnerProductSpace ℝ V]
    (W : V →ₗ[ℝ] V) (η : ℝ) (v x : V) :
    ⟪(W + updateOperator η v x) x, v⟫_ℝ =
    ⟪W x, v⟫_ℝ + ⟪(updateOperator η v x) x, v⟫_ℝ := by
  rw [linearityOfUpdatedActivation, inner_add_left]

/-- ALG-003: Exact change in projection -/
theorem exactChangeInProjection {V : Type*} [NormedAddCommGroup V] [InnerProductSpace ℝ V]
    (W : V →ₗ[ℝ] V) (η : ℝ) (v x : V) :
    ⟪(W + updateOperator η v x) x, v⟫_ℝ - ⟪W x, v⟫_ℝ =
    η • ⟪x, x⟫_ℝ • ⟪v, v⟫_ℝ := by
  rw [projectionExpansion, add_sub_cancel_left]
  simp [updateAction, inner_smul_left, smul_eq_mul]

/-- THR-001: Sufficient condition for threshold crossing -/
theorem thresholdSufficientCondition {V : Type*} [NormedAddCommGroup V] [InnerProductSpace ℝ V]
    (W : V →ₗ[ℝ] V) (η : ℝ) (v x : V) (θ : ℝ)
    (hx : x ≠ 0) (hv : v ≠ 0)
    (hη : η > (θ - ⟪W x, v⟫_ℝ) / (⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ)) :
    thoughtFires ((W + updateOperator η v x) x) v θ := by
  have hxi : 0 < ⟪x, x⟫_ℝ := by
    rw [real_inner_self_eq_norm_sq]
    exact pow_pos (norm_pos_iff.mpr hx) 2
  have hvi : 0 < ⟪v, v⟫_ℝ := by
    rw [real_inner_self_eq_norm_sq]
    exact pow_pos (norm_pos_iff.mpr hv) 2
  have hprod : 0 < ⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ := mul_pos hxi hvi
  have hexp : ⟪(W + updateOperator η v x) x, v⟫_ℝ
      = ⟪W x, v⟫_ℝ + η * (⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ) := by
    have h := exactChangeInProjection W η v x
    have h2 : η • ⟪x, x⟫_ℝ • ⟪v, v⟫_ℝ = η * (⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ) := by
      simp [smul_eq_mul, mul_assoc]
    linarith
  show ⟪(W + updateOperator η v x) x, v⟫_ℝ > θ
  rw [hexp]
  have hη' : (θ - ⟪W x, v⟫_ℝ) < η * (⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ) := (div_lt_iff hprod).mp hη
  linarith

end TokenModel
