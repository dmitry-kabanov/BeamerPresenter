/*
 * This file is part of BeamerPresenter.
 * Copyright (C) 2019  stiglers-eponym

 * BeamerPresenter is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * BeamerPresenter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with BeamerPresenter. If not, see <https://www.gnu.org/licenses/>.
 */

#include "pdfdoc.h"

PdfDoc::PdfDoc(QString pathToPdf)
{
    pdfPath = pathToPdf;
}

PdfDoc::~PdfDoc()
{
    qDeleteAll(pdfPages);
    pdfPages.clear();
    labels.clear();
    delete popplerDoc;
}

Poppler::Document const * PdfDoc::getDoc() const
{
    return popplerDoc;
}

void PdfDoc::loadDocument()
{
    if (popplerDoc != nullptr) {
        qWarning() << "WARNING: A document has already been loaded. Deleting it in order to avoid memory leaks.";
        delete popplerDoc;
    }
    popplerDoc = Poppler::Document::load(pdfPath);
    if (popplerDoc == nullptr)
        return;
    if (popplerDoc->isLocked()) {
        // TODO: use a nicer way of entering passwords (a QDialog?)
        std::cout << "WARNING: File " << pdfPath.toStdString() << ":\n"
                  << "This file is locked. Support for locked files is HIGHLY EXPERIMENTAL!" << std::endl
                  << "You can try to enter your password here.\n"
                  << "YOUR PASSWORD WILL BE VISIBLE IF YOU ENTER IT HERE!" << std::endl;
        std::string ownerPassword;
        std::string userPassword;
        std::cout << "Owner password (NOT HIDDEN!): ";
        std::cin >> ownerPassword;
        std::cout << "User password (NOT HIDDEN!): ";
        std::cin >> userPassword;
        popplerDoc->unlock(QByteArray::fromStdString(ownerPassword), QByteArray::fromStdString(userPassword));
    }
    popplerDoc->setRenderHint(Poppler::Document::TextAntialiasing);
    popplerDoc->setRenderHint(Poppler::Document::TextHinting);
    popplerDoc->setRenderHint(Poppler::Document::ThinLineShape);
    popplerDoc->setRenderHint(Poppler::Document::Antialiasing);

    qDeleteAll(pdfPages);
    pdfPages.clear();
    labels.clear();
    for (int i=0; i < popplerDoc->numPages(); i++) {
        Poppler::Page* p = popplerDoc->page(i);
        pdfPages.append(p);
        labels.append(p->label());
    }

    if (popplerDoc->hasOptionalContent())
        qWarning() << "This file has optional content. Optional content is not supported.";
    if (popplerDoc->hasEmbeddedFiles())
        qWarning() << "This file contains embedded files. Embedded files are not supported.";
    if (popplerDoc->scripts().size() != 0)
        qWarning() << "This file contains JavaScript scripts. JavaScript is not supported.";
}

QSize PdfDoc::getPageSize(int const pageNumber) const
{
    if (pageNumber < 0)
        return pdfPages[0]->pageSize();
    if (pageNumber >= popplerDoc->numPages())
        return pdfPages[popplerDoc->numPages()-1]->pageSize();
    return pdfPages[pageNumber]->pageSize();
}

Poppler::Page * PdfDoc::getPage(int pageNumber) const
{
    if (pageNumber < 0)
        return pdfPages[0];
    if (pageNumber >= popplerDoc->numPages())
        return pdfPages[popplerDoc->numPages()-1];
    return pdfPages[pageNumber];
}

int PdfDoc::getNextSlideIndex(int const index) const
{
    QString label = labels[index];
    for (int i=index; i<popplerDoc->numPages(); i++) {
        if (label != labels[i])
            return i;
    }
    return popplerDoc->numPages()-1;
}

int PdfDoc::getPreviousSlideEnd(int const index) const
{
    QString label = labels[index];
    for (int i=index; i>0; i--) {
        if (label != labels[i]) {
            double duration = pdfPages[i]->duration();
            int j=i;
            // Don't return the index of a slides which is shown for less than one second
            while (duration > -0.01 && duration < 1. && (labels[j] == labels[i]))
                duration = pdfPages[--j]->duration();
            return j;
        }
    }
    return 0;
}

void PdfDoc::setPagePart(int const state)
{
    pagePart = state;
}
