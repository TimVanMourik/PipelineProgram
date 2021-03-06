/*  Copyright (C) Tim van Mourik, 2017, DCCN
    All rights reserved

 This file is part of the Porcupine pipeline tool, see
 https://github.com/TimVanMourik/Porcupine for the documentation and
 details.

    This toolbox is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This toolbox is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the fmri analysis toolbox. If not, see
    <http://www.gnu.org/licenses/>.
*/

#include <QCheckBox>
#include <QLineEdit>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

#include "Argument.hpp"
#include "PortBlock.hpp"
#include "PortPair.hpp"
#include "PortRow.hpp"

PortRow::PortRow(
        PortPair* _port,
        PortBlock* _parent
        ) :
    QWidget(),
    m_parent(_parent),
    m_port(_port),
    m_parameterName(new QLineEdit()),
    m_showCheckbox(new QCheckBox()),
    m_iterateCheckbox(0),
    m_deleteButton(new QPushButton())
{
    m_parameterName->setPlaceholderText("<value>");
    m_parameterName->setAlignment(Qt::AlignLeft);
    QString name = _port->getFileName();
    if(!name.isEmpty()) m_parameterName->setText(name);
    QHBoxLayout* rowLayout = new QHBoxLayout(this);
    rowLayout->addWidget(m_parameterName);
    rowLayout->addWidget(m_showCheckbox);
    if(_port->getInputPort())
    {
        m_iterateCheckbox = new QCheckBox();
        m_iterateCheckbox->setChecked(_port->getArgument().m_isIterator);
        rowLayout->addWidget(m_iterateCheckbox);
        connect(m_iterateCheckbox, SIGNAL(toggled(bool)), this, SLOT(iteratePort(bool)));
    }
    rowLayout->addWidget(m_deleteButton);

    m_parameterName->setEnabled(_port->getArgument().m_isEditable && _port->getArgument().m_isVisible);
    m_showCheckbox-> setChecked(_port->getArgument().m_isVisible);

    connect(m_parameterName, SIGNAL(textEdited(QString)),     _port,           SLOT(fileNameChanged(QString)));
    connect(m_showCheckbox,  SIGNAL(toggled(bool)),           this ,           SLOT(showPort(bool)));
    connect(m_deleteButton,  SIGNAL(clicked(bool)),           this,            SLOT(removePort()));
//    connect(_port,           SIGNAL(setConnected(bool)),      m_showCheckbox,  SLOT(setDisabled(bool)));
    connect(_port,           SIGNAL(setConnected(bool)),      this,            SLOT(portConnected(bool)));
    connect(_port,           SIGNAL(changeFileName(QString)), m_parameterName, SLOT(setText(QString)));

    initialiseStyleSheets();
}

void PortRow::removePort(
        )
{
//    QMessageBox::StandardButton reply = QMessageBox::question(this, QString("Are you sure?"), QString(), QMessageBox::Yes | QMessageBox::No);
    QMessageBox message;
//    message.setIconPixmap(QPixmap(":/images/porcupine-sad.png"));
    message.setIcon(QMessageBox::Question);
    message.setText("Are you sure you want to remove this port?");
    message.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    message.setDefaultButton(QMessageBox::Yes);
//    QVBoxLayout* messageLayout = new QVBoxLayout();

//    QWidget* w = new QWidget();
//    QPalette palette;
//    palette.setBrush(w->backgroundRole(), QBrush(QImage(":/images/porcupine-sad.png")));
//    qDebug() << message.layout()->itemAt(0);

    switch(message.exec())
    {
    case QMessageBox::Yes :
        break;
    case QMessageBox::No :
        return;
    default :
        return;
    }

    m_port->removePort();
    disconnect(m_showCheckbox,  SIGNAL(toggled(bool)), this, SLOT(showPort(bool)));
    disconnect(m_deleteButton,  SIGNAL(toggled(bool)), this, SLOT(removePort()));
    m_parent->removePortRow(this);

}

void PortRow::iteratePort(
        bool _iterator
        )
{
    m_port->setAsIterator(_iterator);
}

void PortRow::showPort(
        bool _visible
        )
{
    if(_visible)
    {
        m_parameterName->setEnabled(m_port->getArgument().m_isEditable);
    }
    else
    {
        m_parameterName->setEnabled(false);
    }

    m_port->setVisibility(_visible);
    if(!_visible && m_iterateCheckbox)
    {
        m_iterateCheckbox->setChecked(false);
        iteratePort(false);
    }
}

void PortRow::saveToJson(
        QJsonObject& o_json
        )
{
    o_json = m_port->toJson();
    o_json["value"]      = m_parameterName->text();
    o_json["inputPort"]  = QString::number((quint64) m_port->getInputPort(), 16);
    o_json["outputPort"] = QString::number((quint64) m_port->getOutputPort(), 16);
}

QString PortRow::getParameterName(
        ) const
{
    return m_parameterName->text();
}


void PortRow::initialiseStyleSheets(
        )
{
    QFile fileVisibility(":/qss/visibilityButton.qss");
    fileVisibility.open(QFile::ReadOnly);
    QString styleSheetVisibility = QString::fromLatin1(fileVisibility.readAll());
    m_showCheckbox->setStyleSheet(styleSheetVisibility);

    if(m_iterateCheckbox)
    {
        QFile fileIterator(":/qss/iteratorButton.qss");
        fileIterator.open(QFile::ReadOnly);
        QString styleSheetIterator = QString::fromLatin1(fileIterator.readAll());
        m_iterateCheckbox->setStyleSheet(styleSheetIterator);
    }

    QFile fileRemove(":/qss/removeButton.qss");
    fileRemove.open(QFile::ReadOnly);
    QString styleSheetRemove = QString::fromLatin1(fileRemove.readAll());
    m_deleteButton->setStyleSheet(styleSheetRemove);
}

void PortRow::portConnected(
        bool _connected
        )
{
    if(_connected)
    {
        m_showCheckbox->setDisabled(true);
        if(m_port->getArgument().m_isOutput && !m_port->getArgument().m_isInput)
        {
            m_parameterName->setDisabled(true);
        }
    }
    else
    {
        m_showCheckbox->setDisabled(false);
        m_parameterName->setEnabled(m_port->getArgument().m_isEditable);
    }
}

PortRow::~PortRow()
{
//    delete m_parameterName;
//    delete m_showCheckbox;
//    delete m_iterateCheckbox;
//    delete m_deleteButton;
}
