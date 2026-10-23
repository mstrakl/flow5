# GUI behaviour changes in the Axis fork

Changes to how the flow5 GUI behaves compared with upstream. These changes do not affect computed results.

## Deleting plane operating points

- **Delete one operating point** (explorer context menu, Delete key or the operating point menu): the
  matching point of its polar is deleted too, so the polar graphs no longer show it. The point is matched
  on the variable that the polar type sweeps: α for T1/T2/T3, V∞ for T4, β for T5, the control value
  for T6/T7, and α, β and V∞ for T8.
- **Delete all operating points of a polar, of a plane, or of the project**: the polar data is kept, as in
  upstream. These actions only remove the stored operating points, for example to make the project
  file smaller. Use *Reset polar* to delete the polar data as well.
