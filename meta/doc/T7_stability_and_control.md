# T7 stability and control analysis — user guide

This guide covers the T7 (stability) analysis of xfl-type planes in flow5: how to set it up, what each option does, how to read the log and the graphs, and how to recognise failed or unreliable results. Sections marked **(new)** describe features added on top of the original flow5 T7 implementation.

---

## 1. What T7 computes

For each value of the **control parameter** in the analysis range, T7:

1. Sets the flaps and wing angles defined in the polar's **Flaps** tab (deflection = Flaps-tab value × control parameter).
2. **Trims** the plane: finds the angle of attack α₀ where Cm = 0 about the CoG, then the speed V₀ where lift equals weight. The reference state is **level flight** (flight path angle γ₀ = 0, so pitch attitude θ₀ = α₀).
3. Computes the **stability derivatives** (X_u, Z_w, M_q, L_p, N_r, …) by perturbing the trimmed solution.
4. Computes one **control derivative** set (X_δ … N_δ) for each AVL-type control set, by deflecting its surfaces by a small amount.
5. Builds the linear state matrices (longitudinal: u, w, q, θ; lateral: v, p, r, φ), and computes the **eigenmodes**.

From these, the time response view simulates **free** responses (modal, initial conditions) and **forced** responses to a control input history.

### What is and is not modelled

| Item | Status |
|---|---|
| Trim, stability and control derivatives | **Inviscid** panel solution |
| Viscous drag (profile, fuselage, extra drag) | Added to the operating point results if the polar is viscous; **(new)** also added to X_u |
| Viscous loss of control-surface effectiveness | Not computed; **(new)** user-defined effectiveness factor |
| Downwash lag (Z_ẇ, M_ẇ) | **(new)** optional geometric estimate; otherwise zero |
| Propulsion (thrust, thrust–speed variation, slipstream) | Not modelled |
| Stall, flap separation, large-angle aerodynamics | Not modelled — the model is linear |
| Coupling between longitudinal and lateral motion | Not modelled — integrated separately |

---

## 2. Before you start: plane prerequisites

- **Masses and inertia.** The rates in a response scale with 1/I during the first fraction of a second. Define the structural masses of the wings and fuselage **and** every significant point mass: battery, servos, receiver, motor, ballast. A structure-only plane usually has 1.5–2× too little inertia.
- **Control surfaces.** Each surface you want to deflect must be a trailing-edge flap in the wing definition. In the polar dialogs, flaps are numbered `flap_1`, `flap_2`, … per wing.
- **Wing types.** The downwash lag estimate (option C, §3.6) needs one wing of type *Main* and one of type *Elevator*.
- **Foil polars.** For a viscous analysis, the foils' 2D polars must cover the Reynolds numbers and angles reached by each wing. Otherwise the interpolation is clamped (§7.1).

---

## 3. Setting up the T7 polar

Create a new plane polar and select **Type 7** on the *Polar type* tab. The tabs relevant to T7:

### 3.1 Method, reference dimensions, fluid
Use the same choices as for your other analyses. The reference area, span and chord are used for the nondimensional derivatives printed in the log.

### 3.2 Inertia
- **Auto inertia checked:** mass, CoG and inertia tensor are copied from the plane **at the start of every run**.
- **Unchecked:** the values typed in the tab are used as-is.

The inertia tensor is **not** varied with the control parameter, even if mass or CoG are. The log warns about this (§5.1).

### 3.3 Viscosity
- **Viscous checked:** viscous drag is interpolated from the foil polars and added to the operating point results (CD, VCD, viscous moments). **(new)** It is also added to X_u, which improves the phugoid damping (§3.6, option B).
- It does **not** change the trim, the lift or any other derivative.
- The *Viscous loop* option is not available for T7.

### 3.4 Flaps — the trim deflection
Each value is a **gain in degrees per unit of control parameter**. The trim deflection of a flap is:

    δ_trim = Flaps-tab value × control parameter

**Example:** to trim with the elevator 2° trailing edge up, enter −2 on the elevator flaps and run at control parameter 1. Alternatively, enter −1 and run at 2.

**Sign convention:** positive flap angles are **trailing edge down**.

### 3.5 AVL-type ctrls — control sets for the forced response
Each **control set** produces one set of control derivatives, and is what the forced response actuates.

