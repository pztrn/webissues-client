/**************************************************************************
* This file is part of the WebIssues Desktop Client program
* Copyright (C) 2006 Michał Męciński
* Copyright (C) 2007-2012 WebIssues Team
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef MARKUPTEXTEDIT_H
#define MARKUPTEXTEDIT_H

#include "data/datamanager.h"

#include <QWidget>

class InputTextEdit;

class QComboBox;

class MarkupTextEdit : public QWidget
{
    Q_OBJECT
public:
    MarkupTextEdit( QWidget* parent );
    ~MarkupTextEdit();

public:
    void setInputValue( const QString& value );
    QString inputValue();

    void setTextFormat( TextFormat format );
    TextFormat textFormat() const;

    bool validate();

    void setRequired( bool required );
    bool isRequired() const;

private:
    InputTextEdit* m_edit;

    QComboBox* m_comboBox;
};

#endif