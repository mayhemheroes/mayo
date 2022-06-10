/****************************************************************************
** Copyright (c) 2022, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include "../graphics/graphics_object_ptr.h"

#include <QtWidgets/QWidget>

namespace Mayo {

class GuiDocument;

class WidgetMeasure : public QWidget {
    Q_OBJECT
public:
    WidgetMeasure(GuiDocument* guiDoc, QWidget* parent = nullptr);
    ~WidgetMeasure();

signals:
    void sizeAdjustmentRequested();

private:
    enum class MeasureType {
        None, VertexPosition, CircleCenter, CircleDiameter, MinDistance, Length, Angle, SurfaceArea
    };
    static MeasureType toMeasureType(int comboBoxId);
    void onMeasureTypeChanged(int id);

    class Ui_WidgetMeasure* m_ui= nullptr;
    GuiDocument* m_guiDoc = nullptr;
};

} // namespace Mayo
