/****************************************************************************
** Copyright (c) 2022, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "widget_measure.h"
#include "theme.h"
#include "ui_widget_measure.h"
#include "../gui/gui_document.h"

#include "../graphics/graphics_mesh_object_driver.h"
#include "../graphics/graphics_shape_object_driver.h"

#include <AIS_Shape.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GProp_GProps.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <TopoDS.hxx>

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

Span<const GraphicsObjectSelectionMode> MeasureShapeDriver::selectionModes(MeasureType type) const
{
    switch (type) {
    case MeasureType::VertexPosition: {
        static const GraphicsObjectSelectionMode modes[] = { AIS_Shape::SelectionMode(TopAbs_VERTEX) };
        return modes;
    }
    case MeasureType::CircleCenter:
    case MeasureType::CircleDiameter:
    case MeasureType::Length:
    case MeasureType::Angle: {
        static const GraphicsObjectSelectionMode modes[] = { AIS_Shape::SelectionMode(TopAbs_EDGE) };
        return modes;
    }
    case MeasureType::MinDistance: {
        static const GraphicsObjectSelectionMode modes[] = {
            AIS_Shape::SelectionMode(TopAbs_VERTEX),
            AIS_Shape::SelectionMode(TopAbs_EDGE),
            AIS_Shape::SelectionMode(TopAbs_FACE)
        };
        return modes;
    }
    case MeasureType::SurfaceArea: {
        static const GraphicsObjectSelectionMode modes[] = { AIS_Shape::SelectionMode(TopAbs_FACE) };
        return modes;
    }
    } // endswitch

    return {};
}

bool MeasureShapeDriver::supports(const GraphicsObjectPtr& object) const
{
    auto gfxDriver = GraphicsObjectDriver::get(object);
    return gfxDriver ? !GraphicsShapeObjectDriverPtr::DownCast(gfxDriver).IsNull() : false;
}

bool MeasureShapeDriver::supports(MeasureType type) const
{
    return type != MeasureType::None;
}

namespace {

const TopoDS_Shape& getShape(const GraphicsOwnerPtr& owner)
{
    static const TopoDS_Shape nullShape;
    auto brepOwner = Handle_StdSelect_BRepOwner::DownCast(owner);
    return brepOwner ? brepOwner->Shape() : nullShape;
}

} // namespace

IMeasureDriver::Result<gp_Pnt> MeasureShapeDriver::vertexPosition(const GraphicsOwnerPtr& owner) const
{
    const TopoDS_Shape& shape = getShape(owner);
    if (shape.IsNull() || shape.ShapeType() != TopAbs_VERTEX)
        return {};

    return BRep_Tool::Pnt(TopoDS::Vertex(shape));
}

IMeasureDriver::Result<gp_Pnt> MeasureShapeDriver::circleCenter(const GraphicsOwnerPtr& owner) const
{
    const TopoDS_Shape& shape = getShape(owner);
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE)
        return {};

    const BRepAdaptor_Curve curve(TopoDS::Edge(shape));
    return curve.GetType() == GeomAbs_Circle ? curve.Circle().Location() : gp_Pnt{};
}

IMeasureDriver::Result<QuantityLength> MeasureShapeDriver::circleDiameter(const GraphicsOwnerPtr& owner) const
{
    const TopoDS_Shape& shape = getShape(owner);
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE)
        return {};

    const BRepAdaptor_Curve curve(TopoDS::Edge(shape));
    return curve.GetType() == GeomAbs_Circle ? 2 * curve.Circle().Radius() * Quantity_Millimeter : QuantityLength{};
}

IMeasureDriver::Result<IMeasureDriver::MinDistance> MeasureShapeDriver::minDistance(
        const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const
{
    const TopoDS_Shape& shape1 = getShape(owner1);
    if (shape1.IsNull())
        return {};

    const TopoDS_Shape& shape2 = getShape(owner2);
    if (shape2.IsNull())
        return {};

    const BRepExtrema_DistShapeShape dist(shape1, shape2);
    if (!dist.IsDone())
        return {};

    IMeasureDriver::MinDistance distResult;
    distResult.pnt1 = dist.PointOnShape1(1);
    distResult.pnt2 = dist.PointOnShape2(1);
    distResult.distance = dist.Value() * Quantity_Millimeter;
    return distResult;
}

IMeasureDriver::Result<QuantityLength> MeasureShapeDriver::length(Span<const GraphicsOwnerPtr> spanOwner) const
{
    double len = 0;
    // TODO Parallelize this for-loop. Verify GCPnts_AbscissaPoint::Length() is thread-safe
    for (const GraphicsOwnerPtr& owner : spanOwner) {
        const TopoDS_Shape& shape = getShape(owner);
        if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE)
            return {};

        const BRepAdaptor_Curve curve(TopoDS::Edge(shape));
        len += GCPnts_AbscissaPoint::Length(curve, 1e-6);
    }

    return len * Quantity_Millimeter;
}

IMeasureDriver::Result<QuantityAngle> MeasureShapeDriver::angle(
        const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const
{
    const TopoDS_Shape& shape1 = getShape(owner1);
    if (shape1.IsNull())
        return {};

    const TopoDS_Shape& shape2 = getShape(owner2);
    if (shape2.IsNull())
        return {};

    const BRepAdaptor_Curve curve1(TopoDS::Edge(shape1));
    if (curve1.GetType() != GeomAbs_Line)
        return {};

    const BRepAdaptor_Curve curve2(TopoDS::Edge(shape2));
    if (curve2.GetType() != GeomAbs_Line)
        return {};

    return {}; // TODO Implement
}

IMeasureDriver::Result<QuantityArea> MeasureShapeDriver::surfaceArea(Span<const GraphicsOwnerPtr> spanOwner) const
{
    double area = 0;
    // TODO Parallelize this for-loop. Verify BRepGProp::SurfaceProperties() is thread-safe
    for (const GraphicsOwnerPtr& owner : spanOwner) {
        const TopoDS_Shape& shape = getShape(owner);
        if (shape.IsNull() || shape.ShapeType() != TopAbs_FACE)
            return {};

        GProp_GProps gprops;
        BRepGProp::SurfaceProperties(TopoDS::Face(shape), gprops);
        area += gprops.Mass();
    }

    return area * Quantity_SquaredMillimeter;
}

} // namespace Mayo
