# RAVE source inventory from user-supplied rave(6).zip

## Scope
Inventory only. No RAVEMEMS code change, no SQLite rebuild, no DLL rebuild, no MEMS Manager change.

## Source result
The user-supplied archive `rave(6).zip` contains exactly 47 PDF files totaling 1,359 pages.

Breakdown:
- `rave/Mini Tech Bulletins`: 35 PDFs / 73 pages
- `rave/general/testbook`: 2 PDFs / 216 pages
- `rave/library`: 1 PDF / 1 page
- `rave/xn`: 9 PDFs / 1,069 pages

## Language finding
The corpus is not five full copies of the same manuals in five languages. The PDFs are overwhelmingly English technical documents. Several cover pages print document titles in multiple languages, but their body content is English.

Examples:
- `wmxn990e.pdf`: MINI Workshop Manual, publication RCL0193ENG, English body text.
- `cdxn990e.pdf`: Electrical Circuit Diagrams, RCL0194ENG, English body text.
- `roxn990e.pdf`: Repair Operation Times, RCL0195ENG, English body text.
- `elxn970e.pdf`: Electrical Library, RCL0213ENG, English body text.
- `tb12212e.pdf`: TestBook 1 User Manual, RCL0221ENG, English body text.
- `tb22382e.pdf`: TestBook 2 User Manual, RCL0238ENG, English body text.
- `hr01795e.pdf`: Owner's Handbook, RCL0179ENG, English body text.
- `Hr01791x.pdf`: separate RCL0179ENX handbook variant/edition, English body text; not merely a French/German/etc duplicate.

Therefore the new extraction must NOT multiply the technical extraction by application output language. Technical content and images are extracted once from the actual source document; final answer language is handled later.

## Duplicate finding
No exact duplicate PDFs/text bodies were found in the 47-file source set. Apparent same-topic files can be distinct technical publications and must not be collapsed solely by title. Example: `R6255BU.PDF` and `X6255BU.PDF` both concern wiring harness connectors but are different bulletins with different dates/scope/content.

## Proposed corpus classification
### Keep as technical/source knowledge
All substantive vehicle/service/diagnostic documents, including:
- Workshop Manual RCL0193ENG (`wmxn990e.pdf`)
- Electrical Circuit Diagrams RCL0194ENG (`cdxn990e.pdf`)
- Repair Operation Times RCL0195ENG (`roxn990e.pdf`)
- Electrical Library RCL0213ENG (`elxn970e.pdf`)
- Maintenance Check Sheet RCL0225ENG (`mcxn960e.pdf`)
- Paint Refinishing Times combined English publication (`prxn990e.pdf`)
- TestBook manuals RCL0221ENG and RCL0238ENG
- both owner-handbook variants RCL0179ENG / RCL0179ENX
- In-Car Entertainment manual (English)
- substantive Mini Technical Bulletins

### Exclude from the vehicle-answer knowledge index
These files are RAVE/CD/navigation/support metadata rather than useful vehicle technical procedures:
- `rave/library/libxn.pdf`: blank one-page Mini Library shell
- `rave/Mini Tech Bulletins/Xndb2eng.pdf`: one-page multilingual Technical Bulletins menu/index
- `rave/Mini Tech Bulletins/X8386BU.PDF`: RAVE Technical Bulletins CD introduction
- `rave/Mini Tech Bulletins/X8408BU.PDF`: RAVE Technical Bulletins CD version information
- `rave/Mini Tech Bulletins/X8417BU.PDF`: maintaining RAVE CD updates
- `rave/Mini Tech Bulletins/RT0003BU.PDF`: TestBook Help Desk reporting/support-process bulletin; not vehicle repair knowledge

This yields 41 substantive PDFs proposed for technical extraction and 6 metadata/support PDFs excluded from the end-user technical answer corpus.

## Consequence for the revised plan
1. The language-duplication concern is resolved at source: this RAVE archive does not contain five complete language copies to extract repeatedly.
2. The technical extraction should operate once on the substantive English source set.
3. RAVEMEMS V2 must next be corrected before any new full extraction: image-header exclusion only for image capture, reduced crop margin, correct vector grouping, faithful text/procedure extraction.
4. Start validation with `wmxn990e.pdf` / RCL0193ENG, including the known primary-gear end-float page as a mandatory reference case.
5. No SQLite or DLL rebuild until the corrected extraction and images are validated.

## Production state
MEMS Manager production remains frozen. No BUILD #106 is authorized or launched. Application reintegration baseline remains BUILD #101 after the documentary stack is rebuilt and validated independently.