- Right-click the left table to *Append*, *Duplicate*, *Delete* or *Move* a set.
- Select a set, then enter in the right table each surface's **gain in degrees per control unit**. Surfaces with gain 0 do not move.
- A set whose gains are all zero is skipped, and produces no derivatives.
- **(new) Effectiveness** column (default 1.0): a factor applied to that set's control derivatives only. The deflections shown in the graphs stay the true surface angles.
  - The inviscid panel method overestimates the effectiveness of plain flaps, especially at low Reynolds numbers, with hinge gaps or with small flap chords.
  - Typical corrections are **0.6–0.9**. Calibrate from flight data if you can.

**Example:** to deflect only the left elevator, create a set `ElevLh` with gain 1 on that elevator's flap and 0 elsewhere. A forced input u = −3 then means 3° trailing edge up relative to trim.

### 3.6 (new) Downwash lag derivatives — option C
Checkbox at the bottom of the *AVL-type ctrls* tab: **Include the downwash lag derivatives (estimate)**. Off by default.

- **Why it matters:** the quasi-steady panel solution gives no derivatives with respect to α̇. For a conventional tail, the delay of the wing's downwash at the tail supplies roughly 20–40% of the short-period damping. Without it, the overshoot and oscillation in q, α and n_z after an elevator input are overestimated.
- **How it's estimated:** M_ẇ = −q·S_t·a_t·(dε/dα)·l_t²/V₀², and Z_ẇ = M_ẇ/l_t, with the tail lift slope a_t from Helmbold's formula. All inputs are printed in the log.
- **Where dε/dα comes from:** selected with the combo box next to the checkbox. **The selected method is used for every point of the run; there is no switching between methods**, so results stay smooth along the control range.
  - **Panel solution (default):** the flow induced by the wing, the fuselage and the wing's wake (the tail's own panels and wake are excluded) is evaluated along the tail's quarter-chord line at α₀ ± 1°. This accounts for the actual planform, twist, fuselage, and the tail's vertical and longitudinal position.
    - **Frozen samples:** the sample stations are chosen at the first point of the run and kept for all points. The inner 15% of the tail semi-span is always excluded, since it may lie inside the fuselage.
    - **Regularized wake (checkbox, default on):** the wing's wake is evaluated at the tail as vortex rings with a Lamb–Oseen viscous core: 2.5% of the wing's MAC at the trailing edge, growing as √(1 + x/MAC) downstream. This removes the singularity of the infinitely thin sheet, so stations inside the wake plane can be sampled, and it approximates the smoothed downwash a tail in a real, finite-thickness wake sees. Only the downwash evaluation is affected, not the main solution.
    - **Unchecked:** the wake is the solver's singular sheet. Stations closer than 0.1 MAC to it are excluded, and a point whose frozen stations come within 0.03 MAC of the sheet is flagged as unreliable.
    - **Low coverage:** if less than half of the tail span can be sampled, the run continues on the available stations, with a warning. If no station is usable, the downwash lag terms are left out of the whole run, with a warning.
    - **Wake model:** flow5's wake is a flat sheet trailing along the body x-axis. It doesn't follow the real flow direction or roll up, so for a tail close to the wing's trailing-edge height, the result depends on that assumed wake position.
  - **DATCOM:** empirical, from the wing's aspect ratio, taper and sweep, and the tail's distance and height. Mostly based on full-size aircraft data. Useful as a comparison, or when the tail sits in the wake plane.
- **When it does nothing:** without a *Main* wing and an *Elevator*, e.g. on a flying wing. The log says so.

### 3.7 Other tabs
*Ground*, *Fuselage*, *Extra drag* and *Wake* work as in the other polar types.

---

## 4. Running the analysis

1. **Check "Store operating points"** in the analysis panel. T7 points are not kept otherwise, and the time response view will have nothing to work with.
2. **Set the control parameter range** in the table:
   - Each row has a start, an end and an increment. A row with start = end and increment 0 runs **one** point at the start value. For example, 0 / 0 / 0 runs once at control 0.
   - **Grey rows are inactive and skipped**, even if they contain values. Click the first cell of a row, or right-click → *Activate/de-activate*, to toggle it. Editing a value activates its row.
   - For forced responses about the neutral elevator, include **0**. Each control value is a separate trim point.
3. **Click Calculate.** If nothing can run, a message in the log window and status bar explains why: no plane, no polar, external polar, no active row, unsupported method. A T7 polar without any active AVL-type control set produces a note: the analysis runs, but forced responses will not be available.

