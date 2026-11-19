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
  LinearAlgebraVerification.lean
  Complete formal verification of outer-product update algebra
-/

import Mathlib.Analysis.InnerProductSpace.Basic
import Mathlib.Data.Real.Basic

open scoped InnerProductSpace

/-!
# Linear Algebra Verification

Complete proofs of:
- Outer product action identity
- Linearity of matrix-vector multiplication
- Inner product expansion under update
- Exact change in projection
- Threshold conditions for projection crossing
- Existence of gains achieving arbitrary thresholds
-/

namespace LinearAlgebraVerification

/-! ## Vector Space Setup -/

variable {V : Type*} [NormedAddCommGroup V] [InnerProductSpace ℝ V]

/-! ## Outer Product Definition -/

/-- The outer product of vectors v and x: (v ⊗ xᵀ)(y) = ⟨x,y⟩ · v -/
def outerProduct (v x : V) : V →ₗ[ℝ] V where
  toFun y := ⟪x, y⟫_ℝ • v
  map_add' a b := by simp [inner_add_right, add_smul]
  map_smul' c y := by simp [inner_smul_right, smul_smul]

/-! ## ALG-001: Outer Product Action -/

/-- (v ⊗ xᵀ)x = ‖x‖² · v -/
theorem outerProductAction (v x : V) :
    outerProduct v x x = ⟪x, x⟫_ℝ • v := by
  rfl

/-! ## Update Operator -/

/-- ΔW = η · (v ⊗ xᵀ) -/
def updateOperator (η : ℝ) (v x : V) : V →ₗ[ℝ] V :=
  η • outerProduct v x

/-- Updated activation: (W + ΔW)x -/
def updatedActivation (W : V →ₗ[ℝ] V) (η : ℝ) (v x : V) : V :=
  (W + updateOperator η v x) x

/-! ## ALG-002: Linearity -/

/-- (W + ΔW)x = Wx + ΔWx -/
theorem linearity (W : V →ₗ[ℝ] V) (η : ℝ) (v x : V) :
    updatedActivation W η v x = W x + updateOperator η v x x := by
  simp [updatedActivation, LinearMap.add_apply]

/-! ## ALG-003: Inner Product Expansion -/

/-- ⟨(W + ΔW)x, v⟩ = ⟨Wx, v⟩ + ⟨ΔWx, v⟩ -/
theorem innerProductExpansion (W : V →ₗ[ℝ] V) (η : ℝ) (v x : V) :
    ⟪updatedActivation W η v x, v⟫_ℝ =
    ⟪W x, v⟫_ℝ + ⟪updateOperator η v x x, v⟫_ℝ := by
  rw [linearity, inner_add_left]

/-! ## ALG-004: Exact Change in Projection -/

/-- ⟨(W + ΔW)x, v⟩ - ⟨Wx, v⟩ = η · ‖x‖² · ‖v‖² -/
theorem exactChangeInProjection (W : V →ₗ[ℝ] V) (η : ℝ) (v x : V) :
    ⟪updatedActivation W η v x, v⟫_ℝ - ⟪W x, v⟫_ℝ =
    η • ⟪x, x⟫_ℝ • ⟪v, v⟫_ℝ := by
  rw [innerProductExpansion, add_sub_cancel_left]
  simp [updateOperator, outerProductAction, inner_smul_left, smul_smul, smul_eq_mul, mul_assoc]

/-! ## Expansion in multiplicative form -/

/-- ⟨(W + ΔW)x, v⟩ = ⟨Wx, v⟩ + η · ‖x‖² · ‖v‖² -/
theorem updatedActivation_inner (W : V →ₗ[ℝ] V) (η : ℝ) (v x : V) :
    ⟪updatedActivation W η v x, v⟫_ℝ
      = ⟪W x, v⟫_ℝ + η * (⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ) := by
  rw [innerProductExpansion]
  congr 1
  have hupd : updateOperator η v x x = η • ⟪x, x⟫_ℝ • v := by
    simp [updateOperator, outerProductAction, smul_smul, smul_eq_mul]
  rw [hupd]
  simp [inner_smul_left, smul_eq_mul, mul_assoc]

