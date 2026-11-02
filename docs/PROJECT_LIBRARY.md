# Project Library / Reaction Pathway

The **프로젝트** book icon in YoungCraft's top toolbar (shortcut **Ctrl+Shift+P**) opens the project library on demand. The ordinary startup and molecular workspace remain unchanged.

## Workflow

1. Click **프로젝트** and choose **새 프로젝트**. Save a \`.ycproject\` file in the research folder.
2. Use **계산 파일 추가** to import Gaussian \`.out\`/\`.log\` files, or **폴더에서 가져오기** to recursively import up to 500 output files.
3. Edit species names, set each role (Reactant/TS/Intermediate/Product), and reorder the pathway with ▲ and ▼.
4. Choose **기준점 지정** to define the relative energy zero. Toggle **Electronic (SCF)** and **Gibbs (frequency)** as available.
5. Click a point on the plot to select its species, or choose **3D로 열기** to inspect the source calculation in YoungCraft.
6. Export a CSV table for manuscript or Supporting Information work.

Projects appear in the library on future runs. The \`.ycproject\` file stores file references, roles, order, the reference point, and cached energies. File references are relative to the project file when possible, making the project directory easier to relocate. **목록에서 제거** only unregisters a project; it never deletes source calculations or project data.

## Energy interpretation

The pathway uses the final Gaussian \`SCF Done\` energy or \`Sum of electronic and thermal Free Energies\` value in each imported output. Relative energies are computed with 627.509474 kcal mol⁻¹ per Hartree. An unavailable value is shown as “—”; no estimated correction is inserted. Users must ensure the compared calculations have compatible levels of theory and standard states. Frequency-derived Gibbs values are not automatically combined with higher-basis single-point energies. This initial version deliberately keeps those corrections separate.

The project library is groundwork for broader SI Generator integration, including links to structure images, method tables, and atom-mapped geometry comparisons.
