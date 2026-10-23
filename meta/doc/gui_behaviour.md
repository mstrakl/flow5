# GUI behaviour changes in the Axis fork

Changes to how the flow5 GUI behaves compared with upstream. These changes do not affect computed results.

## Deleting plane operating points

A polar's points are the results of its operating points. When an operating point is deleted, its point
in the polar is deleted too, so the polar graphs, the root locus and *Edit data points* no longer show it.
This applies to every way of deleting operating points:

- one operating point (explorer context menu, Delete key or the operating point menu);
- all the operating points of the current polar;
- all the operating points of the current plane;
- all the operating points of the project.

The point is matched on the variable that the polar type sweeps:

| Polar type | Matched on |
|---|---|
| T1, T2, T3 | α |
| T4 | V∞ |
| T5 | β |
| T6, T7 | the control value |
| T8 | α, β and V∞ |

Polar points that have no stored operating point are kept, for example points computed with
"store operating points" turned off. Use *Reset polar* to delete all the polar's data.

Objects cannot be deleted or reset while an analysis is running; flow5 shows a message instead.
