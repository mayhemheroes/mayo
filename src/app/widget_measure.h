/****************************************************************************
** Copyright (c) 2022, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include "../base/quantity.h"
#include "../base/span.h"
#include "../graphics/graphics_object_ptr.h"
#include "../graphics/graphics_owner_ptr.h"

#include <QtWidgets/QWidget>

#include <gp_Pnt.hxx>
#include <gp_Circ.hxx>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Mayo {

class GuiDocument;

enum class MeasureType {
    None, VertexPosition, CircleCenter, CircleDiameter, MinDistance, Length, Angle, SurfaceArea
};

struct MeasureConfig {
    std::string_view strLengthUnit = "mm";
    std::string_view strAngleUnit = "deg";
};

class IMeasureTool {
public:
    virtual Span<const GraphicsObjectSelectionMode> selectionModes(MeasureType type) const = 0;
    virtual bool supports(const GraphicsObjectPtr& object) const = 0;
    virtual bool supports(MeasureType type) const = 0;

    template<typename T> struct Result {
        bool isValid = false;
        std::string errorMessage;
        T value;
        Result() = default;
        Result(const T& v) : isValid(true), value(v) {}
        Result(T&& v) : isValid(true), value(std::move(v)) {}
        Result(std::string_view errMsg) : isValid(false), errorMessage(errMsg) {}
    };

    struct MinDistance {
        gp_Pnt pnt1;
        gp_Pnt pnt2;
        QuantityLength distance;
    };

    virtual Result<gp_Pnt> vertexPosition(const GraphicsOwnerPtr& owner) const = 0;
    virtual Result<gp_Circ> circle(const GraphicsOwnerPtr& owner) const = 0;
    virtual Result<MinDistance> minDistance(const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const = 0;
    virtual Result<QuantityLength> length(Span<const GraphicsOwnerPtr> spanOwner) const = 0;
    virtual Result<QuantityAngle> angle(const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const = 0;
    virtual Result<QuantityArea> surfaceArea(Span<const GraphicsOwnerPtr> spanOwner) const = 0;
};

class MeasureShapeTool : public IMeasureTool {
public:
    Span<const GraphicsObjectSelectionMode> selectionModes(MeasureType type) const override;
    bool supports(const GraphicsObjectPtr& object) const override;
    bool supports(MeasureType type) const override;

    Result<gp_Pnt> vertexPosition(const GraphicsOwnerPtr& owner) const override;
    Result<gp_Circ> circle(const GraphicsOwnerPtr& owner) const override;
    Result<MinDistance> minDistance(const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const override;
    Result<QuantityLength> length(Span<const GraphicsOwnerPtr> spanOwner) const override;
    Result<QuantityAngle> angle(const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const override;
    Result<QuantityArea> surfaceArea(Span<const GraphicsOwnerPtr> spanOwner) const override;
};

class IMeasure {
public:
    virtual void update(const MeasureConfig& config) = 0;
    virtual std::string text() const = 0;
    virtual int graphicsObjectsCount() const = 0;
    virtual GraphicsObjectPtr graphicsObjectAt(int i) const = 0;
};

class WidgetMeasure : public QWidget {
    Q_OBJECT
public:
    WidgetMeasure(GuiDocument* guiDoc, QWidget* parent = nullptr);
    ~WidgetMeasure();

    void setMeasureOn(bool on);

    static void addTool(std::unique_ptr<IMeasureTool> tool);

signals:
    void sizeAdjustmentRequested();

private:
    void onMeasureTypeChanged(int id);

    static MeasureType toMeasureType(int comboBoxId);
    static std::string_view toMeasureLengthUnit(int comboBoxId);
    static std::string_view toMeasureAngleUnit(int comboBoxId);

    MeasureType currentMeasureType() const;
    MeasureConfig currentMeasureConfig() const;

    void onGraphicsSelectionChanged();

    class Ui_WidgetMeasure* m_ui= nullptr;
    GuiDocument* m_guiDoc = nullptr;
    std::vector<GraphicsOwnerPtr> m_vecSelectedOwner;
    IMeasureTool* m_tool = nullptr;
    QMetaObject::Connection m_connGraphicsSelectionChanged;
};

} // namespace Mayo
