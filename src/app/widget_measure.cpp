/****************************************************************************
** Copyright (c) 2022, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "widget_measure.h"
#include "measure_display.h"
#include "measure_shape_tool.h"
#include "qstring_conv.h"
#include "theme.h"
#include "ui_widget_measure.h"

#include "../base/unit_system.h"
#include "../gui/gui_document.h"

#include <QtCore/QtDebug>

#include <cmath>
#include <vector>

namespace Mayo {

namespace {

using IMeasureToolPtr = std::unique_ptr<IMeasureTool>;
std::vector<IMeasureToolPtr>& getMeasureTools()
{
    static std::vector<IMeasureToolPtr> vecTool;
    return vecTool;
}

IMeasureTool* findSupportingMeasureTool(const GraphicsObjectPtr& gfxObject, MeasureType measureType)
{
    for (const IMeasureToolPtr& ptr : getMeasureTools()) {
        if (ptr->supports(measureType) && ptr->supports(gfxObject))
            return ptr.get();
    }

    return nullptr;
}

} // namespace

WidgetMeasure::WidgetMeasure(GuiDocument* guiDoc, QWidget* parent)
    : QWidget(parent),
      m_ui(new Ui_WidgetMeasure),
      m_guiDoc(guiDoc)
{
    if (getMeasureTools().empty())
        getMeasureTools().push_back(std::make_unique<MeasureShapeTool>());

    m_ui->setupUi(this);
    const QColor msgBackgroundColor = mayoTheme()->color(Theme::Color::MessageIndicator_Background);
    m_ui->label_Message->setStyleSheet(QString("QLabel { background-color: %1 }").arg(msgBackgroundColor.name()));

    QObject::connect(
                m_ui->combo_MeasureType, qOverload<int>(&QComboBox::currentIndexChanged),
                this, &WidgetMeasure::onMeasureTypeChanged
    );

    this->onMeasureTypeChanged(m_ui->combo_MeasureType->currentIndex());
}

WidgetMeasure::~WidgetMeasure()
{
    delete m_ui;
}

void WidgetMeasure::setMeasureOn(bool on)
{
    auto gfxScene = m_guiDoc->graphicsScene();
    if (on) {
        this->onMeasureTypeChanged(m_ui->combo_MeasureType->currentIndex());
        gfxScene->foreachDisplayedObject([=](const GraphicsObjectPtr& gfxObject) {
            gfxScene->foreachActiveSelectionMode(gfxObject, [=](int /*mode*/) {
            });
        });
        m_connGraphicsSelectionChanged = QObject::connect(
                    gfxScene, &GraphicsScene::selectionChanged,
                    this, &WidgetMeasure::onGraphicsSelectionChanged
        );
    }
    else {
        gfxScene->foreachDisplayedObject([=](const GraphicsObjectPtr& gfxObject) {
            gfxScene->deactivateObjectSelection(gfxObject);
            gfxScene->activateObjectSelection(gfxObject, 0);
        });
        QObject::disconnect(m_connGraphicsSelectionChanged);
    }
}

void WidgetMeasure::addTool(std::unique_ptr<IMeasureTool> tool)
{
    if (tool)
        getMeasureTools().push_back(std::move(tool));
}

MeasureType WidgetMeasure::toMeasureType(int comboBoxId)
{
    switch (comboBoxId) {
    case 0: return MeasureType::VertexPosition;
    case 1: return MeasureType::CircleCenter;
    case 2: return MeasureType::CircleDiameter;
    case 3: return MeasureType::MinDistance;
    case 4: return MeasureType::Length;
    case 5: return MeasureType::Angle;
    case 6: return MeasureType::SurfaceArea;
    }
    return MeasureType::None;
}

std::string_view WidgetMeasure::toMeasureLengthUnit(int comboBoxId)
{
    switch (comboBoxId) {
    case 0: return "mm";
    case 1: return "cm";
    case 2: return "m";
    case 3: return "in";
    case 4: return "foot"; // TODO Fix unit_system.cpp
    case 5: return "yd"; // TODO Fix unit_system.cpp
    }
    return {};
}

