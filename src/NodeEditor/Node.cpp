/*
 * Copyright (C) 2014 Tim van Mourik
*/

#include <assert.h>
#include <iostream>

#include <QDomDocument>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QLinearGradient>

#include "Argument.hpp"
#include "Node.hpp"
#include "NodeEditor.hpp"
#include "NodeLibrary.hpp"
#include "NodeSetting.hpp"
#include "Preferences.hpp"
#include "PortPair.hpp"

qreal Node::s_horizontalMargin  = 5;
qreal Node::s_verticalMargin    = 3;
qreal Node::s_textSpacing       = 1;

Node::Node(
        NodeEditor* _editor,
        NodeSetting* _setting
        ) :
    QGraphicsPathItem(0),
    m_setting(_setting),
    m_nameLabel(new QGraphicsTextItem(this)),
    m_width(2 * s_horizontalMargin),
    m_height(2 * s_verticalMargin)
{
    _editor->scene()->addItem(this);
    QPainterPath p;
    p.addRect(0, -m_height / 2, m_width, m_height);
    setPath(p);
    setPen(QPen(Qt::darkRed));
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    loadFromNodeSetting(_setting);
}

void Node::loadFromNodeSetting(
        NodeSetting* _setting
        )
{
    if(_setting)
    {
        m_setting = _setting;
        Argument argument(_setting->getName());
        setName(argument);

        addInputPorts(_setting->getInput());
        addInOutPorts(_setting->getInOut());
        addOutputPorts(_setting->getOutput());
    }
}

void Node::addInputPorts(
        const QVector<Argument>& names
        )
{
    foreach(const Argument& argument, names)
    {
        addInputPort(argument);
    }
}

void Node::addInOutPorts(
        const QVector<Argument>& names
        )
{
    foreach(const Argument& argument, names)
    {
        addInOutPort(argument);
    }
}

void Node::addOutputPorts(
        const QVector<Argument>& names
        )
{
    foreach(const Argument& argument, names)
    {
        addOutputPort(argument);
    }
}

void Node::addInputPort(
        const Argument& _argument
        )
{
    addPortPair(_argument, "i");
}

void Node::addInOutPort(
        const Argument& _argument
        )
{
    addPortPair(_argument, "io");
}

void Node::addOutputPort(
        const Argument& _argument
        )
{
    addPortPair(_argument, "o");
}

void Node::setName(
        const Argument& _argument
        )
{
    Preferences& preferences = Preferences::getInstance();

    QFont font(scene()->font());
    font.setBold(true);
    m_nameLabel->setFont(font);
    m_nameLabel->setPlainText(_argument.getName());
    m_nameLabel->setDefaultTextColor(preferences.getPortTextColor());

    qreal width = m_nameLabel->boundingRect().width();
    qreal height = m_nameLabel->boundingRect().height();
    repositionPorts(width, height);
}

void Node::addPortPair(
        const Argument& _argument,
        const QString& _type
        )
{
    Preferences& preferences = Preferences::getInstance();
    PortPair* pair = new PortPair(this);
    pair->setArgument(_argument);
    pair->setDefaultTextColor(preferences.getPortTextColor());

    if(_type.compare("i") == 0)
    {
        pair->createInputPort();
    }
    else if(_type.compare("io") == 0)
    {
        pair->createInputPort();
        pair->createOutputPort();
    }
    else if(_type.compare("o") == 0)
    {
        pair->createOutputPort();
    }
    m_ports.append(pair);

    qreal textWidth  = pair->boundingRect().width();
    qreal textHeight = pair->boundingRect().height();
    repositionPorts(textWidth, textHeight);
}

void Node::repositionPorts(
        qreal _width,
        qreal _height
        )
{
    //Expand the Node when the text is too large
    if(_width > m_width - s_horizontalMargin * 2)
    {
        m_width = _width + s_horizontalMargin * 2;
    }
    m_height += _height + s_textSpacing;

    QPainterPath path;
    path.addRect(-m_width / 2, -m_height / 2, m_width, m_height);
    setPath(path);
    int y = s_verticalMargin - m_height / 2;
    m_nameLabel->setPos(-m_nameLabel->boundingRect().width() / 2, y);

    y += _height + s_textSpacing * 4;
    for(int i = 0; i < m_ports.length(); ++i)
    {
        m_ports[i]->setPos(-m_ports[i]->boundingRect().width() / 2, y - _height / 2);
        m_ports[i]->repositionPorts(m_width, y);
        y += _height + s_textSpacing;
    }
}