---

## 5. Reading the analysis log

The log is written in this order. Each block tells you which effects are active.

### 5.1 (new) Inertia block, once per run
```
   Inertia used by the stability analysis
      Source: plane inertia, computed from the parts and point masses
      mass = …  CoG = (…)
      Ixx = …  Iyy = …  Izz = …  Ixz = …
      Breakdown about the CoG …
         component   mass(kg)  x(m)  z(m)  Ixx(%)  Iyy(%)  Izz(%)
      Nondimensional radii of gyration (Roskam): Rx = …, Ry = …, Rz = …
      Note: the trim and the stability and control derivatives are computed from the inviscid solution.
            Viscous drag in X_u: included / not included
            Downwash lag derivatives: estimated and included / not included
            Control set …: effectiveness …
```

**What to check:**
- **Source.** Is it the inertia you intended to use?
- **Breakdown.** Is every heavy item listed, at a plausible x? Is its share believable? A battery far forward should carry a large part of Iyy, and wing tips a large part of Ixx.
- **Radii of gyration.** Typical conventional aircraft fall in 0.2–0.4. A value outside 0.15–0.5 triggers a warning and almost always means missing or misplaced masses.
- **Warnings:**
  - *no point masses*
  - *polar inertia differs by more than 5% from the plane's components* (custom polar inertia that no longer matches the plane)
  - *mass or CoG vary with the control variable but the inertia is kept constant*
- **The Note lines.** This is the definitive record of which corrections apply to this run.

### 5.2 Per control value
```
    Processing control value= …
      Setting flap positions            ← trim deflections from the Flaps tab
      Calculating trimmed conditions
      Calculating Plane for α=…         ← trimmed angle of attack
      Adding interpolated viscous drag… ← only if viscous
      Calculating stability derivatives
          Viscous drag … N added to X_u: dX_u = …      ← (new) option B
          Wake proximity check: …                       ← (new) first point only, every run (§7.1)
          Downwash gradient method for this run: …      ← (new) option C, first point only: method, regularized wake, stations sampled, % of tail span
          Downwash lag derivatives (estimate): …        ← (new) option C
             tail arm from CoG l_t = …, S_t = …, AR_t = …, a_t = …
             tail behind wing a.c. l_H = …, tail height above wing root chord plane h_H = … (h_H/b = …)
             AR_w = …, taper = …, sweep c/4 = …
             deps/dalpha (panel solution) = …  <- used;  downwash at the tail at trim = … deg
             deps/dalpha (DATCOM)         = …  (would be … with the tail in the wing plane)
             Z_wdot = …,  M_wdot = …
             M_alphadot/M_q = …
             Processing control set …
        Control derivatives multiplied by the effectiveness factor …   ← (new) option A, only if ≠ 1
      Longitudinal / Lateral / Control derivatives tables
      Neutral Point position = …
      Calculating eigenthings
      ___Longitudinal modes___ / ___Lateral modes___
```

**Sanity checks on the derivative tables** (stability axes; signs for a conventional, statically stable aircraft):

| Derivative | Expected sign | If wrong |
|---|---|---|
| Cma | negative | statically unstable in pitch: CoG behind the neutral point |
| Cmq | negative | pitch damping missing: check the tail geometry |
| CZa | negative | |
| Cxu | negative | |
| Cnb | positive | directionally unstable: fin too small or too far forward |
| Clb | negative | negative dihedral effect: check dihedral and wing position |
| Clp | negative | |
| Cnr | negative | |

- **Neutral point.** It must lie **behind** the CoG for static stability. The static margin is (x_NP − x_CoG) / MAC.
- **(new) Downwash lag rows** (option C only), printed under `Mq` in the longitudinal table: `Zwp`/`CZad`, `Mwp`/`Cmad` and `Total pitch damping Cmq+Cmad`. **Cma and Cmq do not change with option C.** The downwash lag is a separate term, which acts through the state matrix on the eigenvalues and the time responses. The total pitch damping line shows its weight: Cmad should be negative, and should add roughly 20–40% to Cmq.
- **M_alphadot/M_q** (option C). Expect roughly 0.2–0.4 for a conventional tail.
- **Panel vs DATCOM dε/dα** (option C). Both are printed at every point; the selected one is marked `<- used`. Agreement within about ±25% is normal. A large disagreement, a low sampled fraction at the start of the run, or a per-point warning that a sample lies close to the wake sheet means the tail is near the assumed wake: treat M_ẇ as uncertain, or compare with a DATCOM run. Values above about 0.6, or a negative tail arm, point to a wrong wing type assignment or a wrong CoG.
- **Control derivatives.** A set that should move the elevator must produce a clearly non-zero CMde. For a one-sided surface, also expect CLde and CNde.