std::string_view WidgetMeasure::toMeasureAngleUnit(int comboBoxId)
{
    switch (comboBoxId) {
    case 0: return "deg";
    case 1: return "rad";
    }
    return {};
}

void WidgetMeasure::onMeasureTypeChanged(int id)
{
    const MeasureType measureType = WidgetMeasure::toMeasureType(id);
    const bool measureIsLengthBased = measureType != MeasureType::Angle;
    const bool measureIsAngleBased = measureType == MeasureType::Angle;
    m_ui->label_LengthUnit->setVisible(measureIsLengthBased);
    m_ui->combo_LengthUnit->setVisible(measureIsLengthBased);
    m_ui->label_AngleUnit->setVisible(measureIsAngleBased);
    m_ui->combo_AngleUnit->setVisible(measureIsAngleBased);
    emit this->sizeAdjustmentRequested();

    m_vecSelectedOwner.clear();
    auto gfxScene = m_guiDoc->graphicsScene();

    // Find measure tool
    m_tool = nullptr;
    gfxScene->foreachDisplayedObject([=](const GraphicsObjectPtr& gfxObject) {
        if (!m_tool)
            m_tool = findSupportingMeasureTool(gfxObject, measureType);
    });

    gfxScene->clearSelection();
    gfxScene->foreachDisplayedObject([=](const GraphicsObjectPtr& gfxObject) {
        gfxScene->deactivateObjectSelection(gfxObject);
        if (m_tool) {
            for (GraphicsObjectSelectionMode mode : m_tool->selectionModes(measureType))
                gfxScene->activateObjectSelection(gfxObject, mode);
        }
    });
    gfxScene->redraw();
}

MeasureType WidgetMeasure::currentMeasureType() const
{
    return WidgetMeasure::toMeasureType(m_ui->combo_MeasureType->currentIndex());
}

MeasureConfig WidgetMeasure::currentMeasureConfig() const
{
    return {
        WidgetMeasure::toMeasureLengthUnit(m_ui->combo_LengthUnit->currentIndex()),
        WidgetMeasure::toMeasureAngleUnit(m_ui->combo_AngleUnit->currentIndex())
    };
}

