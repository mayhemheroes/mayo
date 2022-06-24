/****************************************************************************
** Copyright (c) 2022, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "measure_shape_tool.h"

#include "../base/text_id.h"
#include "../graphics/graphics_shape_object_driver.h"

#include <AIS_Shape.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GProp_GProps.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>

namespace Mayo {

namespace {

const TopoDS_Shape& getShape(const GraphicsOwnerPtr& owner)
{
    static const TopoDS_Shape nullShape;
    auto brepOwner = Handle_StdSelect_BRepOwner::DownCast(owner);
    return brepOwner ? brepOwner->Shape() : nullShape;
}

struct MeasureShapeToolI18N { MAYO_DECLARE_TEXT_ID_FUNCTIONS(Mayo::MeasureShapeTool) };

} // namespace

Span<const GraphicsObjectSelectionMode> MeasureShapeTool::selectionModes(MeasureType type) const
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

bool MeasureShapeTool::supports(const GraphicsObjectPtr& object) const
{
    auto gfxDriver = GraphicsObjectDriver::get(object);
    return gfxDriver ? !GraphicsShapeObjectDriverPtr::DownCast(gfxDriver).IsNull() : false;
}

bool MeasureShapeTool::supports(MeasureType type) const
{
    return type != MeasureType::None;
}

IMeasureTool::Result<gp_Pnt> MeasureShapeTool::vertexPosition(const GraphicsOwnerPtr& owner) const
{
    const TopoDS_Shape& shape = getShape(owner);
    if (shape.IsNull() || shape.ShapeType() != TopAbs_VERTEX)
        return {};

    return BRep_Tool::Pnt(TopoDS::Vertex(shape)).Transformed(owner->Location());
}

IMeasureTool::Result<gp_Circ> MeasureShapeTool::circle(const GraphicsOwnerPtr& owner) const
{
    const TopoDS_Shape& shape = getShape(owner);
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE)
        return MeasureShapeToolI18N::textIdTr("Picked entity must be a circular edge");

    const BRepAdaptor_Curve curve(TopoDS::Edge(shape));
    if (curve.GetType() == GeomAbs_Circle) {
        return curve.Circle().Transformed(owner->Location());
    }
    else if (curve.GetType() == GeomAbs_Ellipse) {
        const gp_Elips ellipse  = curve.Ellipse();
        if (std::abs(ellipse.MinorRadius() - ellipse.MajorRadius()) < Precision::Confusion())
            return gp_Circ(ellipse.Position(), ellipse.MinorRadius()).Transformed(owner->Location());
    }
    else if (curve.GetType() == GeomAbs_BSplineCurve) {
        // TODO Support this case
        // See https://stackoverflow.com/questions/35310195/extract-arc-circle-definition-from-bspline
    }

    return MeasureShapeToolI18N::textIdTr("Picked entity must be a circular edge");
}

IMeasureTool::Result<IMeasureTool::MinDistance> MeasureShapeTool::minDistance(
        const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const
{
    const TopoDS_Shape& shape1 = getShape(owner1);
    if (shape1.IsNull())
        return MeasureShapeToolI18N::textIdTr("First picked entity must be a shape(BREP)");

    const TopoDS_Shape& shape2 = getShape(owner2);
    if (shape2.IsNull())
        return MeasureShapeToolI18N::textIdTr("Second picked entity must be a shape(BREP)");

    const BRepExtrema_DistShapeShape dist(shape1, shape2);
    if (!dist.IsDone())
        return MeasureShapeToolI18N::textIdTr("Computation of minimum distance failed");

    IMeasureTool::MinDistance distResult;
    distResult.pnt1 = dist.PointOnShape1(1).Transformed(owner1->Location());
    distResult.pnt2 = dist.PointOnShape2(1).Transformed(owner2->Location());;
    distResult.distance = dist.Value() * Quantity_Millimeter;
    return distResult;
}

IMeasureTool::Result<QuantityLength> MeasureShapeTool::length(Span<const GraphicsOwnerPtr> spanOwner) const
{
    double len = 0;
    // TODO Parallelize this for-loop. Verify GCPnts_AbscissaPoint::Length() is thread-safe
    for (const GraphicsOwnerPtr& owner : spanOwner) {
        const TopoDS_Shape& shape = getShape(owner);
        if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE)
            return MeasureShapeToolI18N::textIdTr("All picked entities must be edges");

        const BRepAdaptor_Curve curve(TopoDS::Edge(shape));
        len += GCPnts_AbscissaPoint::Length(curve, 1e-6);
    }

    return len * Quantity_Millimeter;
}

IMeasureTool::Result<QuantityAngle> MeasureShapeTool::angle(
        const GraphicsOwnerPtr& owner1, const GraphicsOwnerPtr& owner2) const
{
    const TopoDS_Shape& shape1 = getShape(owner1);
    if (shape1.IsNull())
        return MeasureShapeToolI18N::textIdTr("First picked entity must be a linear edge");

    const TopoDS_Shape& shape2 = getShape(owner2);
    if (shape2.IsNull())
        return MeasureShapeToolI18N::textIdTr("Second picked entity must be a linear edge");

    const BRepAdaptor_Curve curve1(TopoDS::Edge(shape1));
    if (curve1.GetType() != GeomAbs_Line)
        return MeasureShapeToolI18N::textIdTr("First picked entity must be a linear edge");

    const BRepAdaptor_Curve curve2(TopoDS::Edge(shape2));
    if (curve2.GetType() != GeomAbs_Line)
        return MeasureShapeToolI18N::textIdTr("Second picked entity must be a linear edge");

    return {}; // TODO Implement
}

IMeasureTool::Result<QuantityArea> MeasureShapeTool::surfaceArea(Span<const GraphicsOwnerPtr> spanOwner) const
{
    double area = 0;
    // TODO Parallelize this for-loop. Verify BRepGProp::SurfaceProperties() is thread-safe
    for (const GraphicsOwnerPtr& owner : spanOwner) {
        const TopoDS_Shape& shape = getShape(owner);
        if (shape.IsNull() || shape.ShapeType() != TopAbs_FACE)
            return MeasureShapeToolI18N::textIdTr("All picked entities must be faces");

        GProp_GProps gprops;
        BRepGProp::SurfaceProperties(TopoDS::Face(shape), gprops);
        area += gprops.Mass();
    }

    return area * Quantity_SquaredMillimeter;
}


} // namespace Mayo