/-! ## Positivity of self inner products -/

private theorem self_inner_pos {x : V} (hx : x ≠ 0) : 0 < ⟪x, x⟫_ℝ := by
  rw [real_inner_self_eq_norm_sq]
  exact pow_pos (norm_pos_iff.mpr hx) 2

/-! ## THR-001: Sufficient Condition -/

/-- η > (θ - ⟨Wx,v⟩)/(‖x‖²·‖v‖²) implies threshold crossing -/
theorem thresholdSufficient (W : V →ₗ[ℝ] V) (η : ℝ) (v x : V) (θ : ℝ)
    (hx : x ≠ 0) (hv : v ≠ 0)
    (hη : η > (θ - ⟪W x, v⟫_ℝ) / (⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ)) :
    ⟪updatedActivation W η v x, v⟫_ℝ > θ := by
  have hprod : 0 < ⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ :=
    mul_pos (self_inner_pos hx) (self_inner_pos hv)
  rw [updatedActivation_inner]
  have hη' : (θ - ⟪W x, v⟫_ℝ) < η * (⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ) := (div_lt_iff hprod).mp hη
  linarith

/-! ## THR-002: Existence of Valid Gain -/

/-- For any finite θ and nonzero x,v, there exists η > 0 achieving threshold -/
theorem existenceOfValidGain (W : V →ₗ[ℝ] V) (v x : V) (θ : ℝ)
    (hx : x ≠ 0) (hv : v ≠ 0) :
    ∃ η > 0, ⟪updatedActivation W η v x, v⟫_ℝ > θ := by
  have hprod : 0 < ⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ :=
    mul_pos (self_inner_pos hx) (self_inner_pos hv)
  refine ⟨(|θ - ⟪W x, v⟫_ℝ| + 1) / (⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ), ?_, ?_⟩
  · exact div_pos (add_pos_of_nonneg_of_pos (abs_nonneg _) one_pos) hprod
  · rw [updatedActivation_inner]
    have hcan : (|θ - ⟪W x, v⟫_ℝ| + 1) / (⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ) * (⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ)
        = |θ - ⟪W x, v⟫_ℝ| + 1 :=
      div_mul_cancel₀ _ (ne_of_gt hprod)
    rw [hcan]
    have hle := le_abs_self (θ - ⟪W x, v⟫_ℝ)
    linarith

/-! ## Arbitrary Margin -/

/-- For any ε > 0, there exists η > 0 achieving margin θ + ε -/
theorem arbitraryMargin (W : V →ₗ[ℝ] V) (v x : V) (θ ε : ℝ)
    (hx : x ≠ 0) (hv : v ≠ 0) (_hε : ε > 0) :
    ∃ η > 0, ⟪updatedActivation W η v x, v⟫_ℝ > θ + ε := by
  have hprod : 0 < ⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ :=
    mul_pos (self_inner_pos hx) (self_inner_pos hv)
  refine ⟨(|θ + ε - ⟪W x, v⟫_ℝ| + 1) / (⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ), ?_, ?_⟩
  · exact div_pos (add_pos_of_nonneg_of_pos (abs_nonneg _) one_pos) hprod
  · rw [updatedActivation_inner]
    have hcan : (|θ + ε - ⟪W x, v⟫_ℝ| + 1) / (⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ) * (⟪x, x⟫_ℝ * ⟪v, v⟫_ℝ)
        = |θ + ε - ⟪W x, v⟫_ℝ| + 1 :=
      div_mul_cancel₀ _ (ne_of_gt hprod)
    rw [hcan]
    have hle := le_abs_self (θ + ε - ⟪W x, v⟫_ℝ)
    linarith

end LinearAlgebraVerification