### 5.3 Eigenvalues
Each mode is printed as an eigenvalue λ = σ + iω.

- **σ < 0:** the mode decays. Time to half amplitude is 0.69/|σ|.
- **σ > 0:** the mode diverges. Time to double amplitude is 0.69/σ.
- **Typical identification:**
  - **Longitudinal:** a fast, well-damped complex pair is the *short period*; a slow, lightly damped pair is the *phugoid*.
  - **Lateral:** a large negative real value is *roll subsidence*; a small real value near zero is the *spiral* (positive means slowly divergent, which is common and usually acceptable); a complex pair is the *Dutch roll*.
- A positive real eigenvalue in the short-period or Dutch-roll range indicates a static instability, not just a slow mode.

---

## 6. Graphs

### 6.1 Root locus view (Shift+F8)
Plots the eigenvalues of every stored T7 point. Follow how the modes move with the control parameter. Any mode crossing into the right half-plane (σ > 0) becomes unstable.

### 6.2 Time response view (Ctrl+F8)
Select a T7 operating point first. The panel on the right controls the response:

- **Longitudinal / Lateral:** the motion to simulate. The two are integrated separately, and switching clears the curves.
- **Response type:**
  - **Modal:** excites a single eigenmode. Its amplitude comes from a scaled eigenvector and is **arbitrary**, so only the shape and timing are meaningful.
  - **Initial conditions:** starts from u₀, w₀, q₀ (or v₀, p₀, r₀) in the displayed units. **(fixed)** These are now correctly converted from your speed unit and from °/s.
  - **Forced:** response to a control input history, using the control set picked in the dropdown. The dropdown lists the polar's AVL-type control sets; if it reads *No AVL-type control set in the polar*, see §7.2.
- **Total time / Δt:** the simulation stores at most 1000 points; the step is max(total time / 1000, Δt). Keep **total time / 1000 well below the fastest time constant** (§8.2).
- **Add** computes a new curve with the current settings.
- **Recompute** recomputes the curve selected in the list with the current settings: response type, time step, total time, input function and control set. The curve keeps its name and style; rename it if the settings changed meaningfully.
- Selecting a curve, changing its style or changing a graph's variable **only redraws** the stored curves, so earlier curves stay intact for comparison.

#### (new) Forced input: table and graph
Under the input graph:

- **Table (t, u):** any number of rows. Right-click to *Insert before*, *Insert after*, *Append*, *Delete*, *Copy* or *Paste* (columns can be pasted from a spreadsheet). Rows are kept in time order. **Two rows at the same time make a step.**
- **Graph:** drag points to move them, Shift+click to insert, Ctrl+click to delete. The graph and the table edit the same points.
- **Linear (default):** u(t) is interpolated exactly between the rows.
- **Smooth:** u(t) is a B-spline. The table then shows control points, which the curve passes through only at the first and last point.
- **Hold last value (default on):** after the last row, u keeps the last value until the end of the simulation. Unchecked, it returns to 0. **Before the first row, u = 0 (trim).**
- **Gain hint**, below the table: what u = 1 does to each surface, for example `Elevator flap_1: 1.000°`.

**u is in control units.** The deflection of each surface is:

    δ(t) = built-in foil flap angle + δ_trim + gain × u(t)

**Example**, a 3° trailing-edge-up step on the left elevator at 0.5 s, held (gain 1 °/unit):

| t (s) | u |
|---|---|
| 0 | 0 |
| 0.5 | 0 |
| 0.5 | −3 |

#### Graph variables
Each of the four graphs shows one variable. Change it in the graph's settings, via right-click → graph settings → Y variable. The defaults are the **perturbations from trim**:

| Graph | Longitudinal | Lateral |
|---|---|---|
| 1 | u (speed unit) | v (speed unit) |
| 2 | w (speed unit) | p (°/s) |
| 3 | q (°/s) | r (°/s) |
| 4 | θ (°) | φ (°) |