void WidgetMeasure::onGraphicsSelectionChanged()
{
    auto gfxScene = m_guiDoc->graphicsScene();
    std::vector<GraphicsOwnerPtr> vecNewSelected;
    std::vector<GraphicsOwnerPtr> vecDeselected;
    {
        std::vector<GraphicsOwnerPtr> vecSelected;
        gfxScene->foreachSelectedOwner([&](const GraphicsOwnerPtr& owner) {
            vecSelected.push_back(owner);
        });

        for (const GraphicsOwnerPtr& owner : vecSelected) {
            auto itFound = std::find(m_vecSelectedOwner.begin(), m_vecSelectedOwner.end(), owner);
            if (itFound == m_vecSelectedOwner.end())
                vecNewSelected.push_back(owner);
        }

        for (const GraphicsOwnerPtr& owner : m_vecSelectedOwner) {
            auto itFound = std::find(vecSelected.begin(), vecSelected.end(), owner);
            if (itFound == vecSelected.end())
                vecDeselected.push_back(owner);
        }

        m_vecSelectedOwner = std::move(vecSelected);
    }

    if (!m_tool)
        return;

    for (const GraphicsOwnerPtr& owner : vecDeselected) {
        for (auto link = this->findLink(owner); link != nullptr; link = this->findLink(owner)) {
            auto itMeasure = std::find_if(
                        m_vecMeasure.begin(), m_vecMeasure.end(),
                        [=](const IMeasureDisplayPtr& measure) { return measure.get() == link->measure; }
            );
            if (itMeasure != m_vecMeasure.end())
                m_vecMeasure.erase(itMeasure);

            this->eraseLink(link);
        }
    }

    std::vector<IMeasureDisplayPtr> vecNewMeasure;
    switch (this->currentMeasureType()) {
    case MeasureType::VertexPosition: {
        for (const GraphicsOwnerPtr& owner : vecNewSelected) {
            const IMeasureTool::Result<gp_Pnt> pnt = m_tool->vertexPosition(owner);
            if (pnt.isValid) {
                vecNewMeasure.push_back(std::make_unique<MeasureDisplayVertex>(pnt.value));
                this->addLink(owner, vecNewMeasure.back());
            }
        }
        break;
    }
    case MeasureType::CircleCenter: {
        for (const GraphicsOwnerPtr& owner : vecNewSelected) {
            const IMeasureTool::Result<gp_Circ> circle = m_tool->circle(owner);
            if (circle.isValid) {
                vecNewMeasure.push_back(std::make_unique<MeasureDisplayCircleCenter>(circle.value));
                this->addLink(owner, vecNewMeasure.back());
            }
        }
        break;
    }
    case MeasureType::CircleDiameter: {
        for (const GraphicsOwnerPtr& owner : vecNewSelected) {
            const IMeasureTool::Result<gp_Circ> circle = m_tool->circle(owner);
            if (circle.isValid) {
                vecNewMeasure.push_back(std::make_unique<MeasureDisplayCircleDiameter>(circle.value));
                this->addLink(owner, vecNewMeasure.back());
            }
        }
        break;
    }
    case MeasureType::MinDistance: {
        if (m_vecSelectedOwner.size() == 2) {
            const auto minDist = m_tool->minDistance(m_vecSelectedOwner.front(), m_vecSelectedOwner.back());
            if (minDist.isValid) {
                vecNewMeasure.push_back(std::make_unique<MeasureDisplayMinDistance>(minDist.value));
                this->addLink(m_vecSelectedOwner.front(), vecNewMeasure.back());
                this->addLink(m_vecSelectedOwner.back(), vecNewMeasure.back());
            }
        }
        break;
    }
    } // endswitch

    for (IMeasureDisplayPtr& measure : vecNewMeasure) {
        measure->update(this->currentMeasureConfig());
        for (int i = 0; i < measure->graphicsObjectsCount(); ++i) {
            const GraphicsObjectPtr gfxObject = measure->graphicsObjectAt(i);
            gfxObject->SetZLayer(Graphic3d_ZLayerId_Topmost);
            gfxScene->addObject(gfxObject);
        }

        m_vecMeasure.push_back(std::move(measure));
    }

    m_ui->stackedWidget->setCurrentWidget(m_ui->pageResult);
    QString strResult;
    for (const IMeasureDisplayPtr& measure : m_vecMeasure) {
        const std::string strMeasure = measure->text();
        if (!strMeasure.empty()) {
            if (!strResult.isEmpty())
                strResult += "\n";

            strResult += to_QString(strMeasure);
        }
    }

    m_ui->label_Result->setText(strResult);
    emit this->sizeAdjustmentRequested();
}

void WidgetMeasure::addLink(const GraphicsOwnerPtr& owner, const IMeasureDisplayPtr& measure)
{
    if (owner && measure)
        m_vecLinkGfxOwnerMeasure.push_back({ owner, measure.get() });
}

void WidgetMeasure::eraseLink(const GraphicsOwner_MeasureDisplay* link)
{
    m_vecLinkGfxOwnerMeasure.erase(m_vecLinkGfxOwnerMeasure.begin() + (link - &m_vecLinkGfxOwnerMeasure.front()));
}

const WidgetMeasure::GraphicsOwner_MeasureDisplay* WidgetMeasure::findLink(const GraphicsOwnerPtr& owner) const
{
    auto itFound = std::find_if(
                m_vecLinkGfxOwnerMeasure.begin(),
                m_vecLinkGfxOwnerMeasure.end(),
                [=](const GraphicsOwner_MeasureDisplay& link) { return link.gfxOwner == owner; }
    );
    return itFound != m_vecLinkGfxOwnerMeasure.end() ? &(*itFound) : nullptr;
}

} // namespace Mayo
