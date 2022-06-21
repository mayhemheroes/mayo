/****************************************************************************
** Copyright (c) 2022, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "widget_measure.h"
#include "app_module.h"
#include "qstring_conv.h"
#include "qstring_utils.h"
#include "theme.h"
#include "ui_widget_measure.h"

#include "../base/unit_system.h"
#include "../gui/gui_document.h"
#include "../graphics/graphics_mesh_object_driver.h"
#include "../graphics/graphics_shape_object_driver.h"

#include <AIS_Point.hxx>
#include <AIS_Shape.hxx>
#include <AIS_TextLabel.hxx>
#include <PrsDim_DiameterDimension.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <gp_Circ.hxx>
#include <Geom_CartesianPoint.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GProp_GProps.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <TopoDS.hxx>

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

void WidgetMeasure::onGraphicsSelectionChanged()
{
    std::vector<GraphicsOwnerPtr> vecSelected;
    m_guiDoc->graphicsScene()->foreachSelectedOwner([&](const GraphicsOwnerPtr& owner) {
        vecSelected.push_back(owner);
    });

    std::vector<GraphicsOwnerPtr> vecNewSelected;
    for (const GraphicsOwnerPtr& owner : vecSelected) {
        auto itFound = std::find(m_vecSelectedOwner.begin(), m_vecSelectedOwner.end(), owner);
        if (itFound == m_vecSelectedOwner.end())
            vecNewSelected.push_back(owner);
    }

    std::vector<GraphicsOwnerPtr> vecDeselected;
    for (const GraphicsOwnerPtr& owner : m_vecSelectedOwner) {
        auto itFound = std::find(vecSelected.begin(), vecSelected.end(), owner);
        if (itFound == vecSelected.end())
            vecDeselected.push_back(owner);
    }

    m_vecSelectedOwner = std::move(vecSelected);
    if (!m_tool)
        return;

    switch (this->currentMeasureType()) {
    case MeasureType::VertexPosition: {
        for (const GraphicsOwnerPtr& owner : vecNewSelected) {
            const IMeasureTool::Result<gp_Pnt> pnt = m_tool->vertexPosition(owner);
            if (pnt.isValid) {
                const QString pntText = QStringUtils::text(pnt.value, AppModule::get()->defaultTextOptions());
                auto gfxText = new AIS_TextLabel;
                gfxText->SetText(to_OccExtString(pntText));
                gfxText->SetPosition(pnt.value);
                m_guiDoc->graphicsScene()->addObject(gfxText);
            }
        }
        break;
    }
    case MeasureType::CircleCenter: {
        for (const GraphicsOwnerPtr& owner : vecNewSelected) {
            const IMeasureTool::Result<gp_Circ> circle = m_tool->circle(owner);
            if (circle.isValid) {
                const QString pntText = QStringUtils::text(circle.value.Location(), AppModule::get()->defaultTextOptions());
                auto gfxText = new AIS_TextLabel;
                gfxText->SetText(to_OccExtString("  " + pntText));
                gfxText->SetPosition(circle.value.Location());
                auto gfxPos = new AIS_Point(new Geom_CartesianPoint(circle.value.Location()));
                gfxText->SetZLayer(Graphic3d_ZLayerId_Topmost);
                gfxPos->SetZLayer(Graphic3d_ZLayerId_Topmost);
                m_guiDoc->graphicsScene()->addObject(gfxText);
                m_guiDoc->graphicsScene()->addObject(gfxPos);
            }
        }
        break;
    }
    case MeasureType::CircleDiameter: {
        for (const GraphicsOwnerPtr& owner : vecNewSelected) {
            const IMeasureTool::Result<gp_Circ> circle = m_tool->circle(owner);
            if (circle.isValid) {
                const QuantityLength diameter = 2 * circle.value.Radius() * Quantity_Millimeter;
                const auto trDiameter = UnitSystem::millimeters(diameter);
                auto gfxDiameter = new PrsDim_DiameterDimension(circle.value);
                gfxDiameter->SetZLayer(Graphic3d_ZLayerId_Topmost);
                gfxDiameter->DimensionAspect()->ArrowAspect()->SetZoomable(false);
                gfxDiameter->DimensionAspect()->ArrowAspect()->SetLength(0.5);
                gfxDiameter->DimensionAspect()->MakeUnitsDisplayed(true);
                gfxDiameter->SetModelUnits("mm");
                gfxDiameter->SetDisplayUnits("cm");
                m_guiDoc->graphicsScene()->addObject(gfxDiameter);
                m_ui->stackedWidget->setCurrentWidget(m_ui->pageResult);
                m_ui->label_Result->setText(tr("Diameter: %1%2").arg(trDiameter.value).arg(trDiameter.strUnit));
            }
            else {
                qWarning() << to_QString(circle.errorMessage);
            }
        }
        break;
    }
    } // endswitch

}

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

namespace {

const TopoDS_Shape& getShape(const GraphicsOwnerPtr& owner)
{
    static const TopoDS_Shape nullShape;
    auto brepOwner = Handle_StdSelect_BRepOwner::DownCast(owner);
    return brepOwner ? brepOwner->Shape() : nullShape;
}

struct MeasureShapeToolI18N { MAYO_DECLARE_TEXT_ID_FUNCTIONS(Mayo::MeasureShapeTool) };

} // namespace

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
    distResult.pnt1 = dist.PointOnShape1(1);
    distResult.pnt2 = dist.PointOnShape2(1);
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
