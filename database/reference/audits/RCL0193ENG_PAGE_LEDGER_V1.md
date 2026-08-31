# RCL0193ENG PAGE LEDGER V1

- Exact source: `rave/xn/wmxn990e.pdf`
- Publication: `RCL0193ENG` Mini Workshop Manual
- Physical pages inventoried: **372 / 372**
- This V1 ledger is a structural/textual inventory plus existing-asset mapping. It is **not yet the final visual completeness certificate**; separator/blank and illustrated pages still require systematic visual confirmation before final disposition.

## Section coverage by physical page

| Physical pages | Section | Current disposition |
|---|---|---|
| 1-8 | Front matter / amendments | Useful context except blank separators |
| 9-14 | Introduction | Useful context/technical except blank separators |
| 15-50 | General Information | Useful technical/context except blank separators |
| 51-96 | Engine | Useful technical/context except blank separators |
| 97-104 | Emission Control | Useful technical/context except blank separator |
| 105-136 | Engine Management System - MEMS | Useful technical/context except blank separators |
| 137-146 | Fuel Delivery System | Useful technical/context except blank separators |
| 147-160 | Cooling System | Useful technical/context except blank separators |
| 161-176 | Manifold & Exhaust Systems | Useful technical/context except blank separators |
| 177-192 | Clutch | Useful technical/context except blank separators |
| 193-202 | Manual Gearbox | Useful technical/context except blank separators |
| 203-210 | Drive Shafts | Useful technical/context except blank separator |
| 211-226 | Steering | Useful technical/context except blank separators |
| 227-234 | Suspension | Useful technical/context except blank separators |
| 235-256 | Brakes | Useful technical/context except blank separators |
| 257-280 | Restraint Systems | Useful technical/context except blank separators |
| 281-304 | Body | Useful technical/context except blank separators |
| 305-314 | Heating & Ventilation | Useful technical/context except blank separator |
| 315-324 | Wipers & Washers | Useful technical/context except blank separator |
| 325-358 | Electrical | Useful technical/context except blank separators |
| 359-372 | Instruments | Useful technical/context except blank separator |

## Preliminary page disposition counts

- Useful technical pages: **299**
- Useful context/index/amendment pages: **27**
- Candidate blank/separator pages: **46**
- Total: **372**

Candidate blank/separator physical pages:

`2, 8, 10, 14, 16, 18, 30, 34, 42, 48, 52, 98, 106, 120, 138, 140, 146, 148, 152, 154, 162, 176, 178, 192, 194, 202, 204, 212, 214, 228, 230, 234, 236, 256, 258, 262, 274, 280, 282, 298, 304, 306, 316, 326, 328, 360`

These pages have zero extractable text in the source and appear structurally as separator pages. They remain `needs_visual_confirmation` until visual inspection; they are not silently discarded.

## Existing BUILD #103 RCL0193ENG asset coverage

26 existing assets map exactly to physical source pages:

`39, 40, 41, 99, 102, 108, 109, 110, 113, 114, 115, 118, 119, 121, 122, 123, 124, 126, 127, 128, 129, 130, 131, 132, 136, 171`

Their legacy filename tokens are one less than the exact physical page. See `RCL0193ENG_EXISTING_ASSET_REMAP_V1.md`.

## Extraction rule for the next pass

1. Every non-separator page is in scope for extraction because it contains vehicle/service/repair/diagnostic/specification/context information useful to the final user.
2. Tables and numerical specifications are extracted as structured data, not merely kept as page images.
3. Procedures are extracted into procedure/step structures with exact physical-page traceability.
4. Technical views, component drawings, layouts and diagrams are extracted as visual assets; page images are not the final user-facing resource.
5. Text embedded in a useful visual is represented by multilingual visual labels rather than by generating six copies of the image.
6. Existing correct BUILD #103 facts/assets are linked/reused and not blindly duplicated.
7. Final completion requires a visual pass over all 372 pages and an explicit disposition for every page/section.
