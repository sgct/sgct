del /s /f *.ps *.dvi *.aux *.toc *.idx *.ind *.ilg *.log *.out *.brf *.blg *.bbl refman.pdf

pdflatex refman
makeindex refman.idx
pdflatex refman
