# Compiling the AI Collaboration Reflection Document

This directory contains the reflective analysis document in both Markdown and LaTeX formats.

## Files

- `ai_collaboration_reflection.md` - Markdown version (47KB, human-readable)
- `ai_collaboration_reflection.tex` - LaTeX version (60KB, for professional typesetting)

## Compiling the LaTeX Document

### Requirements

You'll need a LaTeX distribution installed:

- **Linux**: Install `texlive-full`
  ```bash
  sudo apt-get install texlive-full
  ```

- **macOS**: Install [MacTeX](https://www.tug.org/mactex/)
  ```bash
  brew install --cask mactex
  ```

- **Windows**: Install [MiKTeX](https://miktex.org/) or [TeX Live](https://www.tug.org/texlive/)

### Compilation Commands

#### Basic compilation:
```bash
pdflatex ai_collaboration_reflection.tex
```

#### For complete compilation (with table of contents):
```bash
pdflatex ai_collaboration_reflection.tex
pdflatex ai_collaboration_reflection.tex
```
(Run twice to ensure all cross-references and TOC are correct)

#### Using latexmk (recommended for automatic compilation):
```bash
latexmk -pdf ai_collaboration_reflection.tex
```

### Output

The compilation will generate `ai_collaboration_reflection.pdf` with:
- Professional typesetting and formatting
- Table of contents with hyperlinks
- Properly formatted tables and code listings
- Academic paper styling

### Cleaning Build Artifacts

The `.gitignore` file is configured to exclude LaTeX build artifacts (`.aux`, `.out`, `.toc`, etc.) from version control.

To manually clean build files:
```bash
# Using latexmk
latexmk -c ai_collaboration_reflection.tex

# Or manually
rm *.aux *.out *.toc *.log *.synctex.gz *.fls *.fdb_latexmk
```

## Document Features

The LaTeX version includes:
- Title page with project metadata
- Automatic table of contents generation
- Section numbering and cross-referencing
- Professional table formatting with `booktabs`
- Syntax-highlighted code blocks with `listings`
- Hyperlinked references
- Academic paper styling (12pt, A4 paper)
- Header/footer with document title and page numbers

## Viewing Options

- **PDF Viewer**: Open the generated PDF in any PDF viewer
- **Overleaf**: Upload the `.tex` file to [Overleaf](https://www.overleaf.com/) for online editing and compilation
- **VS Code**: Use the [LaTeX Workshop](https://marketplace.visualstudio.com/items?itemName=James-Yu.latex-workshop) extension for integrated editing and compilation
