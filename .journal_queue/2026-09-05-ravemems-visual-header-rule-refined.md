# RAVEMEMS V2 visual header rule refined from real source pages

## Context
After inventorying the user-supplied `rave(6).zip`, real PDF pages were inspected before any RAVEMEMS V2 code change.

## Confirmed RCL0193ENG behavior
In `rave/xn/wmxn990e.pdf` (Workshop Manual RCL0193ENG), physical PDF page 53 contains:
- the repeated ENGINE header icon at approximately y=17.7..58.0 pt;
- the main horizontal header separator around y=57..58 pt;
- the genuine primary-gear technical illustration much lower on the page, source rectangle approximately x=63.9..252.0, y=210.6..502.5 pt.

This confirms that the workshop-manual header/icon must be excluded from IMAGE capture only. Text extraction remains unchanged and continues to read the full page.

## Important corpus-wide exception
A blanket top-of-page image exclusion cannot be applied to all 47 source PDFs.

Example: `rave/Mini Tech Bulletins/R3591BU.PDF`, page 5, contains genuine technical illustrations beginning near y=35 pt and occupying the top half of the page. Rejecting all image candidates in a fixed top band would destroy valid technical diagrams.

Therefore the image-header exclusion must be document/layout aware:
- for RAVE manual pages with the standard repeated header/icon layout, reject header visuals before capture and prevent crop margin from re-entering the header;
- for technical bulletins/pages without that standard header, do not reject a genuine large technical illustration merely because it starts high on the page.

## Crop margin finding
Current RAVEMEMS V2 code uses 24 pt on left/top/right and 32.4 pt below (`margin * 1.35`). This is excessive.

On the RCL0193ENG p53 reference illustration:
- 24 pt includes neighboring procedure text above/below;
- 8 pt keeps the complete technical drawing and its figure identifier without neighboring procedure lines;
- 4 pt also keeps the complete drawing on this reference page.

No final global margin value is fixed from one page alone. The next pilot must test a small representative set before choosing the generic margin or adaptive rule.

## Code status
No RAVEMEMS V2 technical correction has been pushed yet. Current audited source remains `tmp-ravemems-v2-language-fix` commit `4942f1e7bd11bddf3c0f4cf9bcc5cbe0b6e11c4d`.

## Next exact action
Create a new isolated RAVEMEMS V2 foundation branch from the audited V2 engine, implement only the pilot visual-capture correction for RCL0193ENG first, and test it against real pages before extending rules to the rest of the corpus. Do not rebuild SQLite/DLL/MEMS Manager yet.
