/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2006
 *
 * Author(s):
 *	Volker Fischer, Andrea Russo, David Flamand
 *
 * Description:
 *
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#ifndef __UTIL_QT_UTIL_H
#define __UTIL_QT_UTIL_H

#include <QString>
#ifdef QT_GUI_LIB
# include "../GUI-QT/MultColorLED.h"
# include <QTreeWidget>
#endif
#include "../Parameter.h"

class CDRMTransceiver;
class CService;

QString VerifyFilename(QString filename);

QString VerifyHtmlPath(QString path);

QString UrlEncodePath(QString url);

bool IsUrlDirectory(QString url);

QString& Linkify(QString& text, QString linkColor=QString());

void CreateDirectories(const QString& strFilename);

void RestartTransceiver(CDRMTransceiver *DRMTransceiver);

QString	GetCodecString(const CService&);
QString	GetTypeString(const CService&);
QString GetDataTypeString(const CService&);

QString getAMScheduleUrl();

#ifdef QT_GUI_LIB
void SetStatus(CMultColorLED*, ETypeRxStatus);
void ColumnParamFromStr(QTreeWidget* treeWidget, const QString& strColumnParam);
void ColumnParamToStr(QTreeWidget* treeWidget, QString& strColumnParam);
#endif

#endif // __UTIL_QT_UTIL_H