**(new) True flight values,** available on every graph:

| Longitudinal | Lateral |
|---|---|
| V — airspeed | β — sideslip |
| α — angle of attack | φ — bank angle |
| θ — pitch attitude | p body — roll rate, body axes (what a gyro measures) |
| γ — flight path angle | r body — yaw rate, body axes |
| q — pitch rate | ψ — heading change |
| n_z — load factor (1 at trim) | n_y — lateral load factor |
| Δh — height change | |

**Defaults for the forced response, longitudinal:** graphs 1–4 show V (with α on the right axis of graph 1), q (pitch rate), θ (pitch attitude) and n_z, i.e. true flight values rather than perturbations. For the other response types, no right axis is shown by default. The defaults are applied when you switch the response type or the direction. A variable you pick yourself is kept on other refreshes.

These are the trim values (V₀, α₀, θ₀ = α₀) plus the perturbations. n_z and n_y include the control input, and Δh and ψ are integrated over time.

**(new) Surface deflections:** `δ <wing> flap_k (°)` for every flap moved by the polar, either by the trim (Flaps tab) or by any control set. The value is the **total flap angle**, trailing edge down positive. For modal and initial-condition responses it stays constant at the trim value.

**(new) Right y-axis and fused grid:** each time graph can show a second variable on a right y-axis (graph settings → Variables → *Right y-axis*), e.g. α on the left and the elevator deflection on the right. With *Fuse grids* checked (the default; uncheck it per graph to get separate grids), a single grid is drawn: the right axis ticks sit on the left axis gridlines and are labelled with the right axis values at those positions (at most 2 decimals, or exponent notation for very small or large values). The right axis range is unchanged.

**(new) Message log on Add (forced response):** the control set and its effectiveness, the stability-axis inertia used, and the **initial angular accelerations per control unit** (q̇, ṗ, ṙ in °/s²). Open the log window to see it.

---

## 7. Identifying a failed analysis

### 7.1 In the log

| Message | Meaning | Fix |
|---|---|---|
| `Calculation not started: …` | Nothing to run: no active range row, no polar, external polar, … | Follow the message (§4) |
| `no zero-moment angle found` followed by `Unsuccessful attempt to trim the model for control position = … - skipping` | No angle between −45° and +45° gives Cm = 0 for that control value | Check the CoG position, the elevator trim deflection (Flaps tab) and the control parameter value. A CoG far forward with too little elevator authority, or far aft, causes this |
| `Found a negative lift for α=…` | Trim angle found, but lift ≤ 0, so no speed can balance the weight | Trimmed at a negative angle: the CoG or trim deflection is wrong for this control value |
| `Unsuccessful attempt to compute eigenvalues` | State matrix eigenvalue solve failed | Check mass and inertia (zero or negative values), and the derivative tables for NaN or absurd values |
| `Null mass - skipping calculation of eigenthings` | Mass is zero | Set the mass (Inertia tab or plane) |
| `Viscous interpolation warnings (values clamped)` | Foil polars do not cover the local Re or α; values were clamped | Extend the foil polars. Viscous drag and the viscous X_u are unreliable until fixed |
| `Downwash lag derivatives: no main wing or no elevator - skipping` | Option C enabled, but the wing types are missing | Set the wing types in the plane editor, or leave option C off for tailless planes |
| `WARNING: the wake sheet passes within one panel size of some control points …` (wake proximity check) | The main wing's wake sheet, laid flat along the body x-axis, passes very close to the control points of another surface (usually the tail). Thin-surface polars are the most affected | The tail's forces, Cma, Cmq and the control derivatives may be unreliable. The check lists each surface's minimum gap. Compare with a slightly higher or lower tail position: if the results change a lot for a few millimetres, they are dominated by the wake singularity, not by the physics |
| `WARNING: no tail station is usable - the downwash lag derivatives are not included in this run` | Panel method selected, but the whole tail lies in the wing's wake plane or the fuselage zone | Select DATCOM for this polar |
| `WARNING: less than half of the tail span is sampled …` | Panel method on a partly unusable tail | Compare with a DATCOM run; treat M_ẇ as uncertain |
| `WARNING: a sample point lies … from the wing's wake sheet …` | At this control value a frozen sample is very close to the wake sheet, e.g. after a flap moved the wing's trailing edge | Treat this point's M_ẇ as unreliable |
| `WARNING: the panel downwash gradient is outside the usual 0-0.8 range …` | Implausible panel result | Check the tail position and the wake; compare with DATCOM |
| `WARNING: …` in the inertia block | See §5.1 | Fix masses or inertia before trusting rates |

