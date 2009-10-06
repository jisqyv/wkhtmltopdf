//-*- mode: c++; tab-width: 4; indent-tabs-mode: t; c-file-style: "stroustrup"; -*-
// This file is part of wkhtmltopdf.
//
// wkhtmltopdf is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// wkhtmltopdf is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with wkhtmltopdf.  If not, see <http://www.gnu.org/licenses/>.
#include "outline_p.hh"
#ifdef __EXTENSIVE_WKHTMLTOPDF_QT_HACK__

/*!
  \file outline_p.hh
  \brief Defines the classes OutlinePrivate and OutlineItem
*/

/*!
  \class OutlineItem
  \brief Class describing an item in the outline
*/

/*!
  \brief Recursivily delete the subtree
*/
OutlineItem::~OutlineItem() {
	foreach (OutlineItem * i, children)
		delete i;
}

/*!
  \class OutlinePrivate
  \brief Class providing implemenation details of Outline
*/

OutlinePrivate::OutlinePrivate(const Settings & s):
	settings(s) {
}

OutlinePrivate::~OutlinePrivate() {
	foreach (OutlineItem * i, documentOutlines) 
		delete i;
}

void OutlinePrivate::fillChildAnchors(OutlineItem * item, QHash<QString, QWebElement> & anchors) {
	foreach (OutlineItem * i, item->children) {
		if (i->anchor.isEmpty()) continue;
		anchors[i->anchor] = i->element;
		fillChildAnchors(item, anchors);
	}	
}

void OutlinePrivate::outlineChildren(OutlineItem * item, QPrinter * printer, int level) {
	if (level + 1 > settings.outlineDepth) return;
	foreach (OutlineItem * i, item->children) {
		printer->beginSectionOutline(i->value, i->anchor);
		outlineChildren(i, printer, level+1);
		printer->endSectionOutline();
	}	
}


/*!
  \file outline.hh
  \brief Defiens the Outline class
*/

/*!
  \class Outline
  \brief Class responcible for building and keeping an outline of a document.
*/

/*!
  \brief Construct a new outline class
  \param settings The settings to use
*/
Outline::Outline(const Settings & settings): d(new OutlinePrivate(settings)) {}
Outline::~Outline() {delete d;}

/*!
  \brief Add a new webpage to the outline
  \param name The name of the webpage
  \param wp A webprinter for the page
  \param frame The frame containing the webpage
*/
void Outline::addWebPage(const QString & name, QWebPrinter & wp, QWebFrame * frame) {
	QMap< QPair<int, QPair<qreal,qreal> >, QWebElement> headings;
	
	foreach(const QWebElement & e, frame->findAllElements("h1,h2,h3,h4,h5,h6,h7,h8,h9")) {
		QPair<int, QRectF> location = wp.elementLocation(e);
		headings[ qMakePair(location.first, qMakePair(location.second.y(), location.second.x()) ) ] = e;
	}

	//This huristic is a little strange, it tries to create a real tree,
	//even though someone puts a h5 below a h1 or stuff like that
	//The way this is handled is having a level stack, indicating what h-tags
	//a level level in the tree currently represents
	QVector<uint> levelStack;
	levelStack.push_back(0);
	OutlineItem * root = new OutlineItem();
	OutlineItem * old = root;
	for(QMap< QPair<int, QPair<qreal,qreal> >, QWebElement>::iterator i = headings.begin(); i != headings.end(); ++i) {
		const QWebElement & element = i.value();
		
		uint level = element.tagName().mid(1).toInt();
		OutlineItem * item = new OutlineItem();
		item->page = d->pageCount + i.key().first;
		item->value = element.toPlainText();
		item->element = element;
		item->anchor = QString("__WKANCHOR_")+QString::number(d->anchorCounter++,36);
		while(levelStack.back() >= level) {
			old = old->parent;
			levelStack.pop_back();
		}
		item->parent = old;
		old->children.push_back(item);
		old = item;
		levelStack.push_back(level);
	}
	d->pageCount += wp.pageCount();
}

/*!
  \brief Fill in header footer parameters for a given page
  \param page The page to fill in for
  \param parms The structure to fill
 */
void Outline::fillHeaderFooterParms(int page, QHash<QString, QString> & parms) {
	int off = d->settings.pageOffset;
	parms["frompage"] = QString::number(off);
	parms["topage"] = QString::number(off+d->pageCount-1);
	parms["page" ] = QString::number(page+off);
	parms["webpage"] = "foobar";

//  	QString sec[TocPrinter::levels];
// 	for (uint i=0; i < TocPrinter::levels; ++i) {
// 		QMap<int, TocItem*>::const_iterator j = tocPrinter.page2sectionslow[i].find(pageNum);
// 		if (j == tocPrinter.page2sectionslow[i].end()) {
// 			j = tocPrinter.page2sectionshigh[i].upperBound(pageNum);
// 			--j;
// 			if (j == tocPrinter.page2sectionshigh[i].end()) continue;
// 		}
// 		sec[i] = j.value()->value;
// 	}
// 	res["section"] = sec[0];
// 	res["subsection"] = sec[1];
// 	res["subsubsection"] = sec[2];
}

/*!
  \brief Fill in anchor as to add to a given document
  \param doc The 0 indexed document number (in order of addWebPage)
  \param anchors The structure to fill
*/
void Outline::fillAnchors(int doc, QHash<QString, QWebElement> & anchors) {
  if (doc < 0 || doc >= d->documentOutlines.size()) return;
  d->fillChildAnchors( d->documentOutlines[doc], anchors );
}

/*!
  \brief return the number of pages in the outlined document
*/
int Outline::pageCount() {
	return d->pageCount;
}

/*!
  \brief Print the document outline to a given printer
  \param printer The printer to print to
*/
void Outline::printOutline(QPrinter * printer) {
	if (!d->settings.outline) return;
	foreach(OutlineItem * i, d->documentOutlines)
		d->outlineChildren(i, printer, 0);
}
#endif //__EXTENSIVE_WKHTMLTOPDF_QT_HACK__