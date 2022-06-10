/****************************************************************************
** Copyright (c) 2022, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "widget_measure.h"
#include "theme.h"
#include "ui_widget_measure.h"
#include "../gui/gui_document.h"

#include <AIS_Shape.hxx>

namespace Mayo {

WidgetMeasure::WidgetMeasure(GuiDocument* guiDoc, QWidget* parent)
    : QWidget(parent),
      m_ui(new Ui_WidgetMeasure),
      m_guiDoc(guiDoc)
{
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

WidgetMeasure::MeasureType WidgetMeasure::toMeasureType(int comboBoxId)
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

    switch (measureType) {
    case MeasureType::VertexPosition: {
        auto gfxScene = m_guiDoc->graphicsScene();
        gfxScene->foreachDisplayedObject([=](const GraphicsObjectPtr& gfxObject) {
            auto gfxDriver = GraphicsObjectDriver::get(gfxObject);
            if (gfxDriver) {
                m_guiDoc->graphicsScene()->activateObjectSelection(gfxObject, AIS_Shape::SelectionMode(TopAbs_VERTEX));
            }
        });
        break;
    }
    } // endswitch
}

} // namespace Mayo
