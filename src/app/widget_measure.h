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
#include <memory>
#include <string>

namespace Mayo {

class GuiDocument;

enum class MeasureType {
    None, VertexPosition, CircleCenter, CircleDiameter, MinDistance, Length, Angle, SurfaceArea
};

class IMeasureDriver {
public:
    virtual Span<const GraphicsObjectSelectionMode> selectionModes(MeasureType type) const = 0;
    virtual bool supports(const GraphicsObjectPtr& object) const = 0;
    virtual bool supports(MeasureType type) const = 0;

    template<typename T> struct Result : public T {
        bool isValid = false;
        std::string errorMessage;
        T value;
        Result() = default;
        Result(const T& v) : value(v) {}
        Result(T&& v) : value(std::move(v)) {}
    };

    struct MinDistance {
        gp_Pnt pnt1;
        gp_Pnt pnt2;
        QuantityLength distance;
    };

    virtual Result<gp_Pnt> vertexPosition(const GraphicsOwnerPtr& owner) const = 0;
    virtual Result<gp_Pnt> circleCenter(const GraphicsOwnerPtr& owner) const = 0;
    virtual Result<QuantityLength> circleDiameter(const GraphicsOwnerPtr& owner) const = 0;
    virtual Result<MinDistance> minDistance(const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const = 0;
    virtual Result<QuantityLength> length(Span<const GraphicsOwnerPtr> spanOwner) const = 0;
    virtual Result<QuantityAngle> angle(const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const = 0;
    virtual Result<QuantityArea> surfaceArea(Span<const GraphicsOwnerPtr> spanOwner) const = 0;
};

class MeasureShapeDriver : public IMeasureDriver {
public:
    Span<const GraphicsObjectSelectionMode> selectionModes(MeasureType type) const override;
    bool supports(const GraphicsObjectPtr& object) const override;
    bool supports(MeasureType type) const override;

    Result<gp_Pnt> vertexPosition(const GraphicsOwnerPtr& owner) const override;
    Result<gp_Pnt> circleCenter(const GraphicsOwnerPtr& owner) const override;
    Result<QuantityLength> circleDiameter(const GraphicsOwnerPtr& owner) const override;
    Result<MinDistance> minDistance(const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const override;
    Result<QuantityLength> length(Span<const GraphicsOwnerPtr> spanOwner) const override;
    Result<QuantityAngle> angle(const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const override;
    Result<QuantityArea> surfaceArea(Span<const GraphicsOwnerPtr> spanOwner) const override;
};

class WidgetMeasure : public QWidget {
    Q_OBJECT
public:
    WidgetMeasure(GuiDocument* guiDoc, QWidget* parent = nullptr);
    ~WidgetMeasure();

    static void addMeasureDriver(std::unique_ptr<IMeasureDriver> driver);

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