**A point that fails is skipped.** Check that the number of stored points equals the number of control values you asked for.

### 7.2 In the time response view

| Symptom | Cause | Fix |
|---|---|---|
| *Add* is greyed out | No T7 operating point selected | Run T7 with *Store operating points* checked and select a point |
| Forced dropdown shows *No AVL-type control set in the polar* | The polar has no control sets | Define one (§3.5) and re-run |
| `Forced response not computed: …` | The message names the missing prerequisite: points computed before the control sets existed, all gains zero, … | Re-run T7 after fixing the polar |
| A curve is empty on a flap-deflection graph | That flap was not active when the curve was computed | Re-add the curve |
| Curves stop early or explode to huge values | A divergent mode, or a time step too large for the fastest mode (§8.2) | Check the eigenvalues; reduce the total time or Δt |

---

## 8. Recognising potentially erroneous results

A successful run can still be significantly wrong. These are the error sources that can reach tens of percent or more.

### 8.1 Input data
1. **Control effectiveness.** Inviscid flap effectiveness is typically 20–50% too high on small aircraft. Every forced-response output scales with it. Set the effectiveness factor (§3.5); flight-test calibration is best.
2. **Inertia.** Initial rates scale with 1/I. Check the inertia block (§5.1). As a quick cross-check, the logged initial angular acceleration per control unit should equal (control moment per unit) / I.
3. **Propeller slipstream.** With a tractor propeller and the tail in the slipstream, tail effectiveness and damping can be 20–100% higher at cruise power than computed. Not modelled; calibrate with the effectiveness factor.

### 8.2 Numerics
- **Time step.** The step is max(total time / 1000, Δt). It must be well below the fastest time constant, which is usually roll subsidence: 1/|σ_roll|, often 0.05–0.2 s on models. A 60 s simulation has a 0.06 s step, which can be too coarse. Use separate short runs (2–5 s) for the fast response and long runs for the phugoid and spiral.
- **Linear in the input.** Doubling u doubles every perturbation. Large inputs (above about 10° of flap) are outside the validity of the 0.001-unit control derivative and of the flap's linear range.

### 8.3 Validity limits of the linear model

| Condition | Effect | Rule of thumb |
|---|---|---|
| Stall / large α | Not modelled | Do not trust α more than about ±5° from α₀ |
| Bank angle grows | The longitudinal model does not know about the bank: no extra load factor, no height loss, no turn coupling | Trust the output only while \|φ\| < 15–20°. Asymmetric inputs (e.g. one elevator) let φ keep growing through the spiral mode |
| Long simulations | Phugoid and spiral dominate; damping depends on drag (option B only partly helps, X_w viscous is not included) and on the unmodelled thrust | V, γ, Δh beyond several seconds: indicative only |
| Downwash lag off | Short-period overshoot in q, α, n_z overestimated | Enable option C for conventional tails |
| Glide trim (γ₀ ≠ 0) | Reference state is level flight | θ and Δh are off by γ₀ |
| Modal response | Arbitrary amplitude | Use only for the shape and timing of the motion |

### 8.4 Quick plausibility checks
- **Steady pull-up:** once q settles after an elevator step, **n_z ≈ 1 + V₀·q/g**.
- **First 0.1–0.3 s:** the slope of q(t), p(t) or r(t) should match the logged initial angular acceleration × u.
- **Steady roll rate:** for an aileron-like input, p settles within a few roll time constants (1/|σ_roll|), to roughly L_δ·δ / |L_p| from the derivative tables.
- **Deflection graph:** plot `δ … flap_k` and check that the surface does what you intended: sign, size, timing, held value.
- **Compare points:** results should change smoothly with the control parameter. A jump between neighbouring points usually signals a trim or viscous-interpolation problem at one of them.

---

## 9. File compatibility

- **(new)** The effectiveness factors (first 20 control sets) and the downwash-lag checkbox are stored in reserved slots of the plane polar record. Older files open with effectiveness 1.0 and option C off. Files saved with these settings still open in unmodified flow5, which ignores the new values.
- The XML polar export does not include the new settings.
- The forced input table, the interpolation mode and the hold option are application settings, not part of the project file.