void Node::paint(
        QPainter* _painter,
        const QStyleOptionGraphicsItem* _option,
        QWidget* _widget
        )
{
    Q_UNUSED(_option);
    Q_UNUSED(_widget);

    Preferences& preferences = Preferences::getInstance();
    if(isSelected())
    {
        _painter->setPen(preferences.getNodePenSelected());
        _painter->setBrush(preferences.getNodeBrushSelected());
    }
    else
    {
        _painter->setPen(preferences.getNodePenUnselected());
        _painter->setBrush(preferences.getNodeBrushUnselected());
    }
    _painter->drawPath(path());
}

int Node::type(
        ) const
{
    return Type;
}

const QString& Node::getName(
        ) const
{
    return m_setting->getName();
}

const NodeSetting* Node::getSetting(
        ) const
{
    return m_setting;
}

void Node::saveToXml(
        QDomElement& _xmlElement
        )
{
    QDomDocument xml;
    QDomElement node = xml.createElement("node");
    node.setAttribute("name", m_setting->getName());
    QDomElement position = xml.createElement("position");
    position.setAttribute("x", QString::number(pos().x()));
    position.setAttribute("y", QString::number(pos().y()));
    node.appendChild(position);

    QDomElement ports = xml.createElement("pairs");
    foreach(PortPair* pair, m_ports)
    {
        pair->saveToXml(ports);
    }
    node.appendChild(ports);
    _xmlElement.appendChild(node);
}

void Node::loadFromXml(
        QDomElement& _xmlNode,
        QMap<quint64, Port*>& o_portMap
        )
{
    Preferences& preferences = Preferences::getInstance();
    NodeLibrary& nodeLibrary = NodeLibrary::getInstance();

    QString nodeType = _xmlNode.attribute("name");
    NodeSetting* setting = nodeLibrary.getNodeSetting(nodeType);
    loadFromNodeSetting(setting);

    QDomNode n = _xmlNode.firstChild();

    while(!n.isNull())
    {
        QDomElement e = n.toElement();
        if(e.tagName().compare("position") == 0)
        {
            setPos(e.attribute("x").toInt(), e.attribute("y").toInt());
        }
        else if(e.tagName().compare("pairs") == 0)
        {
            QDomNode portNode = e.firstChild();
            while(!portNode.isNull())
            {
                QDomElement portElement = portNode.toElement();
                PortPair* pair = 0;
                foreach(PortPair* port, m_ports)
                {
                    if(port->getName().compare(portElement.attribute("name") )== 0)
                    {
                        pair = port;
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
                if(pair)
                {
                    pair->setDefaultTextColor(preferences.getPortTextColor());
                    pair->loadFromXml(portElement, o_portMap);
                }
                portNode = portNode.nextSibling();
            }
        }
        n = n.nextSibling();
    }
}

bool Node::hasAncestor(
        const Node* _node
        ) const
{
    if(this == _node)
    {
        return true;
    }
    foreach(const PortPair* port, m_ports)
    {
        if(port->hasNodeAncestor(_node))
        {
            return true;
        }
    }
    return false;
}

QVector<const Node*> Node::getDescendants(
        ) const
{
    QVector<const Node*> descendants;
    //Add children
    foreach(const PortPair* port, m_ports)
    {
        QVector<const Node*> portDescendant = port->getDescendantNodes();
        foreach (const Node* descendant, portDescendant)
        {
            if(!descendants.contains(descendant))
            {
                descendants.append(descendant);
            }
        }
    }
    return descendants;
}

const QVector<PortPair*>& Node::getPorts(
        ) const
{
    return m_ports;
}

Node::~Node()
{
    foreach(PortPair* port, m_ports)
    {
        delete port;
    }
}